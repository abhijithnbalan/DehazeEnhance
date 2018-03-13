
#include "dehaze_enhance.h"
#include "capture_frame.h"
#include "logger.h"
#include "view_frame.h"
#include <opencv2/opencv.hpp>

void DehazeEnhance::application(CaptureFrame input)
{
    original_image = input;
    CaptureFrame depth_map;
    // en_HE = algo.hist_equalize(input);
    en_CLAHE = algo.CLAHE_dehaze(input);
    logger.log_warn("CLAHE");
    // depth_map = algo.red_irradiance(input,4);
    // dark_channel = algo.dark_channel(input,5);
    // saturation = algo.saturation_map(input,5);
    // u_darkchannel = algo.u_dark_channel(input,5);
    dark_channel = find_airlight(input, 5);
    // dark_chnnel = find_airlight(en_CLAHE,5);
    show_airlight(input);
    find_transmission(input);
    recover_image(input);
    // viewer.multiple_view_interrupted(original_image,saturation,dark_channel,u_darkchannel,50);
    // viewer.multiple_view_interrupted(en_CLAHE,dark_channel,airlight_image,saturation,50);

    // viewer.single_view_interrupted(dark_channel,50);
    return;
}

void DehazeEnhance::show_airlight(CaptureFrame input_image)
{
    if (max_intensity_point.x + max_intensity_point.y == 0)
    {
        logger.log_warn("Airlight is not identified before to show");
        return;
    }
    cv::Mat temp = input_image.retrieve_image().clone();
    cv::rectangle(temp, cv::Point(max_intensity_point.x + 5, max_intensity_point.y + 5), cv::Point(max_intensity_point.x - 5, max_intensity_point.y - 5), cv::Scalar(0, 0, 255), 1, 8, 0);

    airlight_image.reload_image(temp, "Airlight");
    cv::imshow("Airlight", temp);
    if (cv::waitKey(0) > 0)
        return;
}

int DehazeEnhance::find_airlight(CaptureFrame dark_channel, CaptureFrame saturaiton)
{
    cv::MatND hist, norm_hist;
    cv::Mat temp = dark_channel.retrieve_image().clone();
    /// Establish the number of bins
    int histSize = 256;

    /// Set the ranges ( for B,G,R) )
    float range[] = {0, 256};
    const float *histRange = {range};

    bool uniform = true;
    bool accumulate = false;

    cv::calcHist(&temp, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);
    cv::normalize(hist, norm_hist, 0, hist.rows, 32, -1, cv::Mat());
    cv::imshow("histogram", norm_hist);
    cv::waitKey(0);
    return 1;
}

CaptureFrame DehazeEnhance::find_airlight(CaptureFrame input_image, int radius)
{
    cv::Mat temp = input_image.retrieve_image().clone();
    cv::Mat dark = cv::Mat::zeros(temp.size(), CV_8UC1);
    cv::GaussianBlur(temp, temp, cv::Size(5, 5), 1, 1, 4);
    int width = temp.cols;
    int height = temp.rows;
    int min_background_light = 0;
    int st_row, st_col, ed_row, ed_col;
    uchar *ImagePtr;
    int max_intensity = 0;
    max_intensity_point = cv::Point(0, 0);
    for (int row = 0; row < height; row++)
    {
        for (int col = 0; col < width; col++)
        {
            st_row = row - radius;
            ed_row = row + radius;
            st_col = col - radius;
            ed_col = col + radius;
            st_row = (st_row < 0) ? 0 : st_row;
            ed_row = (ed_row < width) ? ed_row : width - 1;
            st_col = (st_col < 0) ? 0 : st_col;
            ed_col = (ed_col < width) ? ed_col : width - 1;
            int cur = 0;
            int min = 300;
            for (int patch_row = st_row; patch_row < ed_row; patch_row++)
            {
                for (int patch_col = st_col; patch_col < ed_col; patch_col++)
                {
                    cv::Vec3b intensity = temp.at<cv::Vec3b>(patch_row, patch_col);
                    int blue = (int)intensity.val[0];
                    int green = (int)intensity.val[1];
                    int red = (int)intensity.val[2];
                    cur = std::min(blue, std::min(green, red));
                    if (cur < min)
                        min = cur;
                }
            }
            dark.at<uchar>(row, col) = min;
            if (max_intensity < min)
            {
                max_intensity = min;
                max_intensity_point.y = row;
                max_intensity_point.x = col;
            }
        }
    }
    background_light = temp.at<cv::Vec3b>(max_intensity_point.x, max_intensity_point.y);
    std::cout << (int)background_light.val[0] << "\n"
              << (int)background_light.val[1] << "\n"
              << (int)background_light.val[2] << "\n";
    temp.release();
    CaptureFrame output(dark, "DarkChannel");
    return output;
}

void DehazeEnhance::find_transmission(CaptureFrame input_image)
{
    cv::Mat temp = dark_channel.retrieve_image().clone();
    temp.convertTo(temp, CV_32FC1);
    cv::Mat unity(temp.rows, temp.cols, CV_32FC1, cv::Scalar(1));
    // std::cout << temp.rows << "\n"
    //           << temp.cols << "\n"
    //           << unity.rows << "\n"
    //           << unity.cols << "\n";
    cv::Mat red_tra = unity - temp / ((int)background_light.val[2]);
    //    equalizeHist(red_tra,red_tra);
    int attenuation_coefficient_blue = 1.20777 * ((int)background_light.val[2]) / ((int)background_light.val[0]);
    int attenuation_coefficient_green = 1.09777 * ((int)background_light.val[2]) / ((int)background_light.val[1]);
    cv::Mat blue_tra;
    cv::Mat green_tra;
    cv::pow(red_tra, attenuation_coefficient_blue, blue_tra);
    cv::pow(red_tra, attenuation_coefficient_green, green_tra);
    // red_tra.convertTo(red_tra, CV_8UC1);
    // blue_tra.convertTo(blue_tra, CV_8UC1);
    // green_tra.convertTo(green_tra, CV_8UC1);
    red_transmission.reload_image(red_tra, "Transmisison for red channel");
    blue_transmission.reload_image(blue_tra, "Transmisison for blue channel");
    green_transmission.reload_image(green_tra, "Transmisison for green channel");
    //    std::cout<<red_tra.type()<<"\n"<<blue_tra.type()<<"\n"<<green_tra.type()<<"\n";
       cv::imshow("red tra",blue_tra);
       cv::waitKey(0);
    double min, max;
    cv::minMaxLoc(red_tra, &min, &max);
    std::cout<<min<<"\n"<<max<<"\n";

    return;
}

CaptureFrame DehazeEnhance::recover_image(CaptureFrame input_image)
{
    cv::Mat temp,airlight_sub,transmission_div,recovered,tot_transmission;
    temp = input_image.retrieve_image().clone();
    recovered = temp.clone();
    std::vector<cv::Mat> ch;
    
    ch.push_back(red_transmission.retrieve_image().clone());
    ch.push_back(green_transmission.retrieve_image().clone());
    ch.push_back(blue_transmission.retrieve_image().clone());
    cv::merge(ch,tot_transmission);
    
    // cv::subtract(temp,cv::Scalar((int)background_light.val[0],(int)background_light.val[1],(int)background_light.val[2]),airlight_sub);
    // cv::imshow("air",airlight_sub);
    // cv::waitKey(0);
    // airlight_sub.convertTo(airlight_sub, CV_32FC3);
    // cv::divide(airlight_sub,tot_transmission,transmission_div,-1);
    // transmission_div.convertTo(transmission_div, CV_8UC3);
    // cv::imshow("div",tot_transmission);
    // // cv::imshow("div",transmission_div);
    // cv::waitKey(0);
    // cv::add(transmission_div,cv::Scalar((int)background_light.val[0],(int)background_light.val[1],(int)background_light.val[2]),recovered);
    
    std::vector<cv::Mat> reco_channels,channels;
    cv::split(recovered,reco_channels);
    cv::split(temp,channels);
    for(int row = 0;row < temp.rows-2;row++)
    {
        for(int col = 0;col < temp.cols-2;col++)
        {
            for(int color = 0;color<3;color++)
            { 
                
                reco_channels[color].at<uchar>(row,col) = (channels[color].at<uchar>(row,col) - (int)background_light[color])/ch[color].at<float>(row,col) + (int)background_light[color];
            }
        }
    }
    cv::merge(reco_channels,recovered);
    CaptureFrame output(recovered,"The recovered image");
    cv::imshow("Recovered",recovered);
    cv::waitKey(0);
    return output;
}