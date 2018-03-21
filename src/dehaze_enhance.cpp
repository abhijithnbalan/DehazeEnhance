
#include "dehaze_enhance.h"
#include "capture_frame.h"
#include "logger.h"
#include "view_frame.h"
#include <opencv2/opencv.hpp>

void DehazeEnhance::dark_channel_prior(CaptureFrame input)
{
    original_image = input;
    CaptureFrame depth_map;
    // en_HE = algo.hist_equalize(input);
    en_CLAHE = algo.CLAHE_dehaze(input);
    // logger.log_warn("CLAHE");
    depth_map = algo.red_irradiance(input,4);
    // dark_channel = algo.dark_channel(input,5);
    saturation = algo.saturation_map(input,5);
    // u_darkchannel = algo.u_dark_channel(input,5);
    dark_channel = find_airlight(input, 5);
    // int t = find_airlight(dark_channel,saturation);
    // dark_chnnel = find_airlight(en_CLAHE,5);
    airlight_image = show_airlight(input);
    logger.log_warn("viewing");
    // viewer.multiple_view_interrupted(input,dark_channel,saturation,depth_map,50);
    // viewer.multiple_view_interrupted(airlight_image,airlight_color,75);
    find_transmission(input);
    recovered_image = recover_image(input);
    viewer.multiple_view_interrupted(original_image,dark_channel,recovered_image,en_CLAHE,50);
    // viewer.multiple_view_interrupted(en_CLAHE,dark_channel,airlight_image,saturation,50);

    // viewer.single_view_interrupted(recovered_image,50);
    return;
}

CaptureFrame DehazeEnhance::show_airlight(CaptureFrame input_image)
{
    if (max_intensity_point.x + max_intensity_point.y == 0)
    {
        logger.log_warn("Airlight is not identified before to show");
        return input_image;
    }
    cv::Mat temp = input_image.retrieve_image().clone();
    cv::rectangle(temp, cv::Point(max_intensity_point.x + 5, max_intensity_point.y + 5), cv::Point(max_intensity_point.x - 5, max_intensity_point.y - 5), cv::Scalar(0, 0, 255), 2, 8, 0);
    CaptureFrame output(temp,"airlight");
    return output;
}

int DehazeEnhance::find_airlight(CaptureFrame dark_channel, CaptureFrame saturaiton)
{
    cv::MatND hist, norm_hist;
    cv::Mat temp_dark = dark_channel.retrieve_image().clone();
    cv::Mat temp_satu = dark_channel.retrieve_image().clone();
    cv::Mat filtered_satu(temp_satu.rows,temp_satu.cols,CV_8UC1);
    cv::Mat filtered_dark(temp_dark.rows,temp_dark.cols,CV_8UC1);
    cv::Mat filtered_common(temp_dark.rows,temp_dark.cols,CV_8UC1);
    /// Establish the number of bins
    // int histSize = 256;

    /// Set the ranges ( for B,G,R) )
    // float range[] = {0, 256};
    // const float *histRange = {range};

    // bool uniform = true;
    // bool accumulate = false;

    // cv::calcHist(&temp, 1, 0, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);
    // cv::normalize(hist, norm_hist, 0, hist.rows, 32, -1, cv::Mat());
    cv::equalizeHist(temp_satu,temp_satu);
    cv::equalizeHist(temp_dark,temp_dark);
    cv::inRange(temp_satu,cv::Scalar(250),cv::Scalar(255),filtered_satu);
    cv::inRange(temp_dark,cv::Scalar(250),cv::Scalar(255),filtered_dark);
    cv::bitwise_and(filtered_satu,filtered_dark,filtered_common);
    cv::imshow("filter", filtered_common);
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
    background_light = temp.at<cv::Vec3b>(max_intensity_point.y, max_intensity_point.x);
    std::cout << (int)background_light.val[0] << "\n"
              << (int)background_light.val[1] << "\n"
              << (int)background_light.val[2] << "\n";
    
    background_color = cv::Scalar((float)background_light.val[0],(float)background_light.val[1],(float)background_light.val[2]);
    cv::Mat temp_color(temp.rows,temp.cols,CV_32FC3,background_color);
    airlight_color.reload_image(temp_color,"Background color");



    temp.release();
    CaptureFrame output(dark, "DarkChannel");
    return output;
}

void DehazeEnhance::find_transmission(CaptureFrame input_image)
{
    cv::Mat temp = dark_channel.retrieve_image().clone();
    // std::vector<cv::Mat> channels;
    // cv::split(temp_c,channels);
    // cv::Mat temp = channels[2].clone();
    temp.convertTo(temp, CV_32FC1);
    cv::Mat unity(temp.rows, temp.cols, CV_32FC1, cv::Scalar(1));
    cv::Mat temp_recovered = temp/(float)background_light.val[2];
    // temp_recovered.convertTo(temp, CV_8UC1);
    double min, max;
    // cv::imshow("reduced image",temp_recovered);
    // cv::waitKey(0);
    cv::minMaxLoc(temp_recovered, &min, &max);
    std::cout<<min<<"\n"<<max<<"\n";
    cv::Mat red_tra = unity - temp_recovered;
    cv::minMaxLoc(red_tra, &min, &max);
    std::cout<<min<<"\n"<<max<<"\n";

    // cv::Mat for_view = red_tra * 255;
    // cv::imshow("sdfadsf",for_view);
    // cv::waitKey(0);

    
    // CaptureFrame dark_pic = algo.dark_channel(transmission_airlight,5);

    // std::cout << temp.rows << "\n"
    //           << temp.cols << "\n"
    //           << unity.rows << "\n"
    //           << unity.cols << "\n"
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
    // cv::minMaxLoc(red_tra, &min, &max);
    // std::cout<<min<<"\n"<<max<<"\n";
    logger.log_warn("transmission successful");
    return;
}

CaptureFrame DehazeEnhance::recover_image(CaptureFrame input_image)
{
    cv::Mat temp,transmission_div,recovered,tot_transmission;
    temp = input_image.retrieve_image().clone();
     double min, max;
    std::vector<cv::Mat> ch;
    cv::Mat airlight_sub(temp.rows,temp.cols,CV_32FC3,cv::Scalar(0,0,0));
    cv::Mat tempooo,airlight;
    airlight = airlight_color.retrieve_image();
    airlight.convertTo(tempooo,CV_8UC3);
    // cv::imshow("air color",tempooo);
    recovered = temp.clone();
    // cv::waitKey(0);
    temp.convertTo(temp, CV_32FC3);
    ch.push_back(blue_transmission.retrieve_image());
    
    ch.push_back(green_transmission.retrieve_image());
    ch.push_back(red_transmission.retrieve_image());
    cv::merge(ch,tot_transmission);

    airlight_sub = temp - airlight;

    // cv::imshow("air",airlight_sub);
    // cv::waitKey(0);
    cv::Mat toview = airlight_sub+airlight;
    // toview.convertTo(toview,CV_32FC3);
    // cv::imshow("ckeck",toview);
    // cv::waitKey(0);
    // airlight_sub.convertTo(airlight_sub, CV_32FC3);
    // cv::divide(airlight_sub,tot_transmission,transmission_div,-1);
    std::vector<cv::Mat> reco_channels,channels;
    cv::split(recovered,reco_channels);
    cv::split(airlight_sub,channels);
    cv::Mat blue_trans;
    cv::Mat red_trans;
    cv::Mat green_trans;
    cv::Mat max_transmission(blue_transmission.retrieve_image().rows,blue_transmission.retrieve_image().cols,CV_32FC1,cv::Scalar(0.1));
    
    cv::max(blue_transmission.retrieve_image(),max_transmission,blue_trans);
    cv::max(green_transmission.retrieve_image(),max_transmission,green_trans);
    cv::max(red_transmission.retrieve_image(),max_transmission,red_trans);
    
    reco_channels[0] = 0.9 * channels[0]/blue_trans;
    reco_channels[1] = 0.9 * channels[1]/green_trans;
    reco_channels[2] = 0.9 * channels[2]/red_trans;
    cv::GaussianBlur(reco_channels[0],reco_channels[0],cv::Size(5,5), 0, 0, cv::BORDER_DEFAULT);
    cv::GaussianBlur(reco_channels[1],reco_channels[1],cv::Size(5,5), 0, 0, cv::BORDER_DEFAULT);
    cv::GaussianBlur(reco_channels[2],reco_channels[2],cv::Size(5,5), 0, 0, cv::BORDER_DEFAULT);
    cv::minMaxLoc(channels[0], &min, &max);
    std::cout<<min<<"\n"<<max<<"\n";
    cv::merge(reco_channels,transmission_div);
    // transmission_div.convertTo(transmission_div, CV_8UC3);
    // cv::imshow("div",tot_transmission);
    // cv::imshow("div",transmission_div);
    // cv::waitKey(0);
    recovered = transmission_div + airlight;  
    
    recovered.convertTo(recovered, CV_8UC3);
    // // recovered = algo.CLAHE_dehaze(recovered);
    // std::vector<cv::Mat> reco_channels,channels;
    // cv::split(recovered,reco_channels);
    // cv::split(temp,channels);
    // for(int row = 0;row < temp.rows;row++)
    // {
    //     for(int col = 0;col < temp.cols;col++)
    //     {
    //         for(int color = 0;color<3;color++)
    //         { 
                
    //             reco_channels[color].at<uchar>(row,col) = (channels[color].at<uchar>(row,col) - (int)background_light[color])/ch[color].at<float>(row,col) + (int)background_light[color];
    //         }
    //     }
    // }
    // cv::merge(reco_channels,recovered);
    CaptureFrame output(recovered,"The recovered image");
    // cv::imshow("Recovered",recovered);
    // cv::waitKey(0);
    return output;
}
void DehazeEnhance::fusion(CaptureFrame input)
{
    CaptureFrame original(input.retrieve_image().clone(),"original input");
    // balanceWhite(input.retrieve_image(),white_bal,WHITE_BALANCE_SIMPLE);
    white_balanced_image = algo.balance_white(input);
    en_CLAHE = algo.CLAHE_dehaze(original);
    // en_HE = algo.hist_equalize(original);
    logger.log_warn("contrasts estimation");
    laplacian_contrast_1 = algo.laplacian_contrast(white_balanced_image);
    laplacian_contrast_2 = algo.laplacian_contrast(en_CLAHE);
    local_contrast_1 = algo.local_contrast(white_balanced_image);
    local_contrast_2 = algo.local_contrast(en_CLAHE);
    saliency_contrast_1 = algo.saliency_contrast(white_balanced_image);
    saliency_contrast_2 = algo.saliency_contrast(en_CLAHE);
    exposedness_1 = algo.exposedness(white_balanced_image);
    exposedness_2 = algo.exposedness(en_CLAHE);
    viewer.multiple_view_interrupted(en_CLAHE,white_balanced_image,50);
    viewer.multiple_view_interrupted(laplacian_contrast_1,local_contrast_1,saliency_contrast_1,exposedness_1,50);
    viewer.multiple_view_interrupted(laplacian_contrast_2,local_contrast_2,saliency_contrast_2,exposedness_2,50);
    logger.log_warn("normalize weights");
    normalize_weights();
    
    logger.log_warn("blending together");

    CaptureFrame final_image = pyramid_fusion();
    // CaptureFrame final_image = fusion_blender();

    // viewer.multiple_view_interrupted(laplace_fusion,local_fusion,saliency_fusion,exposedness_fusion,50);
    
    viewer.multiple_view_interrupted(original,final_image,50);
    // std::vector<cv::Mat> image = algo.gaussion_pyrdown(en_CLAHE,5);

    // std::vector<cv::Mat> imag = algo.laplacian_pyrdown(en_CLAHE,5);
    
    return;
}
void DehazeEnhance::normalize_weights()
{
    
    cv::Mat laplace_1 = laplacian_contrast_1.retrieve_image().clone();
    cv::Mat laplace_2 = laplacian_contrast_2.retrieve_image().clone();
    cv::Mat local_1 = local_contrast_1.retrieve_image().clone();
    cv::Mat local_2 = local_contrast_2.retrieve_image().clone();
    cv::Mat saliency_1 = saliency_contrast_1.retrieve_image().clone();
    cv::Mat saliency_2 = saliency_contrast_2.retrieve_image().clone();
    cv::Mat expose_1 = exposedness_1.retrieve_image().clone();
    cv::Mat expose_2 = exposedness_2.retrieve_image().clone();

    cv::Mat laplace_normal_1(laplace_1.rows,laplace_1.cols,CV_32FC1);
    cv::Mat laplace_normal_2(laplace_2.rows,laplace_2.cols,CV_32FC1);
    cv::Mat local_normal_1(local_1.rows,local_1.cols,CV_32FC1);
    cv::Mat local_normal_2(local_2.rows,local_2.cols,CV_32FC1);
    cv::Mat saliency_normal_1(saliency_1.rows,saliency_1.cols,CV_32FC1);
    cv::Mat saliency_normal_2(saliency_2.rows,saliency_2.cols,CV_32FC1);
    cv::Mat expose_normal_1(expose_1.rows,expose_1.cols,CV_32FC1);
    cv::Mat expose_normal_2(expose_2.rows,expose_2.cols,CV_32FC1);
   
    laplace_1.convertTo(laplace_1,CV_32FC1,1.0/255.0,0);
    laplace_2.convertTo(laplace_2,CV_32FC1,1.0/255.0,0);
    local_1.convertTo(local_1,CV_32FC1,1.0/255.0,0);
    local_2.convertTo(local_2,CV_32FC1,1.0/255.0,0);
    saliency_1.convertTo(saliency_1,CV_32FC1,1.0/255.0,0);
    saliency_2.convertTo(saliency_2,CV_32FC1,1.0/255.0,0);
    expose_1.convertTo(expose_1,CV_32FC1,1.0/255.0,0);
    expose_2.convertTo(expose_2,CV_32FC1,1.0/255.0,0);

    laplace_normal_1 = laplace_1 / (laplace_2 + laplace_1);
    local_normal_1 = local_1 /(local_2 + local_1);
    saliency_normal_1 = saliency_1 / (saliency_2 + saliency_1);
    expose_normal_1 = expose_1 /( expose_2 + expose_1);

    laplace_normal_2 = laplace_2 /( laplace_2 + laplace_1);
    local_normal_2 = local_2 / (local_2 + local_1);
    saliency_normal_2 = saliency_2 / (saliency_2 + saliency_1);
    expose_normal_2 = expose_2 / (expose_2 + expose_1);
    
    laplacian_contrast_1.reload_image(laplace_normal_1,"Normalized weights");
    local_contrast_1.reload_image(local_normal_1,"Normalized weights");
    saliency_contrast_1.reload_image(saliency_normal_1,"Normalized weights");
    exposedness_1.reload_image(expose_normal_1,"Normalized weights");

    laplacian_contrast_2.reload_image(laplace_normal_2,"Normalized weights");
    local_contrast_2.reload_image(local_normal_2,"Normalized weights");
    saliency_contrast_2.reload_image(saliency_normal_2,"Normalized weights");
    exposedness_2.reload_image(expose_normal_2,"Normalized weights");
    return;
}

CaptureFrame DehazeEnhance::fusion_blender()
{
    cv::Mat fusion_naive , fus_lap,fus_loc,fus_exp,fus_sal;
    fusion_naive= white_balanced_image.retrieve_image().clone().mul(cv::Scalar(0.6,0.6,0.6)) + en_CLAHE.retrieve_image().clone().mul(cv::Scalar(0.7,0.7,0.7));
    

    cv::Mat img1 = white_balanced_image.retrieve_image().clone();
    cv::Mat img2 = en_CLAHE.retrieve_image().clone();
    std::vector<cv::Mat> white_channels,contrast_channels,output_channels;
    cv::Mat black = cv::Mat::zeros(img1.rows,img1.cols,CV_32FC3);
    cv::split(img1,white_channels);
    cv::split(img2,contrast_channels);
    cv::split(black,output_channels);
    

    cv::Mat lap_1 = laplacian_contrast_1.retrieve_image().clone();
    cv::Mat lap_2 = laplacian_contrast_2.retrieve_image().clone();
    // cv::GaussianBlur(lap_1,lap_1,cv::Size(5,5),5);
    // cv::GaussianBlur(lap_2,lap_2,cv::Size(5,5),5);
    output_channels[0] = white_channels[0].mul(lap_1) + contrast_channels[0].mul(lap_2);
    output_channels[1] = white_channels[1].mul(lap_1) + contrast_channels[1].mul(lap_2);
    output_channels[2] = white_channels[2].mul(lap_1) + contrast_channels[2].mul(lap_2);
    cv::merge(output_channels,fus_lap);
    
    output_channels.clear();
    cv::split(black,output_channels);

    cv::Mat loc_1 = local_contrast_1.retrieve_image().clone();
    cv::Mat loc_2 = local_contrast_2.retrieve_image().clone();
    output_channels[0] = white_channels[0].mul(loc_1) + contrast_channels[0].mul(loc_2);
    output_channels[1] = white_channels[1].mul(loc_1) + contrast_channels[1].mul(loc_2);
    output_channels[2] = white_channels[2].mul(loc_1) + contrast_channels[2].mul(loc_2);
    cv::merge(output_channels,fus_loc);
    ///////////spli channels
    output_channels.clear();
    cv::split(black,output_channels);

    cv::Mat sal_1 = saliency_contrast_1.retrieve_image().clone();
    cv::Mat sal_2 = saliency_contrast_2.retrieve_image().clone();
    output_channels[0] = white_channels[0].mul(sal_1) + contrast_channels[0].mul(sal_2);
    output_channels[1] = white_channels[1].mul(sal_1) + contrast_channels[1].mul(sal_2);
    output_channels[2] = white_channels[2].mul(sal_1) + contrast_channels[2].mul(sal_2);
    cv::merge(output_channels,fus_sal);

    output_channels.clear();
    cv::split(black,output_channels);
    cv::Mat exp_1 = exposedness_1.retrieve_image().clone();
    cv::Mat exp_2 = exposedness_2.retrieve_image().clone();
    cv::GaussianBlur(exp_1,exp_1,cv::Size(5,5),5);
    cv::GaussianBlur(exp_2,exp_2,cv::Size(5,5),5);
    output_channels[0] = white_channels[0].mul(exp_1) + contrast_channels[0].mul(exp_2);
    output_channels[1] = white_channels[1].mul(exp_1) + contrast_channels[1].mul(exp_2);
    output_channels[2] = white_channels[2].mul(exp_1) + contrast_channels[2].mul(exp_2);
    cv::merge(output_channels,fus_exp);


    fus_lap.convertTo(fus_lap,CV_32FC3);
    fus_loc.convertTo(fus_loc,CV_32FC3);
    fus_sal.convertTo(fus_sal,CV_32FC3);
    fus_exp.convertTo(fus_exp,CV_32FC3);
    laplace_fusion.reload_image(fus_lap,"Laplacian weights are used");
    local_fusion.reload_image(fus_loc,"local weights are used");
    saliency_fusion.reload_image(fus_sal,"saliency weights are used");
    exposedness_fusion.reload_image(fus_exp,"Exposedness weights are used");

    cv::Mat finalfinal = (fus_lap.mul(cv::Scalar(0.3,0.3,0.3)) + fus_loc.mul(cv::Scalar(0.3,0.3,0.3)) + fus_sal.mul(cv::Scalar(0.2,0.2,0.2)) + fus_exp.mul(cv::Scalar(0.2,0.2,0.2)));
    finalfinal.convertTo(finalfinal,CV_8UC1);
    cv::imshow("finalfinal",finalfinal);
    cv::waitKey(0);

    cv::Mat finalweight_1 = (lap_1+loc_1+sal_1+exp_1)/4;
    cv::Mat finalweight_2 = (lap_2+loc_2+sal_2+exp_2)/4;
    cv::Mat finalfinalfinal;
    output_channels[0] = white_channels[0].mul(finalweight_1) + contrast_channels[0].mul(finalweight_2);
    output_channels[1] = white_channels[1].mul(finalweight_1) + contrast_channels[1].mul(finalweight_2);
    output_channels[2] = white_channels[2].mul(finalweight_1) + contrast_channels[2].mul(finalweight_2);
    cv::merge(output_channels,finalfinalfinal);
    cv::imshow("foinaflanf",finalfinalfinal);
    cv::waitKey(0);
    CaptureFrame output(fusion_naive,"naive blending");
    return output;
}

CaptureFrame DehazeEnhance::pyramid_fusion()
{
    int levels = 5;cv::Mat mahn,manh;
    int no_rows = laplacian_contrast_1.retrieve_image().rows;
    int no_cols = laplacian_contrast_1.retrieve_image().cols;
    cv::Mat TWW(no_rows,no_cols,CV_32FC1);
    cv::Mat TWC(no_rows,no_cols,CV_32FC1);
    TWW = (laplacian_contrast_1.retrieve_image().clone() + local_contrast_1.retrieve_image().clone() + saliency_contrast_1.retrieve_image().clone() + exposedness_1.retrieve_image().clone())/4;
    TWC = (laplacian_contrast_2.retrieve_image().clone() + local_contrast_2.retrieve_image().clone() + saliency_contrast_2.retrieve_image().clone() + exposedness_2.retrieve_image().clone())/4;
    
    
    total_weight_contrast = TWC;
    total_weight_white = TWW;
    
    total_weight_white.convertTo(total_weight_white,CV_32FC1);
    total_weight_contrast.convertTo(total_weight_contrast,CV_32FC1);
    CaptureFrame white_weight(total_weight_white,"Whitebalanced weight");
    CaptureFrame contrast_weight(total_weight_contrast,"Contrast enhanced weight");
    std::vector<cv::Mat> weight_1 = algo.gaussion_pyrdown(white_weight,levels);
    std::vector<cv::Mat> weight_2 = algo.gaussion_pyrdown(contrast_weight,levels);

    cv::Mat img1 = white_balanced_image.retrieve_image().clone();
    cv::Mat img2 = en_CLAHE.retrieve_image().clone();
    img1.convertTo(img1,CV_32FC3);
    img2.convertTo(img2,CV_32FC3);
   
    std::vector<cv::Mat> channels,b_white,g_white,r_white,b_contrast,g_contrast,r_contrast,fused_image_r,fused_image_g,fused_image_b;

    cv::split(img1,channels);

    b_white = algo.laplacian_pyrdown(channels[0],5);
    g_white = algo.laplacian_pyrdown(channels[1],5);
    r_white = algo.laplacian_pyrdown(channels[2],5);
    
    cv::split(img2,channels);

    b_contrast = algo.laplacian_pyrdown(channels[0],5);
    g_contrast = algo.laplacian_pyrdown(channels[1],5);
    r_contrast = algo.laplacian_pyrdown(channels[2],5);
    
    cv::Mat sum(img1.rows,img1.cols,CV_32FC1);
    
    for(int i = 0; i < levels; i++)
    {
        
        cv::add(b_white[i].mul(weight_1[i]),b_contrast[i].mul(weight_2[i]),sum);
        fused_image_b.push_back(sum);
        sum.release();
        cv::add(g_white[i].mul(weight_1[i]),g_contrast[i].mul(weight_2[i]),sum);
        fused_image_g.push_back(sum);
        sum.release();        
        cv::add(r_white[i].mul(weight_1[i]),r_contrast[i].mul(weight_2[i]),sum);
        fused_image_r.push_back(sum);
        sum.release();        
    }
    
    CaptureFrame b_channel = algo.pyr_reconstruct(fused_image_b,5);
    CaptureFrame g_channel = algo.pyr_reconstruct(fused_image_g,5);
    CaptureFrame r_channel = algo.pyr_reconstruct(fused_image_r,5);
    std::vector<cv::Mat> recovered_channels;
    recovered_channels.push_back(b_channel.retrieve_image().clone());
    recovered_channels.push_back(g_channel.retrieve_image().clone());
    recovered_channels.push_back(r_channel.retrieve_image().clone());
    cv::Mat fusion;
    
    cv::merge(recovered_channels,fusion);
    fusion.convertTo(fusion,CV_8UC3);
    CaptureFrame output(fusion,"Fused image");
    return output;
}