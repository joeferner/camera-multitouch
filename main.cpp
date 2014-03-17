#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/video/background_segm.hpp>
#include <vector>
#include "input.h"

int main(int argc, char** argv) {
  InputContext* inputContext = inputBegin();
  if (!inputContext) {
    fprintf(stderr, "could not begin input\n");
    return -1;
  }

  CvCapture* capture = cvCaptureFromCAM(0);
  if (!capture) {
    fprintf(stderr, "could not capture\n");
    return -1;
  }

  cv::namedWindow("frame");
  cv::namedWindow("mog");

  cv::Mat fgMaskMog;
  cv::Ptr<cv::BackgroundSubtractor> mog = new cv::BackgroundSubtractorMOG();

  cv::SimpleBlobDetector::Params blobDetectorParams;
  blobDetectorParams.minDistBetweenBlobs = 50.0f;
  blobDetectorParams.filterByInertia = false;
  blobDetectorParams.filterByConvexity = false;
  blobDetectorParams.filterByColor = false;
  blobDetectorParams.filterByCircularity = false;
  blobDetectorParams.filterByArea = true;
  blobDetectorParams.minArea = 500.0f;
  blobDetectorParams.maxArea = 10000.0f;
  cv::Ptr<cv::FeatureDetector> blobDetector = new cv::SimpleBlobDetector(blobDetectorParams);

  while (1) {
    IplImage* iplImg = cvQueryFrame(capture);
    cv::Mat img = iplImg;

    mog->operator ()(img, fgMaskMog, 0.001);

    std::vector<cv::KeyPoint> keypoints;
    blobDetector->detect(fgMaskMog, keypoints);

    if (keypoints.size() > 0) {
      float x = keypoints[0].pt.x;
      float y = keypoints[0].pt.y;
      inputMouseMove(inputContext, x, y);
    }
    for (int i = 0; i < keypoints.size(); i++) {
      float x = keypoints[i].pt.x;
      float y = keypoints[i].pt.y;
      printf("%f, %f\n", x, y);
      cv::line(fgMaskMog, cv::Point(x, y - 10), cv::Point(x, y + 10), cv::Scalar(0, 0, 0));
      cv::line(fgMaskMog, cv::Point(x - 10, y), cv::Point(x + 10, y), cv::Scalar(0, 0, 0));
      cv::line(img, cv::Point(x, y - 10), cv::Point(x, y + 10), cv::Scalar(0, 0, 255));
      cv::line(img, cv::Point(x - 10, y), cv::Point(x + 10, y), cv::Scalar(0, 0, 255));
    }

    cv::imshow("frame", img);
    cv::imshow("mog", fgMaskMog);

    if (cv::waitKey(10) >= 0) {
      break;
    }
  }

  cvReleaseCapture(&capture);

  cv::destroyAllWindows();

  inputEnd(inputContext);

  return 0;
}
