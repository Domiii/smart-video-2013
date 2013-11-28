#include "SmartVideo.h"

#include <iomanip>

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
		DisplayResults = JSonGetProperty(cfgRoot, "displayResults")->int_value;
		LearningRate = JSonGetProperty(cfgRoot, "learningRate")->float_value;

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
        
        if (Config.DisplayResults)
        {
            // create GUI windows (for debugging purposes)
            namedWindow("Frame");
            namedWindow("Foreground");
        }

        cout << "Processing " << clipEntry->Name << "..." << endl;
    }


    /// Processes the current frame.
    void SmartVideoProcessor::ProcessInputFrame()
    {
        //update the background model
        pMOG->operator()(frame, foregroundMask, Config.LearningRate);

        // compute and set weight
        SetWeight(iFrameNumber, ComputeFrameWeight(foregroundMask));
        
        // draw progress
        DrawProgress();
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
    void  SmartVideoProcessor::ProcessVideo(const ClipEntry& clipEntry) {
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
        InitProcessing(&clipEntry);

        string folder = Config.GetClipFolder(clipEntry);

        // iterate over all files:
        //iFrameNumber = clipEntry.StartFrame;
        iFrameNumber = 0;
        for_each(clipEntry.Filenames.begin() + iFrameNumber, clipEntry.Filenames.end(), [&](const string& fname) {
            // read image file
            string fpath = folder + "/" + fname;
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

        // finalize the process
        FinishProcessing();
    }

    void SmartVideoProcessor::DrawProgress()
    {
        // draw progress to console
        auto nTotalFrames = clipEntry->GetFrameCount() - 1;
        string frameNumberString = std::to_string(iFrameNumber) + " / " + std::to_string(nTotalFrames);
        float progress = static_cast<float>(iFrameNumber) / nTotalFrames;
        float lastProgress = static_cast<float>(iFrameNumber-1) / nTotalFrames;
        int progressLen = static_cast<int>(Config.ProgressBarLen * progress + .5f);
        int lastProgressLen = static_cast<int>(Config.ProgressBarLen * lastProgress + .5f);

        // progress bar moved, or we processed the last frame
        if (progressLen != lastProgressLen || iFrameNumber == nTotalFrames)
        {
            std::string progressString("|");
            progressString.reserve(Config.ProgressBarLen+2);
            for (int i = 1; i < Config.ProgressBarLen; ++i)
            {
                if (i <= progressLen)
                    progressString += 'i';
                else
                    progressString += ' ';
            }
            progressString += '|';
            
            cout << '\r' << progressString << setw(15) << frameNumberString << " (" << setprecision(3) << (100 * progress) << "%)   ";
            cout.flush();
        }
        
        if (Config.DisplayResults)
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

            waitKey(10);        // TODO: Add a way to better control playback FPS
        }
    }

    Job SmartVideoProcessor::GetNextIOJob()
    {
        return nullptr;
    }
}
