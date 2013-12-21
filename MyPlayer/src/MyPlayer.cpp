#include "MyPlayer.h"
#include "FileUtil.h"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>
using namespace std;
using namespace cv;
using namespace Util;

void changeBar(int value, void* ptr){
	mp::Player *p = (mp::Player*)ptr;
	int pos = getTrackbarPos("Frame","Weight");
	p->setNowFrameNumber(pos);
	p->showFrame(pos);
}

void changeSpeed(int value, void* ptr){
	mp::Player *p = (mp::Player*)ptr;
	int pos = getTrackbarPos("Speed","Weight");

	int num = 4;
	for(int i=6; i>pos; i--)
		num *= 2;
	p->setWaitKeyNumber(num);
}

namespace mp{

	bool PlayerConfig::InitializeConfig(){
		
        std::string cfgPath(CfgFolder + "/" + CfgFile);
        json_value * cfgRoot = JSonReadFile(cfgPath);
        if (!cfgRoot) return false;
		
        DataFolder = JSonGetProperty(cfgRoot, "dataDir")->GetStringValue();
        ClipinfoDir = JSonGetProperty(cfgRoot, "clipDir")->GetStringValue();
        ClipListFile = JSonGetProperty(cfgRoot, "clipFile")->GetStringValue();
		
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
			entry.SequenceFile = JSonGetProperty(entryNode, "sequenceFile")->GetStringValue();
            entry.StartFrame = JSonGetProperty(entryNode, "startFrame")->int_value;
            entry.Filenames = ReadLines(GetFrameFilePath(entry));
        }
		
        return true;
    }
	
	void Player::initPlayer(const ClipEntry& clipEntry){

		cout << "display " << clipEntry.Name << " ..." << endl;
		cout << "frameNumber: " << clipEntry.GetFrameCount() << endl;
		cout << "weightPath: " << Config.GetWeightsPath(clipEntry) << endl;
		cout << "sequencePath: " << Config.GetSequencePath(clipEntry) << endl;
		
		initName(clipEntry);
		

		frameNumber = clipEntry.GetFrameCount();
		startFrameNumber = 0;
		nowFrameNumber = 0;
		waitKeyNumber = 32;

		sequencePath = Config.GetSequencePath(clipEntry);
		nowSequenceNumber = 0;

		weightPath = Config.GetWeightsPath(clipEntry);
		weightW = 1000;
		weightH = 100;

		initWeight();
		initSequence();

		namedWindow("Display", CV_WINDOW_AUTOSIZE);
		namedWindow("Weight", CV_WINDOW_AUTOSIZE);
		createTrackbar("Frame", "Weight", 0, frameNumber-1, changeBar, (void*)this);
		setTrackbarPos("Frame", "Weight", nowFrameNumber);
		createTrackbar("Speed", "Weight", 0, 6, changeSpeed, (void*)this);
		setTrackbarPos("Speed", "Weight", 3);

		showFrame(s[0]);

		loop();
		
	}

	void Player::initName(const ClipEntry& clipEntry){
		string folder = Config.GetClipFolder(clipEntry);
        // iterate over all files:
        int iFrameNumber = 0;
        for_each(clipEntry.Filenames.begin() + iFrameNumber, clipEntry.Filenames.end(), [&](const string& fname) {
            // read image file
            string fpath = folder + "/" + fname;
			frameName.push_back(fpath);
            iFrameNumber++;
        });
		
	}

	void Player::initSequence(){
		FILE *fp;
		cout << "sequencePath: "  <<sequencePath.c_str() << endl;
		fp = fopen(sequencePath.c_str(),"r");
		int tmps;
		while(fscanf(fp,"%d",&tmps)!=EOF){
			s.push_back(tmps);
		}
		sequenceNumber = (int)s.size();
		cout << "sequenceNumber: " << sequenceNumber << endl;
	}

	void Player::initWeight(){

		FILE *fp;
		fp = fopen(weightPath.c_str(),"r");

		for(int i=0; i<frameNumber; i++){
			double tmpw;
			fscanf(fp, "%lf", &tmpw);
			w.push_back(tmpw);
		}
		
		CvPoint FromPoint,ToPoint;
		CvScalar Color = CV_RGB(100,100,255);
		int Thickness = 1;
		int Shift = 0;

		CvSize ImageSize = cvSize(weightW,weightH);
		imgWeight = cvCreateImage(ImageSize,IPL_DEPTH_8U,3);

		if(frameNumber<1000){
			for(int i=0; i<frameNumber; i++){
				FromPoint = cvPoint(i,weightH);
				ToPoint = cvPoint(i,weightH-(int)(w[i]*100));
				cvLine(imgWeight,FromPoint,ToPoint,Color,Thickness,4,Shift);
			}
		}
		else{
			for(int i=0; i<1000; i++){
				double ratio = (double)i/1000*(double)frameNumber;
				int index = (int)ratio;
				FromPoint = cvPoint(i,weightH);
				ToPoint = cvPoint(i,weightH-(int)(w[index]*100));
				cvLine(imgWeight,FromPoint,ToPoint,Color,Thickness,4,Shift);
			}
		}
		
	
	}

	void Player::setNowFrameNumber(int v){
		nowFrameNumber = v;
		nowSequenceNumber = 0;
		while(nowSequenceNumber<(int)s.size() && s[nowSequenceNumber]<v)
			nowSequenceNumber++;
	}

	void Player::setWaitKeyNumber(int v){
		waitKeyNumber = v;
	}

	void Player::startPlaySequence(){
		while(nowSequenceNumber<sequenceNumber-1){
			nextSequence();
			int key = waitKey(waitKeyNumber);
			if(key=='d')
				break;
			else if(key=='[')
				waitKeyNumber *= 2;
			else if(key==']')
				waitKeyNumber /= 2;
			
		}
		return;
	}

	void Player::startPlayFrame(){
		while(nowFrameNumber<frameNumber-1){
			nextFrame();
			int key = waitKey(waitKeyNumber);
			if(key=='d')
				break;
			else if(key=='[')
				waitKeyNumber *= 2;
			else if(key==']')
				waitKeyNumber /= 2;
			
		}
		return;
	}

	void Player::showFrame(int index){
		nowFrame.release();
		nowFrameNumber = index;

		string framePath;
		framePath = frameName[index];
		cout << framePath << endl;
		nowFrame = imread(framePath, CV_LOAD_IMAGE_COLOR);
		imshow("Display", nowFrame);
		
		CvPoint FromPoint,ToPoint;
		CvScalar Color = CV_RGB(255,0,0);
		int Thickness = 2;
		int Shift = 0;
		int ii;
		if(frameNumber<1000){
			ii = index;
		}
		else{
			double ratio = (double)index/(double)frameNumber*1000;
			ii = (int)ratio;
		}

		FromPoint = cvPoint(ii, weightH);
		ToPoint = cvPoint(ii, 0);

		imgWeightShow = cvCloneImage(imgWeight);
		cvLine(imgWeightShow,FromPoint,ToPoint,Color,Thickness,4,Shift);

		cvShowImage("Weight",imgWeightShow);
		
		return;
	}

	void Player::nextSequence(){
		if(nowSequenceNumber<sequenceNumber-1){
			nowSequenceNumber++;
			nowFrameNumber = s[nowSequenceNumber];
			setTrackbarPos("Frame", "Weight", nowFrameNumber);
		}
		else{
			cout << "the last frame" << endl;
		}
	}

	void Player::nextFrame(){
		if(nowFrameNumber<frameNumber-1){
			nowFrameNumber++;
			setTrackbarPos("Frame", "Weight", nowFrameNumber);
		}
		else{
			cout << "the last frame" << endl;
		}
	}

	void Player::loop(){
		int key;
		while((key=waitKey(0))>0){
			if(key==' ')
				nextFrame();
			else if(key=='q')
				endPlayer();
			else if(key=='s')
				startPlaySequence();
			else if(key=='a')
				startPlayFrame();
		}
	}
	
	void Player::endPlayer(){
		destroyWindow("Display");
		destroyWindow("Weight");
		frameName.clear();
		s.clear();
		w.clear();
	}
}