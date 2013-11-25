#include "SmartVideo.h"

using namespace std;
using namespace SmartVideo;


SmartVideoConfig Config;
std::unique_ptr<SmartVideoProcessor> Processor;

int main(int argc, char* argv[])
{
    // setup config
    Config.DisplayResults = false;
    Config.ProgressBarLen = 40;
    Config.CfgFolder = "..";
    Config.CfgFile = "config.json";

    if (!Config.InitializeConfig() || Config.ClipEntries.size() == 0)
    {
        cerr << "ERROR: Invalid clip list file - " << Config.ClipListFile;
        exit(-1);
    }

    
    // create new processor
    Processor = std::unique_ptr<SmartVideoProcessor>(new SmartVideoProcessor(Config));

    for (auto clip : Config.ClipEntries)
    {
        // process image sequence
        Processor->ProcessImages(clip);
    }

	return EXIT_SUCCESS;
}