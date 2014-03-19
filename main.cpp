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

  CvCapture * capture[2];

  capture[0] = cvCaptureFromCAM(0);
  if (!capture[0]) {
    fprintf(stderr, "could not capture 0\n");
    return -1;
  }

  capture[1] = cvCaptureFromCAM(1);
  if (!capture[1]) {
    fprintf(stderr, "could not capture 1\n");
    return -1;
  }

  cv::namedWindow("mog");

  cv::Mat sideBySide(768, 1025, CV_8UC1);
  cv::Mat fgMaskMog[2];
  cv::Ptr<cv::BackgroundSubtractor> mog[2];
  mog[0] = new cv::BackgroundSubtractorMOG();
  mog[1] = new cv::BackgroundSubtractorMOG();

  cv::SimpleBlobDetector::Params blobDetectorParams;
  blobDetectorParams.minDistBetweenBlobs = 50.0f;
  blobDetectorParams.filterByInertia = false;
  blobDetectorParams.filterByConvexity = false;
  blobDetectorParams.filterByColor = false;
  blobDetectorParams.filterByCircularity = false;
  blobDetectorParams.filterByArea = true;
  blobDetectorParams.minArea = 500.0f;
  blobDetectorParams.maxArea = 10000.0f;
  cv::Ptr<cv::FeatureDetector> blobDetector[2];
  blobDetector[0] = new cv::SimpleBlobDetector(blobDetectorParams);
  blobDetector[1] = new cv::SimpleBlobDetector(blobDetectorParams);

  printf("begin loop\n");
  while (1) {
    for (int i = 0; i < 2; i++) {
      try {
        IplImage* iplImg = cvQueryFrame(capture[i]);
        cv::Mat img = iplImg;

        mog[i]->operator ()(img, fgMaskMog[i], 0.001);

        std::vector<cv::KeyPoint> keypoints;
        blobDetector[i]->detect(fgMaskMog[i], keypoints);

        if (keypoints.size() > 0) {
          float x = keypoints[0].pt.x;
          float y = keypoints[0].pt.y;
          //inputMouseMove(inputContext, x, y);
        }
        for (int j = 0; j < keypoints.size(); j++) {
          float x = keypoints[j].pt.x;
          float y = keypoints[j].pt.y;
          printf("%f, %f\n", x, y);
          cv::line(fgMaskMog[i], cv::Point(x, y - 10), cv::Point(x, y + 10), cv::Scalar(0, 0, 0));
          cv::line(fgMaskMog[i], cv::Point(x - 10, y), cv::Point(x + 10, y), cv::Scalar(0, 0, 0));
        }

        if (i == 0) {
          cv::Mat left(sideBySide, cv::Rect(0, 0, 512, 384));
          cv::resize(fgMaskMog[0], left, left.size());
        }
        if (i == 1) {
          cv::Mat right(sideBySide, cv::Rect(512, 0, 512, 384));
          cv::resize(fgMaskMog[1], right, right.size());
        }
      } catch (cv::Exception e) {

      }
    }

    cv::imshow("mog", sideBySide);

    if (cv::waitKey(10) >= 0) {
      break;
    }
  }

  cvReleaseCapture(&capture[0]);
  cvReleaseCapture(&capture[1]);

  cv::destroyAllWindows();

  inputEnd(inputContext);

  return 0;
}
