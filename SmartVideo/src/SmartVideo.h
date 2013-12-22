#ifndef SMARTVIDEOUTIL_H
#define SMARTVIDEOUTIL_H

#include "Util.h"
#include "ConsoleUtil.h"
#include "JSonUtil.h"
#include "Workers.h"
#include "agglomerative.h"
#include "matcher.h"

#include "opencv2/ml/ml.hpp"
#include "opencv2/flann/flann.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/opencv.hpp>

#include <memory>

#include <queue>


namespace SmartVideo
{
    enum class ClipType
    {
        ImageSequence,
        Video
    };

    /// Represents a clip (video file, or sequence of images)
    struct ClipEntry
    {
        ClipType Type;
        std::string Name;
        int StartFrame;
        std::string BaseFolder;
        std::string WeightFile;

        /// If Type == Video, this is the video file name, else it is the list of images in the sequence
        std::string ClipFile;

        /// If Type == ImageSequence, contains all image file names
        std::vector<std::string> Filenames;

        /// If Type == Video, allows access to the underlying video file
        cv::VideoCapture Video;

        /// TODO: Make const
        size_t GetFrameCount() 
        {
            if (Type == ClipType::Video)
            {
                // TODO: Verify this approach
                return Video.get(CV_CAP_PROP_FRAME_COUNT);
            }
            else
            {
                return Filenames.size(); 
            }
        }
    };

    /// Configuration for the SmartVideo processor.
    struct SmartVideoConfig
    {
        json_value * cfgRoot;

        // SmartVideoProcessor-specific configuration
        bool DisplayFrames;
        int ProgressBarLen;
        int MaxIOQueueSize;
        int NReadThreads;

        // Data configuration
        std::string CfgFolder;
        std::string CfgFile;

        std::string DataFolder;
        std::string ClipinfoDir;
        std::string ClipListFile;

        /// Foreground metadata
        std::string ForegroundDir, ForegroundFrameFile;

        double LearningRate;
        std::string CachedImageType;
        bool UseCachedForForeground;

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

        /// Get the video file name.
        std::string GetVideoFile(const ClipEntry& clipEntry) const
        {
            return CfgFolder + "/" + DataFolder + "/" + clipEntry.BaseFolder + "/" + clipEntry.ClipFile;
        }

        /// Get the folder containing the cahced foreground datas
        std::string GetForegroundFolder(const ClipEntry& clipEntry) const
        {
            return CfgFolder + "/" + DataFolder + "/" + clipEntry.BaseFolder + "/" + ForegroundDir;
        }

        /// Get the folder containing the cahced foreground datas
        std::string GetForegroundFrameFile(const ClipEntry& clipEntry) const
        {
            return GetForegroundFolder(clipEntry) + "/" + ForegroundFrameFile;
        }

        /// Read all config files
        virtual bool InitializeConfig();
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
        /// Object frame
        cv::Mat FrameObjectDetection;

        FrameInfo(Util::JobIndex frameIndex) : FrameIndex(frameIndex) {}

        bool operator<(const FrameInfo& other) const
        {
            return FrameIndex < other.FrameIndex;
        }
    };


    /// The class that does the "SmartVideo" processing.
    struct SmartVideoProcessor
    {
        const SmartVideoConfig Config;

        ClipEntry * clipEntry;                        // current clip
        /// Stores frames read from disk
        Util::ThreadSafeQueue<FrameInfo> frameInBuffer;
        /// Stores frames to be written back to disk
        Util::ThreadSafeQueue<FrameInfo> frameOutBuffer;

        /// Index of next frame to be processed
        Util::JobIndex iNextProcessFrame;
        std::unique_ptr<cv::BackgroundSubtractor> pMOG;     // MOG Background subtractor
        std::vector<float> frameWeights;                    // weight of every frame

        /// WorkerPool for multi-threaded I/O
        Util::WorkerPool ioPool;
        
        /// Progress bar used for showing progress in Console.
        Util::ConsoleProgressBar progressBar;

        /// Object vector information needed for ObjectTracking
        vector<ObjectProfile> prevObject, curObject;

        SmartVideoProcessor(SmartVideoConfig cfg) :
            Config(cfg),
            clipEntry(nullptr),
            frameInBuffer(cfg.MaxIOQueueSize, true, std::bind(&SmartVideoProcessor::IsNextInputFrame, this, std::placeholders::_1)),
            frameOutBuffer(cfg.MaxIOQueueSize, false),
            progressBar(cfg.ProgressBarLen)
        {
        }

        virtual ~SmartVideoProcessor()
        {
            Cleanup();
        }

    private:
        /// Initialize this guy.
        void InitProcessing(ClipEntry * clipEntry);

        /// Finalize the process.
        void FinishProcessing();

        /// Processes the frame that was last read from the input stream.
        void ProcessNextFrame();

        /// Computes the weight of a uchar binary image.
        float ComputeFrameWeight(FrameInfo& info);

        /// Draw progress. TODO: Trigger event instead, and let user draw.
        void UpdateDisplay(FrameInfo& info);

        /// Release all resources
        void Cleanup();

        /// Task queue for file-reader thread.
        bool ReadNextInputFrame(Util::JobIndex iFrame);

        // Sub-Procedures for each Part
        void BackgroundSubtraction(FrameInfo& info);
        void InitObjectTracking();
        void ObjectTracking(FrameInfo& info);

        // Helper Proccesses
        bool ClusterWithK(const cv::Mat& fgmask, int maxCluster, cv::Mat3f& clmask);

    public:
        bool IsNextInputFrame(const FrameInfo& info) const 
        { 
            return info.FrameIndex == iNextProcessFrame; 
        }

        /// Set the weight of the given frame.
        void SetWeight(int iFrame, float weight)
        {
            frameWeights[iFrame] = weight;
        }

        /// Process a stream that is represented by a sequence of images.
        void ProcessClip(ClipEntry& clipEntry);
    };
}

#endif // SMARTVIDEOUTIL_H