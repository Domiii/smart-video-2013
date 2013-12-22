#include "SmartVideo.h"

#include <iomanip>

#include "FileUtil.h"

using namespace cv;
using namespace std;
using namespace Util;


namespace SmartVideo
{
    /// Read all config files
    bool SmartVideoConfig::InitializeConfig()
    {
        std::string cfgPath(CfgFolder + "/" + CfgFile);
        json_value * cfgRoot = JSonReadFile(cfgPath);
        if (!cfgRoot) return false;

        DataFolder = JSonGetProperty(cfgRoot, "dataDir")->GetStringValue();
        ClipinfoDir = JSonGetProperty(cfgRoot, "clipDir")->GetStringValue();
        ClipListFile = JSonGetProperty(cfgRoot, "clipFile")->GetStringValue();
        ForegroundDir = JSonGetProperty(cfgRoot, "fgDir")->GetStringValue();
        DisplayFrames = JSonGetProperty(cfgRoot, "displayResults")->int_value != 0;
        LearningRate = JSonGetProperty(cfgRoot, "learningRate")->float_value;
        CachedImageType = JSonGetProperty(cfgRoot, "cachedImageType")->GetStringValue();
        UseCachedForForeground = (bool)JSonGetProperty(cfgRoot, "useCachedForForeground")->int_value;

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
            entry.BaseFolder = JSonGetProperty(entryNode, "baseDir")->GetStringValue();
            entry.ClipFile = JSonGetProperty(entryNode, "frameFile")->GetStringValue();
            entry.WeightFile = JSonGetProperty(entryNode, "weightFile")->GetStringValue();
            entry.StartFrame = JSonGetProperty(entryNode, "startFrame")->int_value;
            entry.Filenames = ReadLines(GetFrameFilePath(entry));
        }

        // TODO: Properly delete the JSon nodes.

        return true;
    }


    /// The class that does the "SmartVideo" processing.
    void SmartVideoProcessor::InitProcessing(const ClipEntry* clipEntry)
    {
        this->clipEntry = clipEntry;
        pMOG = unique_ptr<BackgroundSubtractorMOG>(new BackgroundSubtractorMOG()); //MOG approach
        iFrameNumber = 0;

        // allocate frame weights
        frameWeights.resize(clipEntry->GetFrameCount());
        if (Config.DisplayFrames)
        {
            // create GUI windows (for debugging purposes)
            namedWindow("Frame");
            namedWindow("Foreground");
        }

        auto nTotalFrames = clipEntry->GetFrameCount() - 1;

        cout << "Processing " << clipEntry->Name << "..." << endl;

        progressBar.InitProgressBar(nTotalFrames);
    }


    /// Processes the current frame.
    void SmartVideoProcessor::ProcessInputFrame()
    {
        //update the background model
        pMOG->operator()(frame, foregroundMask, Config.LearningRate);

        // compute and set weight
        SetWeight(iFrameNumber, ComputeFrameWeight(foregroundMask));

        // draw progress
        UpdateDisplay();
    }


    /// Compute some measure of frame "importance".
    float SmartVideoProcessor::ComputeFrameWeight(cv::Mat frame)
    {
        uint32 nForegroundPixels  = 0;

        // count all foreground pixels
        int cols = frame.cols, rows = frame.rows;
        if (frame.isContinuous())
        {
            cols *= rows;
            rows = 1;
        }
        for(int i = 0; i < rows; i++)
        {
            auto Mi = frame.ptr<uchar>(i);
            for(int j = 0; j < cols; j++)
            {
                auto value = Mi[j];
                if (value > 0)
                    ++nForegroundPixels;
            }
        }

        // return weight as ratio of foreground to total pixels
        float ratio = static_cast<float>(nForegroundPixels) / (cols * rows);
        return ratio;
    }

    /// Finalize processing.
    void SmartVideoProcessor::FinishProcessing()
    {
        cout << endl;
        if (clipEntry->WeightFile.size() > 0)
        {
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



    /// Process video.
    void SmartVideoProcessor::ProcessVideo(const ClipEntry& clipEntry)
    {
        InitProcessing(&clipEntry);
        // TODO: Video support

        ////create the capture object
        //VideoCapture capture(videoFilename);
        //if(!capture.isOpened()){
        //    //error in opening the video input
        //    cerr << "Unable to open video file: " << videoFilename << endl;
        //    exit(EXIT_FAILURE);
        //}

        //// read input data until EOF
        //while(capture.read(frame)){
        //    ProcessInputFrame();
        //}

        //// delete capture object
        //capture.release();

        FinishProcessing();
    }

    /// Process sequence of images.
    void SmartVideoProcessor::ProcessImages(const ClipEntry& clipEntry) 
    {
        // initialize
        cerr << "Initializing..." << endl;
        InitProcessing(&clipEntry);

        // main stuffs
        cerr << "BackgroundSubtraction..." << endl;
        BackgroundSubtraction(clipEntry);
        //cerr << "ObjectDetection..." << endl;
        //ObjectDetection(clipEntry);
        cerr << "ObjectTracking..." << endl;
        ObjectTracking(clipEntry);

        // finalize the process
        FinishProcessing();
    }

    void SmartVideoProcessor::BackgroundSubtraction(const ClipEntry& clipEntry) {
        if(!Config.UseCachedForForeground) {
            string folder = Config.GetClipFolder(clipEntry);
            // iterate over all files:
            //iFrameNumber = clipEntry.StartFrame;
            iFrameNumber = 0;
            for_each(clipEntry.Filenames.begin() + iFrameNumber, clipEntry.Filenames.end(), [&](const string& fname) {
                // read image file
                string fpath = folder + "/" + fname;
                frameName = fname;
                frame = imread(fpath);
                if(!frame.data)
                {
                    // error in opening an image file
                    cerr << "Unable to open image frame: " << fpath << endl;
                    exit(EXIT_FAILURE);
                } 

                // process image
                ProcessInputFrame();

                ++iFrameNumber;
            });
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

    void SmartVideoProcessor::ObjectTracking(const ClipEntry& clipEntry) {
        // TODO
        if(true/*!Config.UseCachedForObjectDetection*/) {
            cvflann::Logger::setLevel(cvflann::FLANN_LOG_INFO); // FIXME: remove later

            string folder = Config.GetClipFolder(clipEntry);
            // iterate over all files:
            iFrameNumber = 0;
            //iFrameNumber = clipEntry.StartFrame;
            
            int curCluster = 0;
            vector<ObjectProfile> prevObject, curObject;

            for_each(clipEntry.Filenames.begin() + iFrameNumber, clipEntry.Filenames.end(), [&](const string& fname) {
                // read image file
                string imgpath = folder + "/" + fname;
                frameName = fname;
                frame = imread(imgpath);
                if(!frame.data)
                {
                    // error in opening an image file
                    cerr << "Unable to open image frame: " << imgpath << endl;
                    exit(EXIT_FAILURE);
                } 

                // read foreground mask
                string fgpath = Config.GetForegroundFolder() + "/" + frameName + "." + Config.CachedImageType;
                Mat fgmask = imread(fgpath, CV_LOAD_IMAGE_GRAYSCALE);
                if(!frame.data)
                {
                    // error in opening an image file
                    cerr << "Unable to open foreground mask: " << fgpath << endl;
                    exit(EXIT_FAILURE);
                } 

                // erode and dilate to get rid of noises?
                const int ErosionSize = 2;
                Mat erosionKernel = getStructuringElement(MORPH_ELLIPSE,
                                                          Size(2*ErosionSize+1,2*ErosionSize+1),
                                                          Point(ErosionSize,ErosionSize));
                erode(fgmask, fgmask, erosionKernel);
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
                const int cthreshold = 9; //25;

                vector<Agglomerative::Point2D> pix;
                Mat nzPixels;
                findNonZero(fgmask, nzPixels);
                for(int i=0; i<nzPixels.size().height; i++)
                    pix.push_back(Agglomerative::Point2D(nzPixels.at<Point>(i).y,nzPixels.at<Point>(i).x));
                Agglomerative::AgglomerativeClustering agc(pix);
                vector<Agglomerative::Result> agcResult = agc.cluster(dthreshold,cthreshold);

                Mat3f clmask = Mat3f(fgmask.size().height, fgmask.size().width, Vec3f(0.0,0.0,0.0));
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
                if(prevObject.size()) { // only do matching if previous objects are present
                    Matcher::ClusterMatcher cm(Matcher::obj2cinfo(prevObject), Matcher::obj2cinfo(curObject));
                    vector<pair<int,int>> matching = cm.solve();
                    // associate related pairs
                    for(auto rel: matching) {
                        int pv = rel.first;
                        int cv = rel.second;
                        adj[pv].push_back(cv);
                    }
                }
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

                // draw bounding boxes
                for(auto co: curObject) {
                    ColorProfile cp = co.avgColor();
                    rectangle(clmask, Point(co.y1,co.x1), Point(co.y2,co.x2), Scalar(cp.r,cp.g,cp.b));
                }

                prevObject = curObject;
                /*for(auto co: curObject) {
                    ColorProfile cp = co.avgColor();
                    cerr << "(" << co.x1 << "," << co.y1 << ") (" << co.x2 << "," << co.y2 << ") "
                         << cp.r*255 << " " << cp.g*255 << " " << cp.b*255 << endl;
                }*/

                // cerr << "ncluster = curObject.size() = " << curObject.size() << endl;
                
                if (Config.DisplayFrames)
                {
                    imshow("Frame", frame);
                    //imshow("Foreground", fgmask);
                    imshow("Foreground", clmask);
                    waitKey(10);        // TODO: Add a way to better control FPS
                }

                ++iFrameNumber;
            });
        }
    }

    void SmartVideoProcessor::UpdateDisplay()
    {
        progressBar.UpdateProgress(iFrameNumber);


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
            imshow("Frame", frame);
            imshow("Foreground", foregroundMask);

            waitKey(10);        // TODO: Add a way to better control FPS
        }

        // Dump the foreground information
        std::string outfile = Config.GetForegroundFolder() + "/" + frameName + "." + Config.CachedImageType;  // save as CachedImageType
        try {
            MkDir(Config.GetForegroundFolder());        // make sure that folder exists
            bool saved = imwrite(outfile, foregroundMask);
            if (!saved) {
                cerr << "Unable to save " << outfile << endl;
            }
        } catch (runtime_error& ex) {
            cerr << "Exception dumping foreground image in ." << Config.CachedImageType << " format: " << ex.what() << endl;
            exit(0);
        }

    }

    Job SmartVideoProcessor::GetNextIOJob()
    {
        return nullptr;
    }
}
