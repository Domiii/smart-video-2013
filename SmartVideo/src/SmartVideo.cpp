#include "SmartVideo.h"

#include <iomanip>

#include "FileUtil.h"
#include <functional>

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

        // allocate frame weights
        frameWeights.resize(clipEntry->GetFrameCount());
        if (Config.DisplayFrames)
        {
            // create GUI windows (for debugging purposes)
            namedWindow("Frame");
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
        uint32 nForegroundPixels  = 0;

        // count all foreground pixels
        int cols = frameInfo.Frame.cols, rows = frameInfo.Frame.rows;
        if (frameInfo.Frame.isContinuous())
        {
            cols *= rows;
            rows = 1;
        }
        for(int i = 0; i < rows; i++)
        {
            auto Mi = frameInfo.Frame.ptr<uchar>(i);
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

        // finalize the process
        FinishProcessing();
    }


    void SmartVideoProcessor::ProcessNextFrame()
    {
        // get next frame from queue
        FrameInfo info = frameInBuffer.Pop();
        
        // main stuffs
        BackgroundSubtraction(info);
        ObjectDetection(info);
        ObjectTracking(info);

        // compute and set weight
        SetWeight(iNextProcessFrame, ComputeFrameWeight(info));

        // draw progress
        UpdateDisplay(info);
    }


    void SmartVideoProcessor::BackgroundSubtraction(FrameInfo& info) {
        if(!Config.UseCachedForForeground) {
            //update the background model
            pMOG->operator()(info.Frame, info.FrameForegroundMask, Config.LearningRate);
        }
    }


    void SmartVideoProcessor::ObjectDetection(FrameInfo& info) {
        if(false/*!Config.UseCachedForObjectDetection*/) {

            // read foreground mask
            string fgpath = Config.GetForegroundFolder() + "/" + info.FrameName + "." + Config.CachedImageType;
            cv::Mat fgmask = imread(fgpath);
            if(!fgmask.data)
            {
                // error in opening an image file
                cerr << "Unable to open foreground mask: " << fgpath << endl;
                exit(EXIT_FAILURE);
            }

            // TODO
        }
    }


    void SmartVideoProcessor::ObjectTracking(FrameInfo& info) {
        // TODO
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
            //imshow("Foreground", info.FrameFoegroundMask);

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
        }

        ioPool.Stop();
        ioPool.Join();
    }
}
