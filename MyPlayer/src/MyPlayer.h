#include "Util.h"
#include "JSonUtil.h"
#include <opencv2/highgui/highgui.hpp>
#include <vector>
#include <memory>

namespace mp{
	struct ClipEntry
    {
        std::string Name;
        int StartFrame;
        std::string BaseFolder;
        std::string ClipFile;
        std::string WeightFile;
		std::string SequenceFile;
        std::vector<std::string> Filenames;

        size_t GetFrameCount() const { return Filenames.size(); }
    };

	struct PlayerConfig
    {
        std::string CfgFolder;
        std::string CfgFile;

        std::string DataFolder;
        std::string ClipinfoDir;
        std::string ClipListFile;

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

		/// Get the path to the file containing all frame sequence.
        std::string GetSequencePath(const ClipEntry& clipEntry) const
        {
            return CfgFolder + "/" + ClipinfoDir + "/" + clipEntry.SequenceFile;
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

        /// Read all config files
        bool InitializeConfig();
    };

	struct Player{
		const PlayerConfig Config;

		std::vector<std::string> frameName;
		int frameNumber;
		int startFrameNumber;
		int nowFrameNumber;
		cv::Mat nowFrame;
		int waitKeyNumber;

		std::string sequencePath;
		std::vector<int> s;
		int sequenceNumber;
		int nowSequenceNumber;

		std::string weightPath;
		std::vector<double> w;
		int weightW;
		int weightH;
		IplImage *imgWeight;
		IplImage *imgWeightShow;

		Player(PlayerConfig cfg) :
            Config(cfg)
        {
            //ioWorker.SetTaskQueue(std::bind(&SmartVideoProcessor::GetNextIOJob, this));
        }


		void setNowFrameNumber(int v);
		void setWaitKeyNumber(int v);
		void initPlayer(const ClipEntry& clipEntry);
		void initName(const ClipEntry& clipEntry);
		void initWeight();
		void initSequence();
		void showFrame(int index);
		void startPlaySequence();
		void startPlayFrame();
		void nextSequence();
		void nextFrame();
		void loop();
		void endPlayer();
	};

	
	
}