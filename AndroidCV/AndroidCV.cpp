// AndroidCV.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"	
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>

using namespace cv;
using namespace std;

bool frameSt = false;
int push_count = 0;
int notFoundNum = 0;
bool notFoundTrigger = false;

enum Direction{
	FORWARD, BACKWARD, UNKNOWN
};

struct baseballBlob {
	
	int frameNum;
	int frameStart;
	bool isEndFrame;
	Mat baseballImgColor;
	Mat baseballImgGray;
	float x_pos;
	float y_pos;
	Direction direct;

};



bool findBall(Mat thresh, Mat frame, Mat grayFrame, int frameCount, baseballBlob &newBBblob) {

	
	SimpleBlobDetector::Params params;
	params.minThreshold = 10;
	params.maxThreshold = 80;
	params.filterByColor = true;
	params.blobColor = 255;
	params.filterByArea = true;
	params.minArea = 50.0f;
	params.filterByCircularity = true;
	params.minCircularity = 0.4f;
	params.filterByConvexity = false;
	params.filterByInertia = false;


	Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create(params);
	Mat im_w_keypoints;
	std::vector<KeyPoint> keypoints;
	detector->detect(thresh, keypoints);
	float xValLeft;
	float yValLeft;
	float xValRight;
	float yValRight;
	Direction dir;

	if (keypoints.size() >= 2) {

		if (keypoints[0].pt.x > keypoints[1].pt.x) {
			dir = FORWARD;
		}
		else if (keypoints[0].pt.x < keypoints[1].pt.x) {
			dir = BACKWARD;
		}
		else {
			dir = UNKNOWN;
		}

		if (keypoints[0].pt.x - 75 < 1) {
			xValLeft = 1;
		}
		else {
			xValLeft = keypoints[0].pt.x - 75;
		}

		if (keypoints[0].pt.x + 75 > 1279) {
			xValRight = 1279;
		}
		else {
			xValRight = keypoints[0].pt.x + 75;
		}


		if (keypoints[0].pt.y - 30 < 1) {
			yValLeft = 1;
		}
		else {
			yValLeft = keypoints[0].pt.y - 30;
		}

		if (keypoints[0].pt.y + 30 > 719) {
			yValRight = 719;
		}
		else {
			yValRight = keypoints[0].pt.y + 30;
		}

		Rect myRect(Point(xValLeft, yValLeft), Point(xValRight, yValRight));
		Mat image_roi = frame(myRect);
		Mat grayimage_roi = grayFrame(myRect);

		newBBblob.baseballImgColor = image_roi;
		newBBblob.baseballImgGray = grayimage_roi;
		

		newBBblob.x_pos = xValLeft;
		newBBblob.y_pos = yValLeft;
		newBBblob.frameNum = frameCount;
		if (frameSt == false) {
			newBBblob.frameStart = true;
			frameSt = true;
		}
		else {
			newBBblob.frameStart = false;
		}
		newBBblob.direct = dir;
		
		
		return true;
	}

	return false;
}


int *algorithm(baseballBlob blobs[50]){

	int frameCount = 0;
		
	VideoCapture cap("C:\\Users\\HitTraxPC\\Desktop\\PythonTests\\testVids\\Works\\TEST_VID_20171017_161229.mp4");
	
		
	if (cap.isOpened() == false)
	{
		cout << "Cannot open the video file" << endl;
		cin.get();
		return 0;
	}

	while (true)
	{
		Mat frameOne;
		Mat frameTwo;
		Mat grayImg1;
		Mat grayImg2;
		Mat diffImg;
		Mat threshImg;
		
		

		bool bSuccess = cap.read(frameOne);

		if (bSuccess == false){
			cout << "Found the end of the video" << endl;
			break;
		}

		frameCount = frameCount + 1;

		//cout << "Frame Count: " << frameCount << endl;

		bSuccess = cap.read(frameTwo);

		if (bSuccess == false)
		{
			cout << "Found the end of the video" << endl;
			break;
		}

		cvtColor(frameOne, grayImg1, COLOR_BGR2GRAY);
		cvtColor(frameTwo, grayImg2, COLOR_BGR2GRAY);

		absdiff(grayImg1, grayImg2, diffImg);

		threshold(diffImg, threshImg, 50, 255, THRESH_BINARY);

		if (findBall(threshImg, frameTwo, grayImg2, frameCount, blobs[push_count])) {

			
			notFoundNum = 0;
			if (notFoundTrigger == false) {
				notFoundTrigger = true;
			}
			//logging for debugging

			/*cout << "Blob Information" << endl;
			cout << "~~~~~~~~~~~~~~~~" << endl;
			cout << "Point of top left of BB: X:" << blobs[push_count].x_pos << " Y:" << blobs[push_count].y_pos << endl;
			cout << "Frame Number: " << blobs[push_count].frameNum << endl;
			if (blobs[push_count].frameStart == true) {
				cout << "This is the first frame with a baseball in it" << endl;
			}
			else {
				cout << "This is not the first frame with a baseball in it" << endl;
			}
			if (blobs[push_count].direct == 0) {
				cout << "Direction: forward" << endl;
			}
			else {
				cout << "Direction: backward" << endl;
			}

			cout << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl << endl;

			imshow("Image ROI - color", blobs[push_count].baseballImgColor);
			imshow("Image ROI - gray", blobs[push_count].baseballImgGray);
			imshow("Frame", frameTwo);
			

			waitKey(0);*/
			push_count++;
		}
		else if(notFoundTrigger) {
			notFoundNum++;
			//cout << "Not Found Num: " << notFoundNum << endl;
			if (notFoundNum > 6) {
				break;
			}
			
		}
	}
	return 0;
}

int main(int argc, char* argv[])
{

	baseballBlob blobs[50];

	auto start = chrono::high_resolution_clock::now();
	algorithm(blobs);
	auto end = chrono::high_resolution_clock::now();

	//ALL THE DATA IS IN BLOBS[BLOB_NUM]
	//THE NUMBER OF BLOBS IS PUSH_COUNT - 1

	cin.get();
	return 0;
}

