#ifndef SMARTVIDEOUTIL_H
#define SMARTVIDEOUTIL_H

#include "Util.h"
#include "ConsoleUtil.h"
#include "JSonUtil.h"
#include "Workers.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>

#include <memory>


namespace SmartVideo
{
    /// Represents a clip (currently: Sequence of images)
    struct ClipEntry
    {
        std::string Name;
        int StartFrame;
        std::string BaseFolder;
        std::string ClipFile;
        std::string WeightFile;
        std::vector<std::string> Filenames;

        size_t GetFrameCount() const { return Filenames.size(); }
    };

    /// Configuration for the SmartVideo processor.
    struct SmartVideoConfig
    {
        bool DisplayFrames;
        int ProgressBarLen;
        std::string CfgFolder;
        std::string CfgFile;

        std::string DataFolder;
        std::string ClipinfoDir;
        std::string ClipListFile;
        std::string ForegroundDir;

        double LearningRate;
        std::string CachedImageType;
        bool UseCachedForForeground;

        std::vector<ClipEntry> ClipEntries;

        /// Get the path to the file containing all clip filenames.
        std::string GetClipListPath() const
        {
            return CfgFolder + "/" + ClipinfoDir + "/" + ClipListFile;
        }

        /// Get the path to the file containing all frame weights.
        std::string GetWeightsPath(const ClipEntry& clipEntry) const
        {
            return CfgFolder + "/" + ClipinfoDir + "/" + clipEntry.WeightFile;
        }

        /// Get the path to the file containing all frame filenames.
        std::string GetFrameFilePath(const ClipEntry& clipEntry) const
        {
            return CfgFolder + "/" + ClipinfoDir + "/" + clipEntry.ClipFile;
        }

        /// Get the folder containing the files containing the given clip's frames
        std::string GetClipFolder(const ClipEntry& clipEntry) const
        {
            return CfgFolder + "/" + DataFolder + "/" + clipEntry.BaseFolder;
        }

        /// Get the folder containing the cahced foreground datas
        std::string GetForegroundFolder() const
        {
            return CfgFolder + "/" + ClipinfoDir + "/" + ForegroundDir;
        }

        /// Read all config files
        bool InitializeConfig();
    };



    /// The class that does the "SmartVideo" processing.
    struct SmartVideoProcessor
    {
        const SmartVideoConfig Config;

        const ClipEntry * clipEntry;                        // current clip
        int iFrameNumber;                                   // index of current frame in video stream
        std::string frameName;                              // file name of current frame
        cv::Mat frame;                                      // current frame
        cv::Mat foregroundMask;                             // binary image, with only the foreground set to 1
        std::unique_ptr<cv::BackgroundSubtractor> pMOG;     // MOG Background subtractor
        std::vector<float> frameWeights;                    // weight of every frame

        /// Worker to perform all I/O operations
        Util::Worker ioWorker;
        
        /// Progress bar used for showing progress in Console.
        Util::ConsoleProgressBar progressBar;


        SmartVideoProcessor(SmartVideoConfig cfg) :
            Config(cfg),
            progressBar(cfg.ProgressBarLen)
        {
            //ioWorker.SetTaskQueue(std::bind(&SmartVideoProcessor::GetNextIOJob, this));
        }

        virtual ~SmartVideoProcessor()
        {
            Cleanup();
        }

    private:
        /// Initialize this guy.
        void InitProcessing(const ClipEntry * clipEntry);

        /// Finalize the process.
        void FinishProcessing();

        /// Processes the frame that was last read from the input stream.
        void ProcessInputFrame();

        /// Computes the weight of a uchar binary image.
        float ComputeFrameWeight(cv::Mat frame);

        /// Draw progress. TODO: Trigger event instead, and let user draw.
        void UpdateDisplay();

        /// Release all resources
        void Cleanup()
        {
            if (Config.DisplayFrames)
            {
                cvDestroyWindow("Frame");
                cvDestroyWindow("Foreground");
            }
        }

        // Sub-Procedures for each Part
        void BackgroundSubtraction(const ClipEntry& clipEntry);
        void ObjectDetection(const ClipEntry& clipEntry);
        void ObjectTracking(const ClipEntry& clipEntry);

        Util::Job GetNextIOJob();

    public:
        /// Set the weight of the given frame.
        void SetWeight(int iFrame, float weight)
        {
            frameWeights[iFrame] = weight;
        }

        /// Process a video.
        void ProcessVideo(const ClipEntry& clipEntry);

        /// Process a stream that is represented by a sequence of images.
        void ProcessImages(const ClipEntry& clipEntry);
    };
}

#endif // SMARTVIDEOUTIL_H