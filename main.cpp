#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/video/background_segm.hpp>
#include <vector>
#include <math.h>
#include <sys/time.h>
#include "input.h"

#define WINDOW_NAME                    "Camera Multi-touch"
#define CAMERA_COUNT                   2
#define PI                             3.1415
#define BACKGROUND_SUBTRACTOR_REFRESH  0.001
#define PREVIEW_WINDOW_WIDTH           1920
#define PREVIEW_WINDOW_HEIGHT          200

#define TV_HEIGHT_MM                   1510.0
#define TV_WIDTH_MM                    2608.0
#define DCX_MM                         570.0
#define DCY_MM                         260.0
#define PX_PER_MM                      (1080.0 / TV_HEIGHT_MM)
#define CAMERA_DISTANCE                (PX_PER_MM * (TV_WIDTH_MM + 2 * DCX_MM))
#define CAMERA_DISTANCE_FROM_SCREEN_X  (PX_PER_MM * DCX_MM)
#define CAMERA_DISTANCE_FROM_SCREEN_Y  (PX_PER_MM * DCY_MM)
#define CAMERA_WIDTH                   1920       /* set to camera x resolution */
#define CAMERA_FOV                     1.20427718 /* 69 degrees */
#define TIME_MS_UNTIL_MOUSE_CLICK      1000
#define MOUSE_CLICK_RADIUS             50

CvCapture* capture[CAMERA_COUNT];
InputContext* inputContext;
cv::Mat backgroundSubtractorOutput[CAMERA_COUNT];
cv::Mat* backgroundSubtractorColorOutput[CAMERA_COUNT];
cv::Ptr<cv::BackgroundSubtractor> backgroundSubtractor[CAMERA_COUNT];
cv::Ptr<cv::FeatureDetector> blobDetector[CAMERA_COUNT];
cv::Mat windowImg(PREVIEW_WINDOW_HEIGHT + 1, PREVIEW_WINDOW_WIDTH + 1, CV_8UC3 /*CV_8UC1*/);
cv::Rect* cropRects[CAMERA_COUNT];
cv::KeyPoint detectedBlobCoords[CAMERA_COUNT];

int captureInput = 1;
int mouseLButtonState = MOUSE_LBUTTON_UP;
int mouseX = 0;
int mouseY = 0;
int mouseStartLButtonDownX = 0;
int mouseStartLButtonDownY = 0;
long mouseStartLButtonDownTime = 0;

void initInputContext();
void releaseInputContext();
void initCameraCapture();
void releaseCameraCapture();
void initBackgroundSubtractors();
void initDetectors();
void displayCaptures(int cameraIdx, std::vector<cv::KeyPoint> &keypoints);
void calculateLocations();
long timems();
int distance(int x1, int y1, int x2, int y2);

int main(int argc, char** argv) {
  if (captureInput) {
    initInputContext();
  }
  initCameraCapture();
  initBackgroundSubtractors();
  initDetectors();

  for (int i = 0; i < CAMERA_COUNT; i++) {
    backgroundSubtractorColorOutput[i] = NULL;
  }
  cropRects[0] = new cv::Rect(0, 610, CAMERA_WIDTH, 50);
  cropRects[1] = new cv::Rect(0, 525, CAMERA_WIDTH, 50);

  cv::namedWindow(WINDOW_NAME);

  printf("begin loop\n");
  while (1) {
    int y = 0;
    for (int i = 0; i < CAMERA_COUNT; i++) {
      try {
        IplImage* cameraIplImg = cvQueryFrame(capture[i]);
        // printf("depth: %d, ch: %d, width: %d, height: %d\n", cameraIplImg->depth, cameraIplImg->nChannels, cameraIplImg->width, cameraIplImg->height);
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
    // printf("%f, %f\n", x, y);
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
    if (mouseLButtonState == MOUSE_LBUTTON_DOWN) {
      printf("mouse up\n");
      inputMouseUp(inputContext, mouseX, mouseY);
      mouseLButtonState = MOUSE_LBUTTON_UP;
      mouseStartLButtonDownTime = 0;
    }
    return;
  }

  float camera_ct = atan2(CAMERA_DISTANCE_FROM_SCREEN_Y, CAMERA_DISTANCE - CAMERA_DISTANCE_FROM_SCREEN_X);

  float theta_A = detectedBlobCoords[0].pt.x / (float) CAMERA_WIDTH * CAMERA_FOV + camera_ct;
  float theta_B = (CAMERA_WIDTH - detectedBlobCoords[1].pt.x) / (float) CAMERA_WIDTH * CAMERA_FOV + camera_ct;
  float theta_P = PI - theta_A - theta_B;

  float d_A = CAMERA_DISTANCE * sin(theta_A) / sin(theta_P);
  float d_B = CAMERA_DISTANCE * sin(theta_B) / sin(theta_P);

  float x_prime = cos(theta_A) * d_B;
  float y_prime = sin(theta_A) * d_B;

  int x = x_prime - CAMERA_DISTANCE_FROM_SCREEN_X;
  int y = y_prime - CAMERA_DISTANCE_FROM_SCREEN_Y;

  printf(
          "x=%d, y=%d (thetaA=%.2f, thetaB=%.2f, thetaP=%.2f, dA=%.0f, dB=%.0f, x'=%.0f, y'=%.0f)\n",
          x, y,
          theta_A, theta_B, theta_P,
          d_A, d_B,
          x_prime, y_prime);
  if (captureInput) {
    if (mouseLButtonState == MOUSE_LBUTTON_UP) {
      int d = distance(x, y, mouseStartLButtonDownX, mouseStartLButtonDownY);
      long t = timems();
      printf("button down? d=%d, time=%d (%d)\n", d, t, t - mouseStartLButtonDownTime);
      if (mouseStartLButtonDownTime == 0 || d > MOUSE_CLICK_RADIUS) {
        mouseStartLButtonDownTime = t;
        mouseStartLButtonDownX = x;
        mouseStartLButtonDownY = y;
        printf("begin L-Button timer %d,%d %ldms\n", x, y, mouseStartLButtonDownTime);
      }
      if (t - mouseStartLButtonDownTime > TIME_MS_UNTIL_MOUSE_CLICK) {
        inputMouseDown(inputContext, x, y);
        mouseLButtonState = MOUSE_LBUTTON_DOWN;
        printf("mouse down\n");
      }
    }
    inputMouseMove(inputContext, mouseLButtonState, x, y);
    mouseX = x;
    mouseY = y;
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

long timems() {
  struct timeval start;
  gettimeofday(&start, NULL);
  return ((start.tv_sec) * 1000 + start.tv_usec / 1000.0) + 0.5;
}

int distance(int x1, int y1, int x2, int y2) {
  int dx = x2 - x1;
  int dy = y2 - y1;
  return sqrt(dx * dx + dy * dy);
}
