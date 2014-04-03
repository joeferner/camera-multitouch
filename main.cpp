#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/video/background_segm.hpp>
#include <vector>
#include <math.h>
#include "input.h"

#define WINDOW_NAME                    "Camera Multi-touch"
#define CAMERA_COUNT                   2
#define PI                             3.1415
#define BACKGROUND_SUBTRACTOR_REFRESH  0.001
#define PREVIEW_WINDOW_WIDTH           1024
#define PREVIEW_WINDOW_HEIGHT          768
#define CAMERA_DISTANCE                2000       /* calibrate to the x distance of the two cameras */
#define CAMERA_DISTANCE_FROM_SCREEN_X  200        /* calibrate to the x distance from top of screen to camera */
#define CAMERA_DISTANCE_FROM_SCREEN_Y  200        /* calibrate to the y distance from top of screen to camera */
#define CAMERA_WIDTH                   1280       /* set to camera x resolution */
#define CAMERA_FOV                     1.20427718 /* 69 degrees */

CvCapture* capture[CAMERA_COUNT];
InputContext* inputContext;
cv::Mat backgroundSubtractorOutput[CAMERA_COUNT];
cv::Mat* backgroundSubtractorColorOutput[CAMERA_COUNT];
cv::Ptr<cv::BackgroundSubtractor> backgroundSubtractor[CAMERA_COUNT];
cv::Ptr<cv::FeatureDetector> blobDetector[CAMERA_COUNT];
cv::Mat windowImg(PREVIEW_WINDOW_HEIGHT + 1, PREVIEW_WINDOW_WIDTH + 1, CV_8UC3 /*CV_8UC1*/);
cv::Rect* cropRects[CAMERA_COUNT];
cv::KeyPoint detectedBlobCoords[CAMERA_COUNT];

int captureInput = 0;

void initInputContext();
void releaseInputContext();
void initCameraCapture();
void releaseCameraCapture();
void initBackgroundSubtractors();
void initDetectors();
void displayCaptures(int cameraIdx, std::vector<cv::KeyPoint> &keypoints);
void calculateLocations();

int main(int argc, char** argv) {
  if (captureInput) {
    initInputContext();
  }
  initCameraCapture();
  initBackgroundSubtractors();
  initDetectors();

  for (int i = 0; i < CAMERA_COUNT; i++) {
    cropRects[i] = new cv::Rect(0, 500, CAMERA_WIDTH, 100);
    backgroundSubtractorColorOutput[i] = NULL;
  }

  cv::namedWindow(WINDOW_NAME);

  printf("begin loop\n");
  while (1) {
    int y = 0;
    for (int i = 0; i < CAMERA_COUNT; i++) {
      try {
        IplImage* cameraIplImg = cvQueryFrame(capture[i]);
        printf("depth: %d, ch: %d, width: %d, height: %d\n", cameraIplImg->depth, cameraIplImg->nChannels, cameraIplImg->width, cameraIplImg->height);
        cv::Mat cameraImg = cameraIplImg;
        cv::Mat croppedImg = cameraImg(*cropRects[i]);

        backgroundSubtractor[i]->operator ()(croppedImg, backgroundSubtractorOutput[i], BACKGROUND_SUBTRACTOR_REFRESH);
        if (backgroundSubtractorColorOutput[i] == NULL) {
          backgroundSubtractorColorOutput[i] = new cv::Mat(backgroundSubtractorOutput[i].rows, backgroundSubtractorOutput[i].cols, CV_8UC3);
        }
        *backgroundSubtractorColorOutput[i] = cv::Scalar(0, 0, 0);
        backgroundSubtractorColorOutput[i]->setTo(cv::Scalar(255, 0, 0), backgroundSubtractorOutput[i]);

        std::vector<cv::KeyPoint> keypoints;
        blobDetector[i]->detect(backgroundSubtractorOutput[i], keypoints);

        displayCaptures(i, keypoints);

        cv::Mat targetColorDisplayImg(windowImg, cv::Rect(0, y, PREVIEW_WINDOW_WIDTH, cropRects[i]->height));
        y += cropRects[i]->height;
        cv::resize(croppedImg, targetColorDisplayImg, targetColorDisplayImg.size());

        cv::Mat targetDisplayImg(windowImg, cv::Rect(0, y, PREVIEW_WINDOW_WIDTH, cropRects[i]->height));
        y += cropRects[i]->height;
        cv::resize(*backgroundSubtractorColorOutput[i], targetDisplayImg, targetDisplayImg.size());
      } catch (cv::Exception e) {
        e.formatMessage();
      }
    }

    cv::imshow(WINDOW_NAME, windowImg);
    calculateLocations();

    if (cv::waitKey(10) >= 0) {
      break;
    }
  } // END while(1))

  releaseCameraCapture();

  if (captureInput) {
    releaseInputContext();
  }

  cv::destroyAllWindows();

  return 0;
}

void displayCaptures(int cameraIdx, std::vector<cv::KeyPoint> &keypoints) {
  if (keypoints.size() > 0) {
    float x = keypoints[0].pt.x;
    float y = keypoints[0].pt.y;
    detectedBlobCoords[cameraIdx].pt.x = x;
    detectedBlobCoords[cameraIdx].pt.y = y;
    printf("%f, %f\n", x, y);
  } else {
    detectedBlobCoords[cameraIdx].pt.x = -1;
    detectedBlobCoords[cameraIdx].pt.y = -1;
  }
  for (int j = 0; j < keypoints.size(); j++) {
    float x = keypoints[j].pt.x;
    float y = keypoints[j].pt.y;
    cv::line(*backgroundSubtractorColorOutput[cameraIdx], cv::Point(x, y - 10), cv::Point(x, y + 10), cv::Scalar(0, 255, 0));
    cv::line(*backgroundSubtractorColorOutput[cameraIdx], cv::Point(x - 10, y), cv::Point(x + 10, y), cv::Scalar(0, 255, 0));
  }
}

// see camera_layout
void calculateLocations() {
  if (CAMERA_COUNT != 2) {
    return;
  }
  if (detectedBlobCoords[0].pt.x < 0 || detectedBlobCoords[1].pt.x < 0) {
    return;
  }

  float theta_A = detectedBlobCoords[0].pt.x / (float) CAMERA_WIDTH * CAMERA_FOV;
  float theta_B = detectedBlobCoords[1].pt.x / (float) CAMERA_WIDTH * CAMERA_FOV;
  float theta_P = PI - theta_A - theta_B;

  float d_A = CAMERA_DISTANCE * sin(theta_A) / sin(theta_P);
  float d_B = CAMERA_DISTANCE * sin(theta_B) / sin(theta_P);

  float x_prime = cos(theta_A) * d_A;
  float y_prime = sin(theta_A) * d_A;

  float x = x_prime - CAMERA_DISTANCE_FROM_SCREEN_X;
  float y = y_prime - CAMERA_DISTANCE_FROM_SCREEN_Y;

  printf(
          "x=%d, y=%d (theta_A=%.2f, theta_B=%.2f, theta_P=%.2f, d_A=%.2f, d_B=%.2f, x_prime=%d, y_prime=%d)\n",
          x, y,
          theta_A, theta_B, theta_P,
          d_A, d_B,
          x_prime, y_prime);
  if (captureInput) {
    inputMouseMove(inputContext, x, y);
  }
}

void initInputContext() {
  InputContext* inputContext = inputBegin();
  if (!inputContext) {
    fprintf(stderr, "could not begin input\n");
    exit(-1);
  }
}

void releaseInputContext() {
  inputEnd(inputContext);
}

void initCameraCapture() {
  for (int i = 0; i < CAMERA_COUNT; i++) {
    capture[i] = cvCaptureFromCAM(i);
    if (!capture[i]) {
      fprintf(stderr, "could not capture %d\n", i);
      exit(-1);
    }
  }
}

void releaseCameraCapture() {
  for (int i = 0; i < CAMERA_COUNT; i++) {
    cvReleaseCapture(&capture[0]);
  }
}

void initBackgroundSubtractors() {
  for (int i = 0; i < CAMERA_COUNT; i++) {
    backgroundSubtractor[i] = new cv::BackgroundSubtractorMOG();
  }
}

void initDetectors() {
  cv::SimpleBlobDetector::Params blobDetectorParams;
  blobDetectorParams.minDistBetweenBlobs = 50.0f;
  blobDetectorParams.filterByInertia = false;
  blobDetectorParams.filterByConvexity = false;
  blobDetectorParams.filterByColor = false;
  blobDetectorParams.filterByCircularity = false;
  blobDetectorParams.filterByArea = true;
  blobDetectorParams.minArea = 500.0f;
  blobDetectorParams.maxArea = 10000.0f;
  for (int i = 0; i < CAMERA_COUNT; i++) {
    blobDetector[i] = new cv::SimpleBlobDetector(blobDetectorParams);
  }
}