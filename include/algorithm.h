#ifndef _algorithm_h
#define _algorithm_h

#include "capture_frame.h"
#include <opencv2/opencv.hpp>

class Algorithm
{
private:
  Logger logger;

protected:
public:
  std::vector<cv::Point2d> matched_current_image, matched_previous_image;
  std::vector<cv::KeyPoint> keypoints_previous_image, //keypoints variables
      keypoints_current_image, inliers_current_image, inliers_previous_image;
  cv::Mat description_current_image, description_previous_image;
  cv::Mat current_image, previous_image;
  float nn_match_ratio;
  int CLAHE_clip_limit;

  float inlier_threshold; // Distance threshold to identify inliers

  //Image Dehazing
  CaptureFrame dark_channel(CaptureFrame input_image, int radius); //Dark channel generator
  CaptureFrame saturation_map(CaptureFrame input, int radius);     //Saturation map creator
  CaptureFrame u_dark_channel(CaptureFrame input, int radius);     //UDCP dark channel only from green and blue color
  CaptureFrame red_irradiance(CaptureFrame input, int radius);     //Red channel extraction and depth map generation

  //Image Enhancing

  //Public
  CaptureFrame CLAHE_dehaze_shallow(CaptureFrame input_image);
  CaptureFrame CLAHE_dehaze(CaptureFrame input_image);  //CLAHE contrast enhancement
  cv::Mat CLAHE_dehaze(cv::Mat input_image);            //CLAHE contrast enhancement
  void set_CLAHE_clip_limit(int clip_limit);            //support function for CLAHE dehazing
  CaptureFrame hist_equalize(CaptureFrame input_image); // histogram equilization algorithm

  //Fusion Enhancing
  CaptureFrame balance_white_shallow(CaptureFrame input);
  CaptureFrame balance_white(CaptureFrame input);      //white balance image
  void balance_white(cv::Mat input);                   //white balance image
  CaptureFrame laplacian_contrast(CaptureFrame input); //Laplacian contrast
  CaptureFrame local_contrast(CaptureFrame input);     //Local contrast
  CaptureFrame saliency_contrast(CaptureFrame input);  //Saliency contrast
  CaptureFrame exposedness(CaptureFrame input);        //Exposedness
  //foloowing are used for blending process using pyramid
  std::vector<cv::Mat> gaussion_pyrdown(CaptureFrame image, int level);  //gaussian pyramid construction
  std::vector<cv::Mat> laplacian_pyrdown(CaptureFrame image, int level); //laplacian pyramid construction
  std::vector<cv::Mat> laplacian_pyrdown(cv::Mat image, int level);      //laplacian pyramid construction
  CaptureFrame pyr_reconstruct(std::vector<cv::Mat> input, int levels);  //Pyramid reconstruction

  // //Image Mosaicing
  //Akaze keypoint creation with discription
  void AKAZE_feature_points(CaptureFrame image1, CaptureFrame image2);
  //ORB keypoint creation with description
  void ORB_feature_points(CaptureFrame image1, CaptureFrame image2);
  //Brute Force matcher : creates matches
  void BF_matcher();

  Algorithm(); //The constructor
};

#endif