
///// INCLUDES /////
#include <cstdlib>
#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <stdio.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/opencv.hpp"

///// SCOPE /////
using namespace std;
using namespace cv;

///// DEFINITIONS /////
#define DELAY_MS    200
#define MAX_THRESH  255
#define nodog 0
#define yesdog 1

///// COLORS /////
#define HUE_GREEN   60
#define HUE_PINK    220
#define HUE_BLUE    160
#define HUE_RED     7
#define HUE_PURPLE  180


int FindDog(int hue, VideoCapture video);
void FindColor(Mat& notprocessed, Mat& processed, int color);

int main()
{
    int color;
    int answer=nodog;

    // Create named window with a title of 'Stream'
    namedWindow("Not Processed");

    // Create named window to display processed image
    namedWindow("Processed");


    cout << "Enter Hue: ";
    cin >> color;
    cout << endl;
   // Open streaming video on default system camera
    // stream is a variable of type videocapture

    VideoCapture video(0);

    while(answer==nodog)
    {
       answer=FindDog(color, video);
       cout << answer << endl;
    }

    return EXIT_SUCCESS;
}


int FindDog(int color, VideoCapture video)
{
    int pixels;
    int answer=nodog;
    Size_<int> windowsize; // variable of type size that is a data structure



    // Get the dimensions of the camera stream in pixels
    windowsize.width = video.get(CV_CAP_PROP_FRAME_WIDTH);
    windowsize.height = video.get(CV_CAP_PROP_FRAME_HEIGHT);


    // Create Matrix data type with a size of
    //  'windowsize', 8-bit colors, and 3 colors per pixel
    Mat notprocessed(windowsize, CV_8UC3);

    // Create Matrix to perform processing with
    Mat processed (windowsize, CV_8UC3);

    video.read(notprocessed);
    imshow("Not Processed",notprocessed);

    FindColor(notprocessed, processed, color);

    imshow("Processed", processed);
    waitKey(DELAY_MS);

    return answer;
}


void FindColor(Mat& notprocessed, Mat& processed, int color)
{
    Mat convertbw;

    int colormax = color+10;
    int colormin = color-10;

    cvtColor(notprocessed,processed,CV_BGR2HLS_FULL);

    // Choose threshold boundry pixels
    Scalar pixelmin(colormin, 20, 45);    // Saturation Range: 0->255
    Scalar pixelmax(colormax, 230, 255);  // Lightness Range: 0->255

    inRange(processed, pixelmin, pixelmax, convertbw);

    vector<vector<Point> > listofshapes;

    findContours(convertbw, listofshapes, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

    processed = Mat::zeros(notprocessed.size(), notprocessed.type());

    drawContours(processed, listofshapes, -1, Scalar::all(255), CV_FILLED);

    processed &= notprocessed;
}
