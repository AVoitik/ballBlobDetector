// AndroidCV.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"	
#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <cmath>

using namespace cv;
using namespace std;

bool frameSt = false;
int push_count = 0;
int notFoundNum = 0;
bool notFoundTrigger = false;
#define PI 3.14159265
float exitVeloAvg = 0;
float exitVeloAvgCounter = 0;

float launchAngleAvg = 0;
float launchAngleAvgNum = 0;

//Uncomment line below for visual output and per-keypoint data
//#define DEBUG

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
	params.minArea = 150;

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
		//Mat image_roi = frame(myRect);
		//Mat grayimage_roi = grayFrame(myRect);

		//newBBblob.baseballImgColor = image_roi;
		//newBBblob.baseballImgGray = grayimage_roi;
		

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

void rotate_90n(cv::Mat const &src, cv::Mat &dst, int angle)
{
	CV_Assert(angle % 90 == 0 && angle <= 360 && angle >= -360);
	if (angle == 270 || angle == -90) {
		// Rotate clockwise 270 degrees
		cv::transpose(src, dst);
		cv::flip(dst, dst, 0);
	}
	else if (angle == 180 || angle == -180) {
		// Rotate clockwise 180 degrees
		cv::flip(src, dst, -1);
	}
	else if (angle == 90 || angle == -270) {
		// Rotate clockwise 90 degrees
		cv::transpose(src, dst);
		cv::flip(dst, dst, 1);
	}
	else if (angle == 360 || angle == 0 || angle == -360) {
		if (src.data != dst.data) {
			src.copyTo(dst);
		}
	}
}


int *algorithm(baseballBlob blobs[50]){

	int frameCount = 0;
		
	VideoCapture cap("C:\\Users\\HitTraxPC\\Desktop\\AndroidCVVids\\test_vid_9.mp4");
	
		
	if (cap.isOpened() == false)
	{
		cout << "Cannot open the video file" << endl;
		cin.get();
		return 0;
	}

	while (true)
	{
		Mat frameOne;
		Mat flippedFrameOne;
		Mat frameTwo;
		Mat flippedFrameTwo;
		Mat grayImg1;
		Mat grayImg2;
		Mat diffImg;
		Mat threshImg;
		
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;

		bool bSuccess = cap.read(flippedFrameOne);
		rotate_90n(flippedFrameOne, frameOne, 180);
		
		if (bSuccess == false){
			cout << "Found the end of the video" << endl;
			break;
		}

		frameCount = frameCount + 1;

		//cout << "Frame Count: " << frameCount << endl;

		bSuccess = cap.read(flippedFrameTwo);
		rotate_90n(flippedFrameTwo, frameTwo, 180);

		if (bSuccess == false)
		{
			cout << "Found the end of the video" << endl;
			break;
		}
		Rect myRect(Point(0, 0), Point(700, 720));
		Mat frameOneROI = frameOne(myRect);
		Mat frameTwoROI = frameTwo(myRect);

		
		cvtColor(frameOneROI, grayImg1, COLOR_BGR2GRAY);
		cvtColor(frameTwoROI, grayImg2, COLOR_BGR2GRAY);

		absdiff(grayImg1, grayImg2, diffImg);

		threshold(diffImg, threshImg, 50, 255, THRESH_BINARY);

		if (findBall(threshImg, frameTwoROI, grayImg2, frameCount, blobs[push_count])) {

			findContours(threshImg, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

			vector<RotatedRect> minRect(contours.size());
			for (int i = 0; i < contours.size(); i++) {
				minRect[i] = minAreaRect(Mat(contours[i]));
			}

			Mat drawing = Mat::zeros(frameTwo.size(), CV_8UC3);
			float prevX = 0.0;
			float prevY = 0.0;
			int goingUpOne = 0;
			int goingUpTwo = 0;

			for (int i = 0; i<contours.size(); i++) {
				float newOne;
				float newTwo;
				Scalar color = Scalar(0,255,255);
				drawContours(frameTwo, contours, i, color, 1, 8, vector<Vec4i>(), 0, Point());
				Point2f rect_points[4];
				minRect[i].points(rect_points);
				float distance = sqrt(pow((rect_points[0].x - rect_points[(0 + 1) % 4].x), 2.0) + pow((rect_points[0].y - rect_points[(0 + 1) % 4].y), 2.0));
				//cout << "Distance " << distance << endl;
				if (distance > 30) {
					goingUpOne = 0;
					goingUpTwo = 2;
				}
				else {
					goingUpOne = 1;
					goingUpTwo = 3;
				}

				for (int j = 0; j < 4; j++) {
					line(frameTwo, rect_points[j], rect_points[(j + 1) % 4], color, 1, 8);
					
					
					
					if ((j == goingUpOne) || (j == goingUpTwo)){
						newOne = rect_points[j].x - rect_points[(j + 1) % 4].x;
						newTwo = rect_points[j].y - rect_points[(j + 1) % 4].y;
						float mResult = newTwo / newOne;
						float launchAngle = atan(mResult) * 180 / PI;
#ifdef DEBUG			
						cout << "Launch Angle: " << launchAngle << endl;
#endif
						
						launchAngleAvg = launchAngleAvg + launchAngle;
						launchAngleAvgNum++;

					}
					
					if (j == 3) {
						if ((prevX == 0) || (prevY == 0)) {
							prevX = rect_points[j].x;
							prevY = rect_points[j].y;
						}
						else {

							float horizDist = rect_points[j].x - prevX;
							float vertDist = rect_points[j].y - prevY;
							float hypotenuse = pow(horizDist, 2.0) + pow(vertDist, 2.0);

							float result = sqrt(hypotenuse);
							
							float pix_per_sec = result / .00833;
							float exitVelocity = pix_per_sec / 141.273055;
							if (goingUpOne == 0) {
								//Adding an extra 10mph to accommodate for the weirdly slow exit velos
								exitVelocity = exitVelocity + 10;
							}
#ifdef DEBUG			
							cout << "Exit Velocity: " << exitVelocity << endl;
#endif
							prevX = rect_points[j].x;
							prevY = rect_points[j].y;
							exitVeloAvg = exitVeloAvg + exitVelocity;
							exitVeloAvgCounter++;
						}
					}
					//cout << "Rect point num: " << j << " : " << rect_points[j] << ", " << rect_points[(j + 1) % 4] << endl;
				}
#ifdef DEBUG			
				imshow("Contours", frameTwo);
				waitKey(0);
#endif
				
				
			}
			notFoundNum = 0;
			if (notFoundTrigger == false) {
				notFoundTrigger = true;
			}
			push_count++;
		}
		else if(notFoundTrigger) {
			notFoundNum++;
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

	float finalExitVelo = exitVeloAvg / exitVeloAvgCounter;
	cout << "Exit Velocity: " << finalExitVelo << endl;

	float finalLaunchAngle = launchAngleAvg / launchAngleAvgNum;
	cout << "Launch Angle: " << finalLaunchAngle << endl;

	cin.get();
	return 0;
}

