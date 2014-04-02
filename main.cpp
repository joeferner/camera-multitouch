#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/video/background_segm.hpp>
#include <vector>
#include "input.h"

#define WINDOW_NAME   "Camera Multi-touch"
#define CAMERA_COUNT  1
#define BACKGROUND_SUBTRACTOR_REFRESH 0.001
#define WINDOW_IMG_HEIGHT 768
#define WINDOW_IMG_WIDTH 1024

CvCapture* capture[CAMERA_COUNT];
InputContext* inputContext;
cv::Mat backgroundSubtractorOutput[CAMERA_COUNT];
cv::Mat* backgroundSubtractorColorOutput[CAMERA_COUNT];
cv::Ptr<cv::BackgroundSubtractor> backgroundSubtractor[CAMERA_COUNT];
cv::Ptr<cv::FeatureDetector> blobDetector[CAMERA_COUNT];
cv::Mat windowImg(WINDOW_IMG_HEIGHT + 1, WINDOW_IMG_WIDTH + 1, CV_8UC3 /*CV_8UC1*/);
cv::Rect* cropRects[CAMERA_COUNT];

int captureInput = 0;

void initInputContext();
void releaseInputContext();
void initCameraCapture();
void releaseCameraCapture();
void initBackgroundSubtractors();
void initDetectors();
void displayCaptures(int cameraIdx, std::vector<cv::KeyPoint> &keypoints);

int main(int argc, char** argv) {
  if (captureInput) {
    initInputContext();
  }
  initCameraCapture();
  initBackgroundSubtractors();
  initDetectors();

  for (int i = 0; i < CAMERA_COUNT; i++) {
    cropRects[i] = new cv::Rect(0, 500, 1280, 100);
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

        cv::Mat targetColorDisplayImg(windowImg, cv::Rect(0, y, WINDOW_IMG_WIDTH, cropRects[i]->height));
        y += cropRects[i]->height;
        cv::resize(croppedImg, targetColorDisplayImg, targetColorDisplayImg.size());

        cv::Mat targetDisplayImg(windowImg, cv::Rect(0, y, WINDOW_IMG_WIDTH, cropRects[i]->height));
        y += cropRects[i]->height;
        cv::resize(*backgroundSubtractorColorOutput[i], targetDisplayImg, targetDisplayImg.size());
      } catch (cv::Exception e) {
        e.formatMessage();
      }
    }

    cv::imshow(WINDOW_NAME, windowImg);

    if (cv::waitKey(10) >= 0) {
      break;
    }
  }

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
    if (captureInput) {
      inputMouseMove(inputContext, x, y);
    }
  }
  for (int j = 0; j < keypoints.size(); j++) {
    float x = keypoints[j].pt.x;
    float y = keypoints[j].pt.y;
    printf("%f, %f\n", x, y);
    cv::line(*backgroundSubtractorColorOutput[cameraIdx], cv::Point(x, y - 10), cv::Point(x, y + 10), cv::Scalar(0, 255, 0));
    cv::line(*backgroundSubtractorColorOutput[cameraIdx], cv::Point(x - 10, y), cv::Point(x + 10, y), cv::Scalar(0, 255, 0));
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