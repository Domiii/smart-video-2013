#include "SmartVideo.h"

#include <iomanip>

#include "FileUtil.h"
#include <functional>

using namespace cv;
using namespace std;
using namespace Util;


namespace SmartVideo
{
    /// Read config file
    bool SmartVideoConfig::InitializeConfig()
    {
        std::string cfgPath(CfgFolder + "/" + CfgFile);
        cfgRoot = JSonReadFile(cfgPath);
        if (!cfgRoot) return false;

        // TODO: Video files

        DataFolder = JSonGetProperty(cfgRoot, "dataDir")->GetStringValue();
        ClipinfoDir = JSonGetProperty(cfgRoot, "clipDir")->GetStringValue();
        ClipListFile = JSonGetProperty(cfgRoot, "clipFile")->GetStringValue();
        ForegroundDir = JSonGetProperty(cfgRoot, "fgDir")->GetStringValue();
        DisplayFrames = JSonGetProperty(cfgRoot, "displayResults")->int_value != 0;
        LearningRate = JSonGetProperty(cfgRoot, "learningRate")->float_value;
        CachedImageType = JSonGetProperty(cfgRoot, "cachedImageType")->GetStringValue();
        UseCachedForForeground = JSonGetProperty(cfgRoot, "useCachedForForeground")->int_value > 0;

        std::string clipListPath(GetClipListPath());
        json_value * clipRoot = JSonReadFile(clipListPath);
        if (!clipRoot) return false;

        ClipEntries.resize(clipRoot->children.size());

        int i = 0;
        for (auto&  x : clipRoot->children)
        {
            ClipEntry& entry = ClipEntries[i++];
            json_value * entryNode = x.second;
            entry.Name = entryNode->name;
            entry.Type = JSonGetProperty(entryNode, "type")->GetStringValue() == "img" ? ClipType::ImageSequence : ClipType::Video;
            entry.BaseFolder = JSonGetProperty(entryNode, "baseDir")->GetStringValue();
            entry.ClipFile = JSonGetProperty(entryNode, "frameFile")->GetStringValue();
            entry.WeightFile = JSonGetProperty(entryNode, "weightFile")->GetStringValue();
            entry.StartFrame = JSonGetProperty(entryNode, "startFrame")->int_value;


            if (entry.Type == ClipType::ImageSequence)
            {
                entry.Filenames = ReadLines(GetFrameFilePath(entry));
            }
            else
            {
                auto fname = GetVideoFile(entry);
                entry.Video = std::move(VideoCapture(fname));

                if(!entry.Video.isOpened())
                {
                    //error in opening the video input
                    cerr << "Unable to open video file: " << fname << endl;
                    exit(EXIT_FAILURE);
                }
                entry.Video.set(CV_CAP_PROP_POS_FRAMES, entry.StartFrame);
            }
        }

        // TODO: Properly delete the JSon nodes.

        return true;
    }


    /// The class that does the "SmartVideo" processing.
    void SmartVideoProcessor::InitProcessing(ClipEntry* clipEntry)
    {
        this->clipEntry = clipEntry;

        // wait for previous I/O operations to stop
        ioPool.Stop();
        ioPool.Join();

        pMOG = unique_ptr<BackgroundSubtractorMOG>(new BackgroundSubtractorMOG()); //MOG approach
        frameInBuffer.Clear();
        frameOutBuffer.Clear();

        // initialize object tracking variables
        InitObjectTracking();

        // allocate frame weights
        frameWeights.resize(clipEntry->GetFrameCount());
        if (Config.DisplayFrames)
        {
            // create GUI windows (for debugging purposes)
            namedWindow("Frame");
            namedWindow("Object");
            namedWindow("Foreground");
        }

        auto nTotalFrames = clipEntry->GetFrameCount() - 1;

        // start I/O queue
        ioPool.AddWorkers(Config.NReadThreads, std::bind(&SmartVideoProcessor::ReadNextInputFrame, this, std::placeholders::_1));

        cout << "Processing " << clipEntry->Name << "..." << endl;

        progressBar.InitProgressBar(nTotalFrames);
    }


    /// Compute some measure of frame "importance".
    float SmartVideoProcessor::ComputeFrameWeight(FrameInfo& frameInfo)
    {
        const float coefFgArea = 1.0;
        const float coefMatchingCost = 1e-4;
        const float coefNumObject = 800.0;
        //cerr << frameInfo.fgArea << " " << frameInfo.matchingCost << " " << frameInfo.numObject << endl;
        float wt = coefFgArea*frameInfo.fgArea + coefMatchingCost*frameInfo.matchingCost + coefNumObject*frameInfo.numObject;
        
        return wt;
    }


    /// Finalize processing.
    void SmartVideoProcessor::FinishProcessing()
    {
        cout << endl;
        if (clipEntry->Video.isOpened())
        {
            // delete capture object
            clipEntry->Video.release();
        }

        if (clipEntry->WeightFile.size() > 0)
        {
            // write weight file
            WriteLines(Config.GetWeightsPath(*clipEntry), frameWeights);
            cout << "Done.";
        }
        else
        {
            cout << "WARNING: No Weight file given. Results have not been stored.";
        }
        cout << endl << endl;

        Cleanup();
    }

    /// Process sequence of images.
    void SmartVideoProcessor::ProcessClip(ClipEntry& clipEntry) 
    {
        // initialize
        InitProcessing(&clipEntry);

        // iterate over all files:
        for (iNextProcessFrame = clipEntry.StartFrame; iNextProcessFrame < clipEntry.GetFrameCount(); ++iNextProcessFrame)
        {
            // process image
            ProcessNextFrame();
        }

        // smooth weight vector
        SmoothWeights();

        // finalize the process
        FinishProcessing();
    }


    void SmartVideoProcessor::ProcessNextFrame()
    {
        // get next frame from queue
        FrameInfo info = frameInBuffer.Pop();
        
        // main stuffs
        BackgroundSubtraction(info);
        ObjectTracking(info);

        // compute and set weight
        //cerr << "frameweight:" << ComputeFrameWeight(info) << endl;
        SetWeight(iNextProcessFrame, ComputeFrameWeight(info));

        // draw progress
        UpdateDisplay(info);
    }


    void SmartVideoProcessor::BackgroundSubtraction(FrameInfo& info) {
        if(!Config.UseCachedForForeground) {
            // denoise
            /*Mat tmp(info.Frame);
            cv::boxFilter(info.Frame, tmp, 1, cv::Size(3,3), cv::Point(1,1));
            info.Frame = tmp;*/

            //update the background model
            pMOG->operator()(info.Frame, info.FrameForegroundMask, Config.LearningRate);
        }
    }

    /*bool SmartVideoProcessor::ClusterWithK(const Mat& __fgmask, int maxCluster, Mat3f& clmask) {
        Mat fgmask, nzPixels;
        __fgmask.copyTo(fgmask);
        cvflann::KMeansIndexParams kmeanParams (
            2 // branching factor = 32
            // iterations = 11
            // center init = CENTERS_RANDOM
            // cb_index = 0.2
            );
        findNonZero(fgmask, nzPixels);
        //cerr << nzPixels.size().height << " x " << nzPixels.size().width << endl;

        // FIXME: autogenerate this later!
        const int maxColor = 9;
        const Vec3f color[maxColor] = {Vec3f(1.0,0.0,0.0), Vec3f(0.0,1.0,0.0), Vec3f(0.0,0.0,1.0),
                                       Vec3f(0.4,0.0,0.0), Vec3f(0.0,0.4,0.0), Vec3f(0.0,0.0,0.4),
                                       Vec3f(1.0,0.6,0.6), Vec3f(0.6,1.0,0.6), Vec3f(0.6,0.6,1.0)};
        assert(maxCluster<maxColor);

        if(maxCluster == 0) return nzPixels.size().height==0;

        const int violateLimit = 40;
        const double threshold = 5;

        int ncluster;
        Mat1f features(nzPixels.size().height, 2);
        Mat1f centers(maxCluster, 2);

        if(nzPixels.size().height > 0) {
            for(int i=0; i<nzPixels.size().height; i++) {
                features[i][0] = nzPixels.at<Point>(i).y;
                features[i][1] = nzPixels.at<Point>(i).x;
                //cerr << "(" << features[i][0] << "," << features[i][1] << ")" << endl;
            }
            // Because of the way the cut in the hierarchical tree is choosen, the number of clusters computed will
            // be the highest number of the form ($branching$-1)*k+1 lower than the number of clusers desired
            ncluster = cv::flann::hierarchicalClustering<cv::flann::L2<float>> (
                            features,
                            centers,
                            kmeanParams
                        );
        } else {
            ncluster = 0;
        }

        // Actually determine proper cluster
        Mat1f clusterID(ncluster,1);
        Mat1f dummy;
        for(int i=0; i<ncluster; i++)
            clusterID[i][0] = i;
        centers = centers(Rect(0,0,2,ncluster));

        int violateCnt = 0;

        if(ncluster) {
            // set clmask
            CvKNearest knn( centers, clusterID, dummy, false, 2 );
            Mat1f results;
            Mat1f distance;
            knn.find_nearest(features,1,results,Mat(),distance);
            // count violation
            //CvKNearest knns[20];
            vector<int> pixelCnt(ncluster,0);
            for(int i=0; i<features.size().height; i++) {
                int r = (int)features[i][0];
                int c = (int)features[i][1];
                int id = (int)results[i][0];
                pixelCnt[id]++;
            }
            vector<Mat1f> pix(ncluster);
            for(int id=0; id<ncluster; id++) {
                pix[id] = Mat1f(pixelCnt[id],2);
                pixelCnt[id] = 0;
            }
            for(int i=0; i<features.size().height; i++) {
                int r = (int)features[i][0];
                int c = (int)features[i][1];
                int id = (int)results[i][0];
                pix[id][pixelCnt[id]][0] = r;
                pix[id][pixelCnt[id]][1] = c;
                pixelCnt[id]++;
            }
            for(int id=0; id<ncluster; id++) {
                Mat1f dummyResponse(pixelCnt[id],1);
                Mat dummyIdx(pixelCnt[id], 1, CV_8SC1);
                //knns[id] = CvKNearest(pix[id],dummyResponse,dummyIdx,false,2);
                for(int i=0; i<pixelCnt[id]; i++)
                    dummyResponse[i][0] = (float)i;
                //centers = pix[id](Rect(0,0,centers.size().width,centers.size().height));
                CvKNearest knnid(pix[id],dummyResponse,dummyIdx,false,2);
                //knns[id].train(pix[id],dummyResponse,dummyIdx,false,2);
                knnid.find_nearest(pix[id],2,Mat(),Mat(),distance);
                for(int i=0; i<pixelCnt[id]; i++) {
                    double d = max(distance[i][0],distance[i][1]); // distance to nearest pixel
                    if(d>threshold) {
                        //cerr << "violate_d: " << d << min(distance[i][0],distance[i][1]) << endl;
                        violateCnt++;
                    }
                }
            }
            if( violateCnt <= violateLimit ) {
                for(int i=0; i<features.size().height; i++) {
                    int r = (int)features[i][0];
                    int c = (int)features[i][1];
                    int id = (int)results[i][0];
                    clmask.at<Vec3f>(r,c) = color[id];
                }
            }
        }
        
        cerr << "violateCnt = " << violateCnt << endl;
        return violateCnt <= violateLimit;
    }*/

    void SmartVideoProcessor::InitObjectTracking() {
        prevObject.clear();
        curObject.clear();
    }
    void SmartVideoProcessor::ObjectTracking(FrameInfo &frameInfo) {
        if(true/*!Config.UseCachedForObjectDetection*/) {
            cvflann::Logger::setLevel(cvflann::FLANN_LOG_INFO); // FIXME: remove later

            //string fgdump = Config.GetForegroundFolder() + "/" + frameInfo.FrameName + "." + Config.CachedImageType;
            Mat fgmask = frameInfo.FrameForegroundMask;
            //cv::cvtColor(fgmask, fgmask, CV_BGR2GRAY); // convert to greyscale

            // erode and dilate to get rid of noises?
            const int ErosionSize = 1;
            const int DilateSize = 2;
            Mat erosionKernel = getStructuringElement(MORPH_ELLIPSE,
                Size(2*ErosionSize+1,2*ErosionSize+1),
                Point(ErosionSize,ErosionSize));
            erode(fgmask, fgmask, erosionKernel);
            Mat dilateKernel = getStructuringElement(MORPH_ELLIPSE,
                Size(2*DilateSize+1,2*DilateSize+1),
                Point(DilateSize,DilateSize));
            dilate(fgmask, fgmask, erosionKernel);

            // hierarchical clustering
            // FIXME: change to agglomerative clustering
            /*while(!ClusterWithK(fgmask, curCluster, clmask)) {
            curCluster++;
            }
            while(curCluster>0 && ClusterWithK(fgmask, curCluster-1, clmask)) {
            curCluster--;
            }*/

            const double dthreshold = 30.0; // FIXME: what are better options?
            const int cthreshold = 64; //25;

            vector<Agglomerative::Point2D> pix;
            Mat nzPixels;
            findNonZero(fgmask, nzPixels);
            for(int i=0; i<nzPixels.size().height; i++)
                pix.push_back(Agglomerative::Point2D(nzPixels.at<Point>(i).y,nzPixels.at<Point>(i).x));
            Agglomerative::AgglomerativeClustering agc(pix);
            vector<Agglomerative::Result> agcResult = agc.cluster(dthreshold,cthreshold);

            Mat3f clmask = Mat3f(fgmask.size().height, fgmask.size().width, Vec3f(0.0,0.0,0.0));
            //cv::cvtColor(fgmask, frameInfo.FrameObjectDetection, CV_GRAY2RGB); // convert to greyscale
            // FIXME: autogenerate this later!
            /*const int maxColor = 9;
            const Vec3f color[maxColor] = {Vec3f(1.0,0.0,0.0), Vec3f(0.0,1.0,0.0), Vec3f(0.0,0.0,1.0),
            Vec3f(0.4,0.0,0.0), Vec3f(0.0,0.4,0.0), Vec3f(0.0,0.0,0.4),
            Vec3f(1.0,0.6,0.6), Vec3f(0.6,1.0,0.6), Vec3f(0.6,0.6,1.0)};
            //assert(maxCluster<maxColor);
            for(auto a: agcResult) {
            clmask.at<Vec3f>(a.pt.x, a.pt.y) = color[a.id];
            }*/

            // Establish currentObjects
            curObject.clear();
            for(auto& a: agcResult) {
                if(a.id>=curObject.size()) curObject.resize(a.id+1);
                curObject[a.id].addPixel(a.pt.x,a.pt.y);
            }
            for(auto& obj: curObject) {
                obj.statistics();
            }
            vector<vector<int>> adj(prevObject.size());
            //if(prevObject.size()) { // only do matching if previous objects are present
            Matcher::ClusterMatcher cm(Matcher::obj2cinfo(prevObject), Matcher::obj2cinfo(curObject));
            vector<pair<int,int>> matching;
            frameInfo.matchingCost = cm.solve(matching);
            // associate related pairs
            for(auto rel: matching) {
                int pv = rel.first;
                int cv = rel.second;
                adj[pv].push_back(cv);
            }
            //}
            for(int i=0; i<prevObject.size(); i++) {
                if(adj[i].size() == 0) continue; // dead-end cluster
                const auto& po = prevObject[i];
                int ind = 0;
                for(auto cc: po.colorProfile) {
                    int to = adj[i][ind++];
                    if(ind==adj[i].size()) ind=0;
                    curObject[to].adoptColor(cc);
                }
            }
            for(auto& co: curObject) {
                if(co.colorProfile.size()==0) co.adoptColor(ColorProfile::randomProfile());
            }

            // record frame weight informatinos
            frameInfo.numObject = curObject.size();
            frameInfo.matchingCost; // recorded in the section of hungarian matching
            frameInfo.fgArea = nzPixels.size().height;

            //frameInfo.FrameObjectDetection = frameInfo.Frame*0.2;
            /*for(int i=0; i<nzPixels.size().height; i++) {
                int r = nzPixels.at<Point>(i).y;
                int c = nzPixels.at<Point>(i).x;
                //frameInfo.FrameObjectDetection.at<Vec3s>(r,c) *= 3;
            }*/
            // draw bounding boxes
            for(auto co: curObject) {
                ColorProfile cp = co.avgColor();
                rectangle(clmask, Point(co.y1,co.x1), Point(co.y2,co.x2), Scalar(cp.r,cp.g,cp.b), 2);
                //rectangle(frameInfo.FrameObjectDetection, Point(co.y1,co.x1), Point(co.y2,co.x2), Scalar(cp.r,cp.g,cp.b), 2);
            }
            frameInfo.FrameObjectDetection = clmask;

            prevObject = curObject;

            // cerr << "ncluster = curObject.size() = " << curObject.size() << endl;
        }
    }

    void SmartVideoProcessor::SmoothWeights()
    {
        const int halfWindow = 5;
        int cc = 0;
        float runningSum = 0.0;
        vector<float> newWeights;
        newWeights.reserve(frameWeights.size());
        for(size_t i=0; i<frameWeights.size(); i++) {
            if(i-halfWindow>=0) {
                cc--;
                runningSum -= frameWeights[i-halfWindow];
            }
            if(i+halfWindow<frameWeights.size()) {
                cc++;
                runningSum += frameWeights[i+halfWindow];
            }
            double w = runningSum / cc;
            newWeights.push_back(w);
        }
        frameWeights = newWeights;
    }

    void SmartVideoProcessor::UpdateDisplay(FrameInfo& info)
    {
        stringstream strstr;
        string statusString;
        strstr << " -- input buffer: " << frameInBuffer.GetSize() << "/" << Config.MaxIOQueueSize << "";
        statusString = strstr.str();
        progressBar.UpdateProgress(info.FrameIndex, statusString);
        
        if (Config.DisplayFrames)
        {
            //// display frame number in viewer
            //const Scalar black(255,255,255);
            //const Scalar white(0,0,0);
            //// clear text area
            //rectangle(frame, cv::Point(10, 2), cv::Point(100, 20), black, -1);
            //// draw frame number
            //putText(frame, frameNumberString.c_str(), cv::Point(15, 15), FONT_HERSHEY_SIMPLEX, 0.5 , white);

            //show the current frame and the fg mask
            imshow("Frame", info.Frame);
            imshow("Foreground", info.FrameForegroundMask);
            imshow("Object", info.FrameObjectDetection);

            waitKey(12);        // TODO: Add a way to better control FPS
        }

        // Dump the foreground information
        std::string outfile = Config.GetForegroundFolder() + "/" + info.FrameName + "." + Config.CachedImageType;  // save as CachedImageType
        try 
        {
            MkDir(Config.GetForegroundFolder());        // make sure that folder exists
            bool saved = imwrite(outfile, info.FrameForegroundMask);
            if (!saved) 
            {
                cerr << "Unable to save " << outfile << endl;
            }
        } 
        catch (runtime_error& ex) 
        {
            cerr << "Exception dumping foreground image in ." << Config.CachedImageType << " format: " << ex.what() << endl;
            exit(0);
        }
    }


    bool SmartVideoProcessor::ReadNextInputFrame(JobIndex iFrame)
    {
        // TODO: Also write out buffer back to file

        iFrame += clipEntry->StartFrame;
        
        auto nFrameCount = clipEntry->GetFrameCount();
        if (iFrame >= nFrameCount)
        {
            return false;
        }

        if (!clipEntry)
        {
            // yield time slice
            this_thread::sleep_for(chrono::milliseconds(1));
            return true;
        }
        

        FrameInfo frameInfo(iFrame);
        frameInfo.FrameName = ToString(iFrame);
        if (clipEntry->Type == ClipType::Video)
        {
            // read frame from video
            if (!clipEntry->Video.read(frameInfo.Frame) || frameInfo.Frame.total() == 0)
            {
                cerr << "ERROR: Unable to read next frame (#" << frameInfo.FrameIndex << ") from video." << endl;
                cout << "Press ENTER to exit." << endl; cin.get();
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            // read frame from image
            string folder = Config.GetClipFolder(*clipEntry);
            auto fname = *(clipEntry->Filenames.begin() + iFrame);
            string fpath = folder + "/" + fname;

            frameInfo.Frame = imread(fpath);
            if(!frameInfo.Frame.data)
            {
                // error in opening an image file
                cerr << "Unable to open image frame: " << fpath << endl;
                exit(EXIT_FAILURE);
            } 
        }

        // add image to queue
        frameInBuffer.Push(frameInfo);
        return true;
    }


    void SmartVideoProcessor::Cleanup()
    {
        if (Config.DisplayFrames)
        {
            cvDestroyWindow("Frame");
            cvDestroyWindow("Foreground");
            cvDestroyWindow("Object");
        }

        ioPool.Stop();
        ioPool.Join();
    }
}
