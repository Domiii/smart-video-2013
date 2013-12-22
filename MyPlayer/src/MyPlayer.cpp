#include "MyPlayer.h"
#include "FileUtil.h"
#include <cstdio>
#include <iostream>
#include <sstream>
#include <vector>
#include "FileUtil.h"


using namespace std;
using namespace cv;
using namespace Util;
using namespace SmartVideo;


void changeBar(int value, void* ptr){
	mp::Player *p = (mp::Player*)ptr;
	int pos = getTrackbarPos("Frame","Weight");
	int diff = pos - p->nowFrameNumber;
	p->setNowFrameNumber(pos);
	p->showFrame(pos,diff);
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

	bool PlayerConfig::InitializeConfig()
    {
        return SmartVideoConfig::InitializeConfig();
    }
	
	void Player::initPlayer(ClipEntry& clipEntry)
    {
		this->clipEntry = &clipEntry;
        clipMaskFileNames = ReadLines(Config.GetForegroundFolder(clipEntry));

		frameNumber = clipEntry.GetFrameCount();
		cout << "frameNumber: " << frameNumber << endl;
		startFrameNumber = 0;
		nowFrameNumber = 0;
		waitKeyNumber = 32;
		nowSequenceNumber = 0;

		weightPath = Config.GetWeightsPath(clipEntry);
		weightW = 1000;
		weightH = 200;

		sequencePath = Config.GetSequencePath(clipEntry);
		initWeight();
		initSequence();

		namedWindow("Display", CV_WINDOW_AUTOSIZE);
		namedWindow("Foreground", CV_WINDOW_AUTOSIZE);
		namedWindow("Weight", CV_WINDOW_AUTOSIZE);
		createTrackbar("Frame", "Weight", 0, frameNumber-1, changeBar, (void*)this);
		setTrackbarPos("Frame", "Weight", nowFrameNumber);
		createTrackbar("Speed", "Weight", 0, 6, changeSpeed, (void*)this);
		setTrackbarPos("Speed", "Weight", 3);

		showFrame(s[0],1);

		loop();
		
	}

	void Player::initSequence(){
		FILE *fp;
		fp = fopen(sequencePath.c_str(),"r");
        if (!fp)
        {
            cerr << "ERROR: Could not open file " << sequencePath << endl ;
            cerr << "Press ENTER to exit." << endl; cin.get();
            exit(EXIT_FAILURE);
        }

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
        if (!fp)
        {
            cerr << "ERROR: Could not open file " << weightPath << endl ;
            cerr << "Press ENTER to exit." << endl; cin.get();
            exit(EXIT_FAILURE);
        }

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
				ToPoint = cvPoint(i,weightH-(int)(weightH*w[i]/10000));
				cvLine(imgWeight,FromPoint,ToPoint,Color,Thickness,4,Shift);
			}
		}
		else{
			for(int i=0; i<1000; i++){
				double ratio = (double)i/1000*(double)frameNumber;
				int index = (int)ratio;
				FromPoint = cvPoint(i,weightH);
				ToPoint = cvPoint(i,weightH-(int)(weightH*w[index]/10000));
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

	void Player::showFrame(int index, int diff){
		nowFrame.release();
		nowFrameNumber = index;

		/*
		string framePath;
		framePath = frameName[index];
		cout << framePath << endl;
		nowFrame = imread(framePath, CV_LOAD_IMAGE_COLOR);
		*/
		nowFrame = imgProcessing(index,diff);

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
			//nowFrameNumber = s[nowSequenceNumber];
			setTrackbarPos("Frame", "Weight", s[nowSequenceNumber]);
		}
		else{
			cout << "the last frame" << endl;
		}
	}

	void Player::nextFrame(){
		if(nowFrameNumber<frameNumber-1){
			//nowFrameNumber++;
			setTrackbarPos("Frame", "Weight", nowFrameNumber+1);
		}
		else{
			cout << "the last frame" << endl;
		}
	}

	cv::Mat Player::imgProcessing(int iFrame, int diff){
        // read actual frame
        Mat frame;
        if (clipEntry->Type == ClipType::Video)
        {
			//clipEntry->Video.set(CV_CAP_PROP_POS_MSEC, (double)iFrame*1000/15);
			//clipEntry->Video.set(CV_CAP_PROP_POS_FRAMES, (double)iFrame);
            // read frame from video
            if (!clipEntry->Video.read(frame) || frame.total() == 0)
            {
                cerr << "ERROR: Unable to read next frame (#" << iFrame << ") from video." << endl;
                cerr << "Press ENTER to exit." << endl; cin.get();
                exit(EXIT_FAILURE);
            }
			while(diff>1){
				if (!clipEntry->Video.read(frame) || frame.total() == 0)
				{
	                cerr << "ERROR: Unable to read next frame (#" << iFrame << ") from video." << endl;
					cerr << "Press ENTER to exit." << endl; cin.get();
					exit(EXIT_FAILURE);
				}
				diff--;
			}
        }
        else
        {
            // read frame from image
		    string frameFolder = Config.GetClipFolder(*clipEntry);
            string framePath = frameFolder + "/" + clipEntry->Filenames[iFrame];
            string folder = Config.GetClipFolder(*clipEntry);
            string fname = *(clipEntry->Filenames.begin() + iFrame);
            string fpath = folder + "/" + fname;

            frame = imread(fpath);
			cout << fpath << endl;
            if(!frame.data)
            {
                // error in opening an image file
                cerr << "Unable to open image frame: " << fpath << endl;
                cerr << "Press ENTER to exit." << endl; cin.get();
                exit(EXIT_FAILURE);
            } 
        }


  //      // read mask
		
        auto foregroundFolder = Config.GetForegroundFolder(*clipEntry);
        //auto foregroundPath = foregroundFolder + "/" + clipMaskFileNames[iFrame];
		char tmp[10];
		sprintf(tmp,"%d.bmp",iFrame);
		string tmps(tmp);
		auto foregroundPath = foregroundFolder + "/" + tmps;
		Mat fg = imread(foregroundPath, CV_LOAD_IMAGE_COLOR);
		imshow("Foreground",fg);

		auto maskFolder = Config.GetMaskFolder(*clipEntry);
		sprintf(tmp,"%d.bmp",iFrame);
		tmps.assign(tmp);
		auto maskPath = maskFolder + "/" + tmps;
		Mat mask = imread(maskPath, CV_LOAD_IMAGE_COLOR);

        // substitute mask in frame
		
		for(int i=0; i<frame.rows; i++){
			for(int j=0; j<frame.cols; j++){
				if(mask.at<Vec3b>(i,j)[0]!=0 || mask.at<Vec3b>(i,j)[1]!=0 || mask.at<Vec3b>(i,j)[2]!=0){
					frame.at<Vec3b>(i,j)[0] = mask.at<Vec3b>(i,j)[0];
					frame.at<Vec3b>(i,j)[1] = mask.at<Vec3b>(i,j)[0];
					frame.at<Vec3b>(i,j)[2] = mask.at<Vec3b>(i,j)[0];
				}
			}
		}
		

		return frame;
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
		destroyWindow("Foreground");
		s.clear();
		w.clear();
	}
}