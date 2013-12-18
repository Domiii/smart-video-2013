#ifndef SMARTVIDEOUTIL_H
#define SMARTVIDEOUTIL_H

#include "Util.h"
#include "ConsoleUtil.h"
#include "JSonUtil.h"
#include "Workers.h"

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>

#include <memory>

#include <queue>


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
        int MaxIOQueueSize;

        std::string CfgFolder;
        std::string CfgFile;

        std::string DataFolder;
        std::string ClipinfoDir;
        std::string ClipListFile;
        std::string ForegroundDir;

        double LearningRate;

        std::vector<ClipEntry> ClipEntries;

        /// Amount of frames in this clip
        int GetFrameCount() const { return ClipEntries.size(); }

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


    /// Data and meta-data of a frame, to be stored in frame buffer.
    struct FrameInfo
    {
        std::string FrameName;
        /// Frame index
        Util::JobIndex FrameIndex;
        /// Frame data
        cv::Mat Frame;
        /// Binary image, with only foreground set to 1
        cv::Mat FrameForegroundMask;

        FrameInfo(Util::JobIndex frameIndex) : FrameIndex(frameIndex) {}
    };


    /// The class that does the "SmartVideo" processing.
    struct SmartVideoProcessor
    {
        const SmartVideoConfig Config;

        const ClipEntry * clipEntry;                        // current clip
        /// Stores frames read from disk
        Util::ThreadSafeQueue<FrameInfo> frameInBuffer;
        /// Stores frames to be written back to disk
        Util::ThreadSafeQueue<FrameInfo> frameOutBuffer;
        std::unique_ptr<cv::BackgroundSubtractor> pMOG;     // MOG Background subtractor
        std::vector<float> frameWeights;                    // weight of every frame

        /// WorkerPool for multi-threaded I/O
        Util::WorkerPool ioPool;
        
        /// Progress bar used for showing progress in Console.
        Util::ConsoleProgressBar progressBar;


        SmartVideoProcessor(SmartVideoConfig cfg) :
            Config(cfg),
            clipEntry(nullptr),
            frameInBuffer(cfg.MaxIOQueueSize),
            frameOutBuffer(cfg.MaxIOQueueSize),
            progressBar(cfg.ProgressBarLen)
        {
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
        void ProcessFrame(Util::JobIndex iFrame);

        /// Computes the weight of a uchar binary image.
        float ComputeFrameWeight(FrameInfo& info);

        /// Draw progress. TODO: Trigger event instead, and let user draw.
        void UpdateDisplay(FrameInfo& info);

        /// Release all resources
        void Cleanup();

        /// Task queue for file-reader thread.
        bool ReadNextInputFrame(Util::JobIndex iFrame);

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