
//Including standard headers
#include <opencv2/opencv.hpp>
#include <math.h>
//Inlcuding user defined headers
#include "algorithm.h"
#include "logger.h"
#include "capture_frame.h"

CaptureFrame Algorithm::CLAHE_dehaze(CaptureFrame object) //CLAHE based basic dehazing algorithm
{
    cv::Mat segmented;
    segmented = CLAHE_dehaze(object.retrieve_image().clone());
    CaptureFrame output(segmented, "Dehazed image");
    return output;
}

CaptureFrame Algorithm::CLAHE_dehaze_shallow(CaptureFrame object) //CLAHE based basic dehazing algorithm
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
    cvtColor(object.retrieve_image().clone(), image_hsv, cv::COLOR_BGR2HSV);
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

CaptureFrame Algorithm::balance_white(CaptureFrame input_image)
{
    cv::Mat white_bal = input_image.retrieve_image().clone();
    balance_white(white_bal);
    CaptureFrame output(white_bal,"White Balanced Image");
    return output;

}

CaptureFrame Algorithm::balance_white_shallow(CaptureFrame input_image)
{
    cv::Mat white_bal = input_image.retrieve_image();
    balance_white(white_bal);
    CaptureFrame output(white_bal,"White Balanced Image");
    return output;

}

void Algorithm::balance_white(cv::Mat mat) //MIT code
{
    double discard_ratio = 0.05;
    int hists[3][256];
    memset(hists, 0, 3 * 256 * sizeof(int));

    for (int y = 0; y < mat.rows; ++y)
    {
        uchar *ptr = mat.ptr<uchar>(y);
        for (int x = 0; x < mat.cols; ++x)
        {
            for (int j = 0; j < 3; ++j)
            {
                hists[j][ptr[x * 3 + j]] += 1;
            }
        }
    }

    // cumulative hist
    int total = mat.cols * mat.rows;
    int vmin[3], vmax[3];
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 255; ++j)
        {
            hists[i][j + 1] += hists[i][j];
        }
        vmin[i] = 0;
        vmax[i] = 255;
        while (hists[i][vmin[i]] < discard_ratio * total)
            vmin[i] += 1;
        while (hists[i][vmax[i]] > (1 - discard_ratio) * total)
            vmax[i] -= 1;
        if (vmax[i] < 255 - 1)
            vmax[i] += 1;
    }

    for (int y = 0; y < mat.rows; ++y)
    {
        uchar *ptr = mat.ptr<uchar>(y);
        for (int x = 0; x < mat.cols; ++x)
        {
            for (int j = 0; j < 3; ++j)
            {
                int val = ptr[x * 3 + j];
                if (val < vmin[j])
                    val = vmin[j];
                if (val > vmax[j])
                    val = vmax[j];
                ptr[x * 3 + j] = static_cast<uchar>((val - vmin[j]) * 255.0 / (vmax[j] - vmin[j]));
            }
        }
    }
}

CaptureFrame Algorithm::laplacian_contrast(CaptureFrame input_image)
{
    cv::Mat temp = input_image.retrieve_image().clone();
    cv::Mat laplacian;
    
    cv::Mat greyscale_image(temp.rows,temp.cols,CV_8UC1);
    cv::cvtColor( temp, greyscale_image, CV_BGR2GRAY );

    cv::Laplacian(temp,laplacian,CV_16S);
    
    // cv::imshow("laplacian contrast",laplacian);
    // cv::waitKey(0);
    // cv::convertScaleAbs(laplacian,laplacian);
    cv::Mat out(laplacian.rows,laplacian.cols,CV_32FC1);
    std::vector<cv::Mat> channels;
    cv::split(laplacian,channels);
    out = channels[2];
    // std::cout<<out.type();
    out.convertTo(out,CV_8UC1);
    
    CaptureFrame output(out,"Laplacian Constrast");
    return output;
}

CaptureFrame Algorithm::local_contrast(CaptureFrame input_image)
{
    const float pi = 3.141592;
    cv::Mat temp = input_image.retrieve_image().clone();
    cv::Mat greyscale_image(temp.rows,temp.cols,CV_8UC1);
    cv::cvtColor( temp, greyscale_image, CV_BGR2GRAY );
    float h[] = {1.0/16.0 , 4.0/16.0 , 6.0/16.0 , 4.0/16.0 , 1.0/16.0 };
    int length = sizeof(h)/sizeof(*h);
    cv::Mat mask(length,length,CV_32FC1);
    for(int i = 0 ; i < length ; i++)
    {
        for(int j = 0 ; j < length ; j++)
        {
            mask.at<float>(i,j) = h[i] * h[j];
            // std::cout<<h[i]<<"\t"<<h[j]<<"\n";
        }        
    }
    // std::cout<<mask;
    // std::cout<<length;
    cv::Mat local_contrast_image;
    greyscale_image.convertTo(greyscale_image,CV_32FC1);
    
    cv::filter2D(greyscale_image,local_contrast_image,mask.depth(),mask);
    for(int i = 0; i < local_contrast_image.rows ; i++)
    {
        for(int j = 0; j < local_contrast_image.cols ; j++)
        {
            if(local_contrast_image.at<float>(i,j) < pi/2.75)
            local_contrast_image.at<float>(i,j) = pi/2.75;
        }
    }
    // cv::subtract(greyscale_image,local_contrast_image,local_contrast_image);
    local_contrast_image.convertTo(local_contrast_image,CV_8UC1);
    CaptureFrame output(local_contrast_image,"Local Contrast");
    return output;
}

CaptureFrame Algorithm::saliency_contrast(CaptureFrame input_image)
{
    cv::Mat temp = input_image.retrieve_image().clone();
    cv::Mat lab_image,l_channel,a_channel,b_channel;
    
    cv::cvtColor(temp,lab_image,CV_BGR2Lab);
    std::vector<cv::Mat> channels;
    cv::split(lab_image,channels);
    channels[0].convertTo(l_channel,CV_32FC1);
    channels[1].convertTo(a_channel,CV_32FC1);
    channels[2].convertTo(b_channel,CV_32FC1);
    
    double l_mean = cv::mean(l_channel).val[0];
    double a_mean = cv::mean(a_channel).val[0];
    double b_mean = cv::mean(b_channel).val[0];

    cv::Mat saliency_map = cv::Mat::zeros(l_channel.rows,l_channel.cols,l_channel.type());
    cv::subtract(l_channel,cv::Scalar(l_mean),l_channel);
    cv::subtract(a_channel,cv::Scalar(a_mean),a_channel);
    cv::subtract(b_channel,cv::Scalar(b_mean),b_channel);

    cv::add(saliency_map,l_channel.mul(l_channel),saliency_map);
    cv::add(saliency_map,a_channel.mul(a_channel),saliency_map);
    cv::add(saliency_map,b_channel.mul(b_channel),saliency_map);

    saliency_map.convertTo(saliency_map,CV_8UC1);
    CaptureFrame output(saliency_map,"Saliency Weight");
    return output;

}

CaptureFrame Algorithm::exposedness(CaptureFrame input_image)
{
    double average = 0.5;
    cv::Mat temp = input_image.retrieve_image().clone();
    cv::Mat greyscale_image(temp.rows,temp.cols,CV_8UC1);
    cv::cvtColor( temp, greyscale_image, CV_BGR2GRAY );
    // cv::imshow("greysnksn",greyscale_image);
    // cv::waitKey(0);
    greyscale_image.convertTo(greyscale_image,CV_32FC1,1.0/255.0,0);
    // cv::imshow("greysnksn",greyscale_image);
    // cv::waitKey(0);
    double data = 0;
    cv::Mat expo = cv::Mat::zeros(greyscale_image.rows,greyscale_image.cols,CV_32FC1);
    for(int i = 0 ; i < expo.rows; i++)
    {
        for(int j = 0 ; j < expo.cols; j++)
        {
            data = std::exp(std::pow(-1.0 * greyscale_image.at<float>(i,j) - average ,2.0)/(2 * 0.0625));
            expo.at<float>(i,j) = data;
        }
    }
    

    // cv::Mat temp1,temp2,temp3;
    // cv::add(greyscale_image,cv::Scalar(average),temp1);
    // cv::pow(temp1,2,temp2);    
    // cv::divide(temp2,cv::Scalar(0.125),temp3);
    // cv::exp(temp3,temp3);
    // temp1.convertTo(temp1,CV_32FC1,1.0/255.0,0);
    // temp2.convertTo(temp2,CV_32FC1,1.0/255.0,0);
    // temp3.convertTo(temp3,CV_32FC1,1.0/255.0,0);
    // cv::imshow("thenga",temp1);
    // cv::waitKey(0);
    // cv::imshow("kola",temp2);
    // cv::waitKey(0);
    // cv::imshow("managa",temp3);
    // cv::waitKey(0);

    // expo.convertTo(expo,CV_32FC1,1.0/255.0,0);
    expo.convertTo(expo,CV_8UC1);
    CaptureFrame output(expo,"Exposedness");
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
    current_image = image1.retrieve_image().clone();
    previous_image = image2.retrieve_image().clone();

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
    cv::blur(red_channel,red_channel,cv::Size(5,5));
    cv::Mat color_map;
    cv::equalizeHist(red_channel,red_channel);
    cv::applyColorMap(red_channel,color_map, cv::COLORMAP_JET);
    CaptureFrame output(color_map,"depth map");
    return output;

}

std::vector<cv::Mat> Algorithm::gaussion_pyrdown(CaptureFrame input, int levels)
{
    cv::Mat temp = input.retrieve_image().clone();
    cv::Mat pyr_image;
    std::vector<cv::Mat> pyramids;
    pyramids.push_back(temp);
    for(int i = 1; i < levels ; i++)
    {
        cv::pyrDown(temp,pyr_image,cv::Size(temp.cols/2,temp.rows/2));
        pyramids.push_back(pyr_image);
        temp = pyr_image.clone();
        // cv::imshow("pyramid",temp);
        // cv::waitKey(0);
    }
    return pyramids;
}

std::vector<cv::Mat> Algorithm::laplacian_pyrdown(CaptureFrame input, int levels)
{
    cv::Mat temp = input.retrieve_image().clone();
    
    std::vector<cv::Mat> pyramids;
    pyramids = laplacian_pyrdown(temp,levels);

    return pyramids;
}

std::vector<cv::Mat> Algorithm::laplacian_pyrdown(cv::Mat temp, int levels)
{
    cv::Mat pyr_image;
    std::vector<cv::Mat> pyramids;
    pyramids.push_back(temp);
    for(int i = 1; i < levels ; i++)
    {
        cv::pyrDown(temp,pyr_image,cv::Size(temp.cols/2,temp.rows/2));
        pyramids.push_back(pyr_image);
        temp = pyr_image.clone();
        // cv::imshow("pyramid",temp);
        // cv::waitKey(0);
    }
    for(int i = 0; i < levels - 1 ; i++)
    {
        cv::Mat tmpyr;
        cv::pyrUp(pyramids[i + 1],tmpyr,pyramids[i].size());
        cv::subtract(pyramids[i],tmpyr,pyramids[i]);
        // cv::imshow("pyramid",pyramids[i]);
        // cv::waitKey(0);
    }

    return pyramids;
}

CaptureFrame Algorithm::pyr_reconstruct(std::vector<cv::Mat> pyramids,int levels)
{
    // int levels = pyramids.length();
    for(int i = levels-1; i > 0; i--)
    {
        cv::Mat tempyr;
        cv::pyrUp(pyramids[i],tempyr,pyramids[i-1].size());
        cv::add(pyramids[i - 1],tempyr,pyramids[i - 1]);
    }
    // cv::imshow("reconpyr",pyramids[0]);
    // cv::waitKey(0);
    CaptureFrame output(pyramids[0],"Reconstructed pyramid");

    return output;
}
