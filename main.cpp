#include <stdio.h>
#include <opencv2/opencv.hpp>
#include <opencv2/video/background_segm.hpp>

int main(int argc, char** argv) {
  CvCapture* capture = cvCaptureFromCAM(0);
  if (!capture) {
    fprintf(stderr, "could not capture\n");
    return -1;
  }

  cv::namedWindow("frame");
  cv::namedWindow("mog");

  cv::Mat fgMaskMog;
  cv::Ptr<cv::BackgroundSubtractor> mog = new cv::BackgroundSubtractorMOG();

  for (;;) {
    IplImage* iplImg = cvQueryFrame(capture);
    cv::Mat img = iplImg;

    mog->operator ()(img, fgMaskMog);

    cv::imshow("frame", img);
    cv::imshow("mog", fgMaskMog);

    if (cv::waitKey(10) >= 0) {
      break;
    }
  }

  cvReleaseCapture(&capture);

  cv::destroyAllWindows();

  return 0;
}
