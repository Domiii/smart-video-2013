#include "SmartVideo.h"

using namespace std;
using namespace SmartVideo;


SmartVideoConfig Config;
std::unique_ptr<SmartVideoProcessor> Processor;

class X
{
    X(const X&);
    X operator=(const X&);

public:
    X() {}
};

int main(int argc, char* argv[])
{
    // setup config
    Config.CfgFolder = "..";
    Config.CfgFile = "config.json";

    // some processor-specific things
    Config.ProgressBarLen = 50;
    Config.MaxIOQueueSize = 50;

    if (!Config.InitializeConfig() || Config.ClipEntries.size() == 0)
    {
        cerr << "ERROR: Invalid config or clip list file - " << Config.ClipListFile;
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