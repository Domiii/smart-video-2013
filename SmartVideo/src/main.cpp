#include "SmartVideo.h"

#include <chrono>

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
    //Config.NReadThreads = 8;
    Config.NReadThreads = 1;

    if (!Config.InitializeConfig() || Config.ClipEntries.size() == 0)
    {
        cerr << "ERROR: Invalid config or clip list file - " << Config.ClipListFile;
        exit(-1);
    }

    
    // create new processor
    Processor = std::unique_ptr<SmartVideoProcessor>(new SmartVideoProcessor(Config));


    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    for (auto clip : Config.ClipEntries)
    {
        // process image sequence
        Processor->ProcessClip(clip);
    }

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now - start ).count();
    cout << "Processing (with " << Config.NReadThreads << " read threads) took: " << millis/1000.f << " s." << endl << endl;

    cout << "Press ENTER to exit." << endl; cin.get();

	return EXIT_SUCCESS;
}