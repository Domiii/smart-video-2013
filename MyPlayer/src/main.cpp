#include <cstdio>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include "MyPlayer.h"
using namespace std;
using namespace cv;
using namespace mp;

PlayerConfig Config;
std::unique_ptr<Player> p;

int main(int argc, char* argv[])
{

	Config.CfgFolder = "..";
    Config.CfgFile = "config.json";

	if (!Config.InitializeConfig() || Config.ClipEntries.size() == 0){
        cerr << "ERROR: Invalid config or clip list file - " << Config.ClipListFile;
        cerr << "Press ENTER to exit." << endl; cin.get();
        exit(EXIT_FAILURE);
    }


	p = std::unique_ptr<Player>(new Player(Config));
	for (auto clip : Config.ClipEntries){
        p->initPlayer(clip);
    }

	return 0;
}