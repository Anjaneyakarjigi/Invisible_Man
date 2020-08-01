
#include <iostream>
#include <chrono>
#include <thread>
#include <opencv2\opencv.hpp>

#define TRACT_BAR_MODE 0		// Enable if you want to get, UP and LOW HSV values for masking  

#define LOW_HSV		Vec3b lowHsv(143,108,9)		// Lower Limit for Masking
#define HIGH_HSV	Vec3b highHsv(179, 255, 136)	// Upper Limit for Masking

using namespace std;
using namespace cv;

const int max_value_H = 179;
const int max_value = 255;
const String window_capture_name = "Video Capture";
const String window_detection_name = "Object Detection";
int low_H = 0, low_S = 0, low_V = 0;
int high_H = max_value_H, high_S = max_value, high_V = max_value;


#if TRACT_BAR_MODE
static void on_low_H_thresh_trackbar(int, void *)
{
	low_H = min(high_H - 1, low_H);
	setTrackbarPos("Low H", window_detection_name, low_H);
}
static void on_high_H_thresh_trackbar(int, void *)
{
	high_H = max(high_H, low_H + 1);
	setTrackbarPos("High H", window_detection_name, high_H);
}
static void on_low_S_thresh_trackbar(int, void *)
{
	low_S = min(high_S - 1, low_S);
	setTrackbarPos("Low S", window_detection_name, low_S);
}
static void on_high_S_thresh_trackbar(int, void *)
{
	high_S = max(high_S, low_S + 1);
	setTrackbarPos("High S", window_detection_name, high_S);
}
static void on_low_V_thresh_trackbar(int, void *)
{
	low_V = min(high_V - 1, low_V);
	setTrackbarPos("Low V", window_detection_name, low_V);
}
static void on_high_V_thresh_trackbar(int, void *)
{
	high_V = max(high_V, low_V + 1);
	setTrackbarPos("High V", window_detection_name, high_V);
}

short createTrackBar()
{
	// Trackbars to set thresholds for HSV values
	createTrackbar("Low H", window_detection_name, &low_H, max_value_H, on_low_H_thresh_trackbar);	//&low_H, max_value_H,
	createTrackbar("High H", window_detection_name, &high_H, max_value_H, on_high_H_thresh_trackbar);
	createTrackbar("Low S", window_detection_name, &low_S, max_value, on_low_S_thresh_trackbar);
	createTrackbar("High S", window_detection_name, &high_S, max_value, on_high_S_thresh_trackbar);
	createTrackbar("Low V", window_detection_name, &low_V, max_value, on_low_V_thresh_trackbar);
	createTrackbar("High V", window_detection_name, &high_V, max_value, on_high_V_thresh_trackbar);
	return 1;
}

#endif 


char *url = "http://10.113.168.111:8080/video"; 
VideoCapture cap(url);

/* This Thread and Mutex is required when Frames reading is very slow which results in lag of real time video */
Mutex mute;
void cap_thread_to_skip_frames()
{
	while (cv::waitKey(1) != 27)
	{
		Mat frame;
		mute.lock();
		cap.read(frame);
		mute.unlock();


	}
}

int main()
{

	if (!cap.isOpened())
	{
		cout << "Unable to open!" << endl;
	}

	/* Sleep for 2 seconds before capture background image */
	std::this_thread::sleep_for(std::chrono::microseconds(2000));

#if TRACT_BAR_MODE
	cv::namedWindow(window_detection_name);
#endif

	Mat backgroundImg,img,grayImg, hsvImg;
	/* Get the Background Image */
	for (int i = 0; i < 60; i++)
		cap.read(backgroundImg);
	cv::flip(backgroundImg, backgroundImg, 1);

	/* Open this Only if your frames reading is faster and frames get accumulate with older frames */
	std::thread skip_frames(cap_thread_to_skip_frames);
	skip_frames.detach();

	while (1)
	{
		mute.lock();
		cap.read(img);
		mute.unlock();

#if TRACT_BAR_MODE
		imshow("Input Image", img);
		waitKey(1);
		createTrackBar();
#endif

		cv::flip(img, img, 1);

		/* Convert image to HSV space */
		cvtColor(img, hsvImg, CV_BGR2HSV);

#if !TRACT_BAR_MODE
		LOW_HSV;
		HIGH_HSV;
#else
		Vec3b lowHsv(low_H, low_S, low_V);
		Vec3b highHsv(high_H, high_S, high_V);
#endif
		Mat mask1, mask2,mask;
		inRange(hsvImg, lowHsv, highHsv, mask1);

#if TRACT_BAR_MODE
		cv::namedWindow("masked_image", CV_WINDOW_NORMAL);
		cv::imshow("masked_image", mask1);
		cv::waitKey(1);
#else

		/* Dilete the mask image to mute the noice */
		morphologyEx(mask1, mask1, cv::MORPH_OPEN, (3,3));
		morphologyEx(mask1, mask1, cv::MORPH_DILATE, (3, 3));

		/* Filter out Noise from Thresholding of Image */
		vector<vector<cv::Point>> contours, cnts;
		vector<Vec4i> hiaerchy;
		/* Extact the bigger contour in the image mask */
		cv::findContours(mask1, contours, hiaerchy, RETR_TREE, CV_CHAIN_APPROX_SIMPLE);

		int large_contour_index = -1;
		double max_contour_area = 0;
		if (contours.size() > 0)
			for (int i = 0; i < contours.size(); i++)
			{// Get Larger area in the threshold image
				if (contourArea(contours[i]) > max_contour_area)
				{
					max_contour_area = contourArea(contours[i]);
					large_contour_index = i;
				}
			}

		/* Creating inverted mask to segment out of masked color from frame */
		Mat EmptyImgMask = Mat::zeros(mask1.size(), CV_8UC1);
		if (large_contour_index != -1)
		{
			drawContours(EmptyImgMask, contours, large_contour_index, (0, 0, 255), CV_FILLED); //CV_FILLED
			bitwise_not(EmptyImgMask, mask2);
			mask1 = 0;
			mask1 = EmptyImgMask;
		}
		
		bitwise_not(mask1, mask2);

		Mat maskedImg, maskedImg_bg;
		/* Segment out required color mask out of frame */
		bitwise_and(img, img, maskedImg, mask2);

		/* Create image showing static background image */
		bitwise_and(backgroundImg, backgroundImg, maskedImg_bg, mask1);

		/* generate final output */
		Mat finalout;
		addWeighted(maskedImg, 1, maskedImg_bg, 1, 0, finalout);
		namedWindow("Mazic_out", WINDOW_AUTOSIZE);
		imshow("Mazic_out", finalout);
		waitKey(1);
#endif

	}

	return 0;
}



