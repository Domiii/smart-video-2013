#include "Util.h"
#include "JSonUtil.h"
#include <opencv2/highgui/highgui.hpp>
#include <vector>
#include <memory>

#include "SmartVideo.h"

namespace mp{
	struct PlayerConfig : public SmartVideo::SmartVideoConfig
    {
        virtual bool InitializeConfig();
    };


	struct Player{
		const PlayerConfig Config;

        SmartVideo::ClipEntry* clipEntry;
		std::vector<std::string> clipMaskFileNames;
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
        }


		void setNowFrameNumber(int v);
		void setWaitKeyNumber(int v);
		void initPlayer(SmartVideo::ClipEntry& clipEntry);
		void initName(SmartVideo::ClipEntry& clipEntry);
		void initWeight();
		void initSequence();
		void showFrame(int index,int diff);
		void startPlaySequence();
		void startPlayFrame();
		void nextSequence();
		void nextFrame();
		cv::Mat imgProcessing(int index, int diff);
		void loop();
		void endPlayer();
	};

	
	
}