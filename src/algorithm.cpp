

#include "algorithm.h"
#include "logger.h"
#include "capture_frame.h"
#include <opencv2/opencv.hpp>

CaptureFrame Algorithm::CLAHE_dehaze(CaptureFrame object) //CLAHE based basic dehazing algorithm
{
    cv::Mat segmented;
    segmented = CLAHE_dehaze(object.retrieve_image());
    CaptureFrame output(segmented, "Dehazed image");
    return output;
}
cv::Mat Algorithm::CLAHE_dehaze(cv::Mat object) //CLAHE based basic dehazing algorithm
{
    cv::Mat image_hsv;
    cvtColor(object, image_hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> channels;
    split(image_hsv, channels);
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(CLAHE_clip_limit);
    clahe->apply(channels[2], channels[2]);
    clahe->apply(channels[1], channels[1]);
    channels[2] = channels[2] * 0.85;
    merge(channels, image_hsv);
    cv::Mat dehazed;
    cvtColor(image_hsv, dehazed, cv::COLOR_HSV2BGR);
    // GaussianBlur(dehazed, dehazed, cv::Size(3, 3), 2, 2);
    return dehazed;
}
CaptureFrame Algorithm::hist_equalize(CaptureFrame object) //CLAHE based basic dehazing algorithm
{
    cv::Mat image_hsv;
    cvtColor(object.retrieve_image(), image_hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> channels;
    split(image_hsv, channels);
    cv::equalizeHist(channels[1], channels[1]);
    channels[2] = channels[2] * 0.85;
    merge(channels, image_hsv);
    cv::Mat dehazed;
    cvtColor(image_hsv, dehazed, cv::COLOR_HSV2BGR);
    // GaussianBlur(dehazed, dehazed, cv::Size(3, 3), 2, 2);
    CaptureFrame output(dehazed, "Dehazed image");
    return output;
}

void Algorithm::set_CLAHE_clip_limit(int clip_limit)//function which set the clip limit in CLAHE dehazing
{
    CLAHE_clip_limit = clip_limit;
}
Algorithm::Algorithm()
{
    inlier_threshold = 2.5f; 
    nn_match_ratio = 0.8f; 
    CLAHE_clip_limit = 2;
}

CaptureFrame Algorithm::dark_channel(CaptureFrame input_image, int radius)
{
    cv::Mat temp = input_image.retrieve_image().clone();
    cv::Mat dark = cv::Mat::zeros(temp.size(),CV_8UC1);
    cv::GaussianBlur(temp,temp,cv::Size(5,5),1,1,4);
    int width = temp.cols;
    int height = temp.rows;
    int min_color = 0;
    int st_row,st_col,ed_row,ed_col;
    uchar* ImagePtr;
    for(int row = 0 ; row < height ; row++)
    {
        for(int col = 0 ; col < width ; col++)
        {
            st_row = row - radius; ed_row = row + radius;
            st_col = col - radius; ed_col = col + radius;
            st_row = (st_row < 0) ? 0 : st_row;
            ed_row = (ed_row < width) ? ed_row : width - 1;
            st_col = (st_col < 0) ? 0 : st_col;
            ed_col = (ed_col < width) ? ed_col : width - 1;
            int cur = 0;
            int min = 300;
            for(int patch_row = st_row;patch_row < ed_row ; patch_row++)
            {
                for(int patch_col = st_col;patch_col < ed_col ; patch_col++)
                {
                    cv::Vec3b intensity = temp.at<cv::Vec3b>(patch_row,patch_col);
                    int blue = (int) intensity.val[0];
                    int green = (int) intensity.val[1];
                    int red = (int) intensity.val[2];
                    cur = std::min(blue,std::min(green,red));
                    if(cur<min) min = cur;

                }
            }
            dark.at<uchar>(row, col) = min;
        }
    }
    temp.release();
    // cv::equalizeHist(dark,dark);
    CaptureFrame output(dark,"DCP");
    return output;
}

CaptureFrame Algorithm::u_dark_channel(CaptureFrame input_image, int radius)
{
    cv::Mat temp = input_image.retrieve_image().clone();
    cv::Mat dark = cv::Mat::zeros(temp.size(),CV_8UC1);
    cv::GaussianBlur(temp,temp,cv::Size(5,5),1,1,4);
    int width = temp.cols;
    int height = temp.rows;
    int min_color = 0;
    int st_row,st_col,ed_row,ed_col;
    uchar* ImagePtr;
    for(int row = 0 ; row < height ; row++)
    {
        for(int col = 0 ; col < width ; col++)
        {
            st_row = row - radius; ed_row = row + radius;
            st_col = col - radius; ed_col = col + radius;
            st_row = (st_row < 0) ? 0 : st_row;
            ed_row = (ed_row < width) ? ed_row : width - 1;
            st_col = (st_col < 0) ? 0 : st_col;
            ed_col = (ed_col < width) ? ed_col : width - 1;
            int cur = 0;
            int min = 300;
            for(int patch_row = st_row;patch_row < ed_row ; patch_row++)
            {
                for(int patch_col = st_col;patch_col < ed_col ; patch_col++)
                {
                    cv::Vec3b intensity = temp.at<cv::Vec3b>(patch_row,patch_col);
                    int blue = (int) intensity.val[0];
                    int green = (int) intensity.val[1];
                    // int red = (int) intensity.val[2];
                    cur = std::min(green,blue);
                    if(cur<min) min = cur;

                }
            }
            dark.at<uchar>(row, col) = min;
        }
    }
    temp.release();
    // cv::equalizeHist(dark,dark);
    CaptureFrame output(dark,"UDCP");
    return output;
}

CaptureFrame Algorithm::saturation_map(CaptureFrame input_image, int radius)
{
    cv::Mat temp = input_image.retrieve_image().clone();
    cv::Mat HLS;
    cv::cvtColor(temp,HLS,CV_BGR2HLS);
    std::vector<cv::Mat> channels;
    split(HLS, channels);
    // cv::equalizeHist(channels[2],channels[2]);
    CaptureFrame output(channels[2],"saturation map");
    return output;
}

//Akaze feature points identification
void Algorithm::AKAZE_feature_points(CaptureFrame image1, CaptureFrame image2)
{
    
    inlier_threshold = 2.5f; // Distance threshold to identify inliers
    
    //Input images for identifying the keypoints
    current_image = image1.retrieve_image().clone();
    previous_image = image2.retrieve_image().clone();

    //Creating akaze object
    cv::Ptr<cv::AKAZE> akaze = cv::AKAZE::create();

    //Detecting keypoints in the images
    akaze->detectAndCompute(current_image, cv::noArray(), keypoints_current_image, description_current_image);
    akaze->detectAndCompute(previous_image, cv::noArray(), keypoints_previous_image, description_previous_image);
    
    //logging
    logger.log_info("Akaze feature point idenfication");
    return;

}

//Orb feature points identification
void Algorithm::ORB_feature_points(CaptureFrame image1,CaptureFrame image2)
{
    //Creating ORB object
    cv::Ptr<cv::ORB> orb = cv::ORB::create();

    //Input images
    current_image = image1.retrieve_image();
    previous_image = image2.retrieve_image();

    //Detecting feature points through ORB
    orb->detectAndCompute(current_image, cv::noArray(), keypoints_current_image, description_current_image);
    orb->detectAndCompute(previous_image, cv::noArray(), keypoints_previous_image, description_previous_image);
    
    //logging
    logger.log_info("ORB feature point idenfication");
    return;
}

//Matches the images in which keypoints are identified already
void Algorithm::BF_matcher()
{  
    //Clearing previous data
    matched_current_image.clear();
    matched_previous_image.clear();

    //Creating BruteForce matcher object
    cv::BFMatcher matcher(cv::NORM_HAMMING);

    //Matching
    std::vector< std::vector<cv::DMatch> > nn_matches;
    matcher.knnMatch(description_current_image, description_previous_image, nn_matches, 2);
    for(size_t i = 0; i < nn_matches.size(); i++) 
    {
        cv::DMatch first = nn_matches[i][0];
        float dist1 = nn_matches[i][0].distance;
        float dist2 = nn_matches[i][1].distance;
        if(dist1 < nn_match_ratio * dist2) 
        {
            matched_current_image.push_back(keypoints_current_image[first.queryIdx].pt);
            matched_previous_image.push_back(keypoints_previous_image[first.trainIdx].pt);
        }
    }

    //logging
    logger.log_info("Brute Force matching");
    return;
}

CaptureFrame Algorithm::red_irradiance(CaptureFrame input_image,int radius)
{
    cv::Mat temp = input_image.retrieve_image().clone();
    std::vector<cv::Mat> channels;
    cv::split(temp,channels);
    cv::Mat red_channel = channels[2].clone();
    // cv::imshow("red channel",red_channel);
    // cv::waitKey(0)
    CaptureFrame output(red_channel,"depth map");
    return output;

}