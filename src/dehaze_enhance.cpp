//Standard includes
#include <opencv2/opencv.hpp>
//User defined includes
#include "dehaze_enhance.h"
#include "capture_frame.h"
#include "logger.h"
#include "view_frame.h"

void DehazeEnhance::dark_channel_prior(CaptureFrame input) //Dark channel prior method.
{
    logger.debug_mode = debug_mode;
    original_image = input; //Keeping a copy of original. Accessible throughout the program.
    CaptureFrame depth_map; //Depth map obtained from degradation from red channel in the input image.

    //CONTRAST ENHANCING
    // en_HE = algo.hist_equalize(input);//Normal Histogram equalization
    en_CLAHE = algo.CLAHE_dehaze(input); //CLAHE algorithm
    

    // DEPTH MAP
    depth_map = algo.red_irradiance(input, dark_channel_patch_size); //Converting Red channel to color-depth-map.
    logger.log_info("Depth map generated");

    //DARK CHANNEL
    dark_channel = algo.dark_channel(input, dark_channel_patch_size); //Calculate darkchannel
    // u_darkchannel = algo.u_dark_channel(input,5);//UDCP only used green and blue color for darkchannel generation.
    logger.log_info("Dark Channel image generated");

    //SATURATION MAP
    saturation = algo.saturation_map(input, dark_channel_patch_size);
    logger.log_info("Saturation map generated");

    if (dev_mode)
    {
        viewer.multiple_view_interrupted(input, dark_channel, saturation, depth_map, 50); //Showing all the filter effects.
    }
    //AIRLIGHT ESTIMATION
    airlight_image = find_airlight(dark_channel, saturation);
    if (dev_mode)
    {
        viewer.multiple_view_interrupted(airlight_image, airlight_color, 75); //Display for airlight estimation
    }
    logger.log_info("Airlight estimated");

    //TRANSMISSION ESTIMATION
    find_transmission(input);

    //RECOVERING IMAGE
    recovered_image = recover_image(input);

    if (dev_mode) //Resutl
    {
        viewer.multiple_view_interrupted(original_image, recovered_image, 75); //Displaying initial image and final image.
    }
    return;
}

CaptureFrame DehazeEnhance::show_airlight(CaptureFrame input_image) //Showing airlight
{
    if (max_intensity_point.x + max_intensity_point.y == 0) //making sure that max_intensity_point has some data
    {
        logger.log_warn("Airlight is not identified before to show");
        return input_image;
    }
    cv::Mat temp = input_image.retrieve_image().clone();
    //Making a rectangle of side 10 pixel centered at the max_intensity point for displaying.
    cv::rectangle(temp, cv::Point(max_intensity_point.x + 5, max_intensity_point.y + 5), cv::Point(max_intensity_point.x - 5, max_intensity_point.y - 5), cv::Scalar(0, 0, 255), 2, 8, 0);
    //Returning the rectangle drawn image.
    CaptureFrame output(temp, "airlight");
    return output;
}
CaptureFrame DehazeEnhance::find_airlight(CaptureFrame dark_channel, CaptureFrame saturaiton) //find airlight from dark channel and saturation map
{

    cv::Mat orig_img = original_image.retrieve_image().clone(); //input image is stored
    cv::Mat temp_dark = dark_channel.retrieve_image().clone();  //darkchannel stored
    cv::Mat temp_satu = saturation.retrieve_image().clone();    //saturation map stored

    cv::Mat filtered_satu(temp_satu.rows, temp_satu.cols, CV_32FC1);
    cv::Mat filtered_dark(temp_dark.rows, temp_dark.cols, CV_32FC1);
    cv::Mat filtered_common(temp_dark.rows, temp_dark.cols, CV_32FC1);

    // converting stored images to Float type
    temp_satu.convertTo(temp_satu, CV_32FC1);
    temp_dark.convertTo(temp_dark, CV_32FC1);
    cv::Mat total_threshold(temp_satu.rows, temp_dark.cols, CV_32FC1); //for thresholding to identify airlight

    total_threshold = temp_satu * saturation_weight + temp_dark * dark_channel_weight; //wighted sum of saturation map and darkchannel
    //Airlight will be having high value on both saturation and darkchannel

    total_threshold.convertTo(total_threshold, CV_8UC1); //converting back to uchar image for applying histogram equalization

    cv::blur(total_threshold, total_threshold, cv::Size(5, 5)); //bluring to blend and remove small patches

    cv::equalizeHist(total_threshold, total_threshold); //histogram equalization for filtering out the largest value pixels.

    total_threshold.convertTo(total_threshold, CV_32FC1, 1.0 / 255.0, 0); //Converting back to Float type

    //filtering according to threshold(chosen as 0.9 to 1)
    cv::inRange(total_threshold, cv::Scalar(airlight_threshold), cv::Scalar(1), filtered_satu);

    //Erode the filtered image to refine the selection
    int erosion_size = 6;
    cv::Mat element = getStructuringElement(cv::MORPH_CROSS,
                                            cv::Size(2 * erosion_size + 1, 2 * erosion_size + 1),
                                            cv::Point(erosion_size, erosion_size));
    cv::erode(filtered_satu, filtered_satu, element, cv::Point(-1, -1), 2); //eroding two times

    //Finding the contour with maximum area
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(filtered_satu, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
    double largest_area = 0;
    int index = 0;
    for (int i = 0; i < contours.size(); i++) // iterate through each contour.
    {
        double a = contourArea(contours[i], false); //  Find the area of contour
        if (a > largest_area)
        {
            largest_area = a;
            index = i; //Store the index of largest contour
        }
    }
    cv::Rect bounding_rect;
    bounding_rect = cv::boundingRect(contours[index]);                      //Creating bounding rectangle for the largest contour
    cv::rectangle(orig_img, bounding_rect, cv::Scalar(0, 255, 0), 1, 8, 0); //Drawing the rectangle on the original image saved before

    //loading data to max_intensity_point and identifying the airlight color
    max_intensity_point.x = bounding_rect.x + bounding_rect.width / 2;
    max_intensity_point.y = bounding_rect.y + bounding_rect.height / 2;
    background_light = orig_img.at<cv::Vec3b>(max_intensity_point.y, max_intensity_point.x);
    background_color = cv::Scalar((float)background_light.val[0], (float)background_light.val[1], (float)background_light.val[2]);

    //Reloading images and returning the airlight identified image back.
    cv::Mat temp_color(orig_img.rows, orig_img.cols, CV_32FC3, background_color);
    airlight_color.reload_image(temp_color, "Background color");
    CaptureFrame output(orig_img, "airlight mark");
    logger.log_info("Airlight estimated");
    return output;
}

CaptureFrame DehazeEnhance::find_airlight(CaptureFrame input_image, int radius) //find airlight with input image(dark channel identification included)
{
    cv::Mat temp = input_image.retrieve_image().clone(); //Storing input
    cv::Mat dark = cv::Mat::zeros(temp.size(), CV_8UC1);
    cv::GaussianBlur(temp, temp, cv::Size(5, 5), 1, 1, 4); //blurring input for blending the colors slightly

    //Calculating darkchannel and at the same time, finding the maximum value
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
            dark.at<uchar>(row, col) = min; //Finding the maximum value
            if (max_intensity < min)
            {
                max_intensity = min;
                max_intensity_point.y = row;
                max_intensity_point.x = col;
            }
        }
    }
    background_light = temp.at<cv::Vec3b>(max_intensity_point.y, max_intensity_point.x); //storing the background light

    background_color = cv::Scalar((float)background_light.val[0], (float)background_light.val[1], (float)background_light.val[2]); //Identifying the color

    //loading the color
    cv::Mat temp_color(temp.rows, temp.cols, CV_32FC3, background_color);
    airlight_color.reload_image(temp_color, "Background color");
    temp.release();

    //Returning the darkchannel
    CaptureFrame output(dark, "DarkChannel");
    return output;
}

void DehazeEnhance::find_transmission(CaptureFrame input_image) //Find transmission
{
    cv::Mat temp = dark_channel.retrieve_image().clone(); //dark channel will be used for transmission

    //Float type mat are used
    cv::Mat red_tra(temp.rows, temp.cols, CV_32FC1);
    cv::Mat blue_tra(temp.rows, temp.cols, CV_32FC1);
    cv::Mat green_tra(temp.rows, temp.cols, CV_32FC1);

    //Dark channel is converted to Float for consistancy
    temp.convertTo(temp, CV_32FC1);

    //Finding transmission map for red channel
    cv::Mat unity(temp.rows, temp.cols, CV_32FC1, cv::Scalar(1));
    cv::Mat temp_recovered(temp.rows, temp.cols, CV_32FC1);
    cv::divide(temp, cv::Scalar((float)background_light.val[2]), temp_recovered);
    temp_recovered.convertTo(temp, CV_32FC1, 1.0 / 255.0, 0);
    red_tra = unity - temp_recovered;

    //Calculating trasnsmission map for other colors from red channel transmission map by attenuation coefficients
    int attenuation_coefficient_blue = 1.20777 * ((float)background_light.val[2]) / ((float)background_light.val[0]);
    int attenuation_coefficient_green = 1.09777 * ((float)background_light.val[2]) / ((float)background_light.val[1]);
    cv::pow(red_tra, attenuation_coefficient_blue, blue_tra);
    cv::pow(red_tra, attenuation_coefficient_green, green_tra);

    //Loading the transmission to global variables
    red_transmission.reload_image(red_tra, "Transmisison for red channel");
    blue_transmission.reload_image(blue_tra, "Transmisison for blue channel");
    green_transmission.reload_image(green_tra, "Transmisison for green channel");

    logger.log_info("Transmission map generated");
    return;
}

CaptureFrame DehazeEnhance::recover_image(CaptureFrame input_image) //Recovering image
{
    cv::Mat temp, transmission_div, recovered, tot_transmission;
    temp = input_image.retrieve_image().clone();

    temp.convertTo(temp, CV_32FC3);
    cv::Mat airlight_sub(temp.rows, temp.cols, CV_32FC3, cv::Scalar(0, 0, 0));
    cv::Mat airlight;
    airlight = airlight_color.retrieve_image(); //Retrieving the airlight color

    recovered = temp.clone();

    //Creating complete transmission map as a 3 channel image
    std::vector<cv::Mat> ch;
    ch.push_back(blue_transmission.retrieve_image());
    ch.push_back(green_transmission.retrieve_image());
    ch.push_back(red_transmission.retrieve_image());
    cv::merge(ch, tot_transmission);

    //FIRST step of Recovering, removing backlight from the whole image.
    airlight_sub = temp - airlight;

    std::vector<cv::Mat> reco_channels, channels;
    cv::split(recovered, reco_channels);
    cv::split(airlight_sub, channels);
    cv::Mat blue_trans;
    cv::Mat red_trans;
    cv::Mat green_trans;

    //
    cv::Mat max_transmission(blue_transmission.retrieve_image().rows, blue_transmission.retrieve_image().cols, CV_32FC1, cv::Scalar(0.1));
    cv::max(blue_transmission.retrieve_image(), max_transmission, blue_trans);
    cv::max(green_transmission.retrieve_image(), max_transmission, green_trans);
    cv::max(red_transmission.retrieve_image(), max_transmission, red_trans);

    //SECOND step, dividing by transmission map for each channel
    reco_channels[0] = channels[0] / blue_trans;
    reco_channels[1] = channels[1] / green_trans;
    reco_channels[2] = channels[2] / red_trans;
    cv::merge(reco_channels, transmission_div);

    //THIRD step, adding the background light back;
    recovered = transmission_div + airlight;

    //Converting back the image to uchar type and returning the recovered image
    recovered.convertTo(recovered, CV_8UC3);
    ch.clear();
    reco_channels.clear();
    channels.clear();
    CaptureFrame output(recovered, "Recovered image");
    return output;
}

CaptureFrame DehazeEnhance::recover_image_shallow(CaptureFrame input_image) //Recovering image
{
    
    cv::Mat temp, transmission_div, recovered, tot_transmission;
    temp = input_image.retrieve_image();

    temp.convertTo(temp, CV_32FC3);
    cv::Mat airlight_sub(temp.rows, temp.cols, CV_32FC3, cv::Scalar(0, 0, 0));
    cv::Mat airlight;
    airlight = airlight_color.retrieve_image(); //Retrieving the airlight color

    recovered = temp;

    //Creating complete transmission map as a 3 channel image
    std::vector<cv::Mat> ch;
    ch.push_back(blue_transmission.retrieve_image());
    ch.push_back(green_transmission.retrieve_image());
    ch.push_back(red_transmission.retrieve_image());
    cv::merge(ch, tot_transmission);

    //FIRST step of Recovering, removing backlight from the whole image.
    airlight_sub = temp - airlight;

    std::vector<cv::Mat> reco_channels, channels;
    cv::split(recovered, reco_channels);
    cv::split(airlight_sub, channels);
    cv::Mat blue_trans;
    cv::Mat red_trans;
    cv::Mat green_trans;

    //
    cv::Mat max_transmission(blue_transmission.retrieve_image().rows, blue_transmission.retrieve_image().cols, CV_32FC1, cv::Scalar(0.1));
    cv::max(blue_transmission.retrieve_image(), max_transmission, blue_trans);
    cv::max(green_transmission.retrieve_image(), max_transmission, green_trans);
    cv::max(red_transmission.retrieve_image(), max_transmission, red_trans);

    //SECOND step, dividing by transmission map for each channel
    reco_channels[0] = channels[0] / blue_trans;
    reco_channels[1] = channels[1] / green_trans;
    reco_channels[2] = channels[2] / red_trans;
    cv::merge(reco_channels, transmission_div);

    //THIRD step, adding the background light back;
    recovered = transmission_div + airlight;

    //Converting back the image to uchar type and returning the recovered image
    recovered.convertTo(recovered, CV_8UC3);
    ch.clear();
    reco_channels.clear();
    channels.clear();
    CaptureFrame output(recovered, "Recovered image");
    return output;
}

void DehazeEnhance::fusion(CaptureFrame input) //Dehazing by fusion
{
    logger.debug_mode = debug_mode;
    CaptureFrame original(input.retrieve_image().clone(), "original input"); //keeping a copy of the original. Accessible throughout the program

    //PREPARING INPUTS
    white_balanced_image = algo.balance_white(input); //White balanced image
    en_CLAHE = algo.CLAHE_dehaze(original);           //Contrast enhanced image
    // en_HE = algo.hist_equalize(original);//Contrast enhanced image
    logger.log_warn("Inputs prepared");

    //PREPARING THE WEIGHTS
    laplacian_contrast_1 = algo.laplacian_contrast(white_balanced_image); //Laplacian contrast weight
    laplacian_contrast_2 = algo.laplacian_contrast(en_CLAHE);
    local_contrast_1 = algo.local_contrast(white_balanced_image); //Local contrast weight
    local_contrast_2 = algo.local_contrast(en_CLAHE);
    saliency_contrast_1 = algo.saliency_contrast(white_balanced_image); //Saliency weight
    saliency_contrast_2 = algo.saliency_contrast(en_CLAHE);
    exposedness_1 = algo.exposedness(white_balanced_image); //Exposedness weight
    exposedness_2 = algo.exposedness(en_CLAHE);

    if (dev_mode)
    {
        viewer.multiple_view_interrupted(en_CLAHE, white_balanced_image, 50); //Displaying two inputs used
        //Displaying weights for both the pictures
        viewer.multiple_view_interrupted(laplacian_contrast_1, local_contrast_1, saliency_contrast_1, exposedness_1, 75);
        viewer.multiple_view_interrupted(laplacian_contrast_2, local_contrast_2, saliency_contrast_2, exposedness_2, 75);
    }
    normalize_weights();
    logger.log_info("Weights Normalized");

    pyramid_blending = pyramid_fusion();
    // naive_blending = fusion_blender();
    logger.log_info("Images blended");
    if (dev_mode)
    {
        viewer.multiple_view_interrupted(original, pyramid_blending, 75); //Displaying image before and after
    }
    return;
}
void DehazeEnhance::normalize_weights() //Normalizing weights
{
    //Retrieving all the weights.
    cv::Mat laplace_1 = laplacian_contrast_1.retrieve_image().clone();
    cv::Mat laplace_2 = laplacian_contrast_2.retrieve_image().clone();
    cv::Mat local_1 = local_contrast_1.retrieve_image().clone();
    cv::Mat local_2 = local_contrast_2.retrieve_image().clone();
    cv::Mat saliency_1 = saliency_contrast_1.retrieve_image().clone();
    cv::Mat saliency_2 = saliency_contrast_2.retrieve_image().clone();
    cv::Mat expose_1 = exposedness_1.retrieve_image().clone();
    cv::Mat expose_2 = exposedness_2.retrieve_image().clone();

    //Initializing normalized weights (Float type).
    cv::Mat laplace_normal_1(laplace_1.rows, laplace_1.cols, CV_32FC1);
    cv::Mat laplace_normal_2(laplace_2.rows, laplace_2.cols, CV_32FC1);
    cv::Mat local_normal_1(local_1.rows, local_1.cols, CV_32FC1);
    cv::Mat local_normal_2(local_2.rows, local_2.cols, CV_32FC1);
    cv::Mat saliency_normal_1(saliency_1.rows, saliency_1.cols, CV_32FC1);
    cv::Mat saliency_normal_2(saliency_2.rows, saliency_2.cols, CV_32FC1);
    cv::Mat expose_normal_1(expose_1.rows, expose_1.cols, CV_32FC1);
    cv::Mat expose_normal_2(expose_2.rows, expose_2.cols, CV_32FC1);

    //Converting all weights to float and mapping every values from 0 to 1
    laplace_1.convertTo(laplace_1, CV_32FC1, 1.0 / 255.0, 0);
    laplace_2.convertTo(laplace_2, CV_32FC1, 1.0 / 255.0, 0);
    local_1.convertTo(local_1, CV_32FC1, 1.0 / 255.0, 0);
    local_2.convertTo(local_2, CV_32FC1, 1.0 / 255.0, 0);
    saliency_1.convertTo(saliency_1, CV_32FC1, 1.0 / 255.0, 0);
    saliency_2.convertTo(saliency_2, CV_32FC1, 1.0 / 255.0, 0);
    expose_1.convertTo(expose_1, CV_32FC1, 1.0 / 255.0, 0);
    expose_2.convertTo(expose_2, CV_32FC1, 1.0 / 255.0, 0);

    //Normalizing weights for white balanced image
    laplace_normal_1 = laplace_1 / (laplace_2 + laplace_1);
    local_normal_1 = local_1 / (local_2 + local_1);
    saliency_normal_1 = saliency_1 / (saliency_2 + saliency_1);
    expose_normal_1 = expose_1 / (expose_2 + expose_1);

    //Normalizing weights for contrast enhanced image.
    laplace_normal_2 = laplace_2 / (laplace_2 + laplace_1);
    local_normal_2 = local_2 / (local_2 + local_1);
    saliency_normal_2 = saliency_2 / (saliency_2 + saliency_1);
    expose_normal_2 = expose_2 / (expose_2 + expose_1);

    //Loading the normalised weights instead of weights.(for white balanced image)
    laplacian_contrast_1.reload_image(laplace_normal_1, "Normalized weights");
    local_contrast_1.reload_image(local_normal_1, "Normalized weights");
    saliency_contrast_1.reload_image(saliency_normal_1, "Normalized weights");
    exposedness_1.reload_image(expose_normal_1, "Normalized weights");

    //Loading the normalised weights instead of weights.(for contrast enhanced image)
    laplacian_contrast_2.reload_image(laplace_normal_2, "Normalized weights");
    local_contrast_2.reload_image(local_normal_2, "Normalized weights");
    saliency_contrast_2.reload_image(saliency_normal_2, "Normalized weights");
    exposedness_2.reload_image(expose_normal_2, "Normalized weights");

    return;
}

CaptureFrame DehazeEnhance::fusion_blender() //Fusion blending basic
{
    cv::Mat fusion_naive, fus_lap, fus_loc, fus_exp, fus_sal;

    //Naive blending, this is the average of contrast enhanced version and white balanced version.
    fusion_naive = white_balanced_image.retrieve_image().clone().mul(cv::Scalar(0.5, 0.5, 0.5)) + en_CLAHE.retrieve_image().clone().mul(cv::Scalar(0.5, 0.5, 0.5));

    //Preparing input images and channels
    cv::Mat img1 = white_balanced_image.retrieve_image().clone();
    cv::Mat img2 = en_CLAHE.retrieve_image().clone();
    std::vector<cv::Mat> white_channels, contrast_channels, output_channels;
    cv::Mat black = cv::Mat::zeros(img1.rows, img1.cols, CV_32FC3);
    cv::split(img1, white_channels);
    cv::split(img2, contrast_channels);
    cv::split(black, output_channels);

    //Checking individual weight effects on the image.

    //Laplacian contrast weights
    cv::Mat lap_1 = laplacian_contrast_1.retrieve_image().clone();
    cv::Mat lap_2 = laplacian_contrast_2.retrieve_image().clone();
    //Applying Laplacinan weights and storing.
    output_channels[0] = white_channels[0].mul(lap_1) + contrast_channels[0].mul(lap_2);
    output_channels[1] = white_channels[1].mul(lap_1) + contrast_channels[1].mul(lap_2);
    output_channels[2] = white_channels[2].mul(lap_1) + contrast_channels[2].mul(lap_2);
    cv::merge(output_channels, fus_lap);

    //Clearing channels and preparing for next weight
    output_channels.clear();
    cv::split(black, output_channels);

    //Local contrast weight
    cv::Mat loc_1 = local_contrast_1.retrieve_image().clone();
    cv::Mat loc_2 = local_contrast_2.retrieve_image().clone();
    //Applying local contrast weights and storing
    output_channels[0] = white_channels[0].mul(loc_1) + contrast_channels[0].mul(loc_2);
    output_channels[1] = white_channels[1].mul(loc_1) + contrast_channels[1].mul(loc_2);
    output_channels[2] = white_channels[2].mul(loc_1) + contrast_channels[2].mul(loc_2);
    cv::merge(output_channels, fus_loc);

    //Clearing channels and preparing for next weight
    output_channels.clear();
    cv::split(black, output_channels);

    //Saliency weight
    cv::Mat sal_1 = saliency_contrast_1.retrieve_image().clone();
    cv::Mat sal_2 = saliency_contrast_2.retrieve_image().clone();
    //Applying saliency weight and storing
    output_channels[0] = white_channels[0].mul(sal_1) + contrast_channels[0].mul(sal_2);
    output_channels[1] = white_channels[1].mul(sal_1) + contrast_channels[1].mul(sal_2);
    output_channels[2] = white_channels[2].mul(sal_1) + contrast_channels[2].mul(sal_2);
    cv::merge(output_channels, fus_sal);

    //Clearing channels and preparing for next weight
    output_channels.clear();
    cv::split(black, output_channels);

    //Exposedness weights
    cv::Mat exp_1 = exposedness_1.retrieve_image().clone();
    cv::Mat exp_2 = exposedness_2.retrieve_image().clone();
    //Applying exposedness weight and storing.
    output_channels[0] = white_channels[0].mul(exp_1) + contrast_channels[0].mul(exp_2);
    output_channels[1] = white_channels[1].mul(exp_1) + contrast_channels[1].mul(exp_2);
    output_channels[2] = white_channels[2].mul(exp_1) + contrast_channels[2].mul(exp_2);
    cv::merge(output_channels, fus_exp);

    //Converting to Float and loading to global varibales
    fus_lap.convertTo(fus_lap, CV_32FC3);
    fus_loc.convertTo(fus_loc, CV_32FC3);
    fus_sal.convertTo(fus_sal, CV_32FC3);
    fus_exp.convertTo(fus_exp, CV_32FC3);
    laplace_fusion.reload_image(fus_lap, "Laplacian weights are used");
    local_fusion.reload_image(fus_loc, "local weights are used");
    saliency_fusion.reload_image(fus_sal, "saliency weights are used");
    exposedness_fusion.reload_image(fus_exp, "Exposedness weights are used");

    //Creating final image by adding all the individual images together.
    cv::Mat addition_of_images = (fus_lap.mul(cv::Scalar(0.25, 0.25, 0.25)) + fus_loc.mul(cv::Scalar(0.25, 0.25, 0.25)) + fus_sal.mul(cv::Scalar(0.25, 0.25, 0.25)) + fus_exp.mul(cv::Scalar(0.25, 0.25, 0.25)));
    addition_of_images.convertTo(addition_of_images, CV_8UC1);
    image_fusion.reload_image(addition_of_images, "Fusion of images");

    //Creating final image by adding all the weights together
    cv::Mat finalweight_1 = (lap_1 + loc_1 + sal_1 + exp_1) / 4;
    cv::Mat finalweight_2 = (lap_2 + loc_2 + sal_2 + exp_2) / 4;
    cv::Mat addition_of_weights;
    output_channels[0] = white_channels[0].mul(finalweight_1) + contrast_channels[0].mul(finalweight_2);
    output_channels[1] = white_channels[1].mul(finalweight_1) + contrast_channels[1].mul(finalweight_2);
    output_channels[2] = white_channels[2].mul(finalweight_1) + contrast_channels[2].mul(finalweight_2);
    cv::merge(output_channels, addition_of_weights);
    weight_fusion.reload_image(addition_of_weights, "Fusion of weights");
    output_channels.clear();
    white_channels.clear();
    contrast_channels.clear();
    output_channels.clear();
    //Returning the basic blending from the function
    CaptureFrame output(fusion_naive, "Naive blending");
    return output;
}

CaptureFrame DehazeEnhance::pyramid_fusion() //Pyramid blending (more advanced)
{

    int levels = pyramid_level; //Number of levels for pyramid operation

    //Preparing for final weight
    int no_rows = laplacian_contrast_1.retrieve_image().rows;
    int no_cols = laplacian_contrast_1.retrieve_image().cols;
    cv::Mat TWW(no_rows, no_cols, CV_32FC1);
    cv::Mat TWC(no_rows, no_cols, CV_32FC1);

    //Computing final weight
    TWW = (laplacian_contrast_1.retrieve_image().clone() + local_contrast_1.retrieve_image().clone() + saliency_contrast_1.retrieve_image().clone() + exposedness_1.retrieve_image().clone()) / 4;
    TWC = (laplacian_contrast_2.retrieve_image().clone() + local_contrast_2.retrieve_image().clone() + saliency_contrast_2.retrieve_image().clone() + exposedness_2.retrieve_image().clone()) / 4;
    total_weight_contrast = TWC;
    total_weight_white = TWW;

    //Converting weights into Float and storing
    total_weight_white.convertTo(total_weight_white, CV_32FC1);
    total_weight_contrast.convertTo(total_weight_contrast, CV_32FC1);
    CaptureFrame white_weight(total_weight_white, "Whitebalanced weight");
    CaptureFrame contrast_weight(total_weight_contrast, "Contrast enhanced weight");

    //Taking Gaussian pyramid for weights
    std::vector<cv::Mat> weight_1 = algo.gaussion_pyrdown(white_weight, levels);
    std::vector<cv::Mat> weight_2 = algo.gaussion_pyrdown(contrast_weight, levels);

    //Retrieving input images and converting them to Float image
    cv::Mat img1 = white_balanced_image.retrieve_image().clone();
    cv::Mat img2 = en_CLAHE.retrieve_image().clone();
    img1.convertTo(img1, CV_32FC3);
    img2.convertTo(img2, CV_32FC3);

    std::vector<cv::Mat> channels, b_white, g_white, r_white, b_contrast, g_contrast, r_contrast, fused_image_r, fused_image_g, fused_image_b;

    //Taking Laplace pyrmid for all the three channels
    cv::split(img1, channels);
    b_white = algo.laplacian_pyrdown(channels[0], levels);
    g_white = algo.laplacian_pyrdown(channels[1], levels);
    r_white = algo.laplacian_pyrdown(channels[2], levels);

    //Taking Laplace pyramid for all the three channels.
    cv::split(img2, channels);
    b_contrast = algo.laplacian_pyrdown(channels[0], levels);
    g_contrast = algo.laplacian_pyrdown(channels[1], levels);
    r_contrast = algo.laplacian_pyrdown(channels[2], levels);

    cv::Mat sum(img1.rows, img1.cols, CV_32FC1); //Initializing sum

    //Fusion of images with weights of all images from all levels of pyramid
    for (int i = 0; i < levels; i++)
    {
        //Fusing all the channels in all levels
        cv::add(b_white[i].mul(weight_1[i]), b_contrast[i].mul(weight_2[i]), sum);
        fused_image_b.push_back(sum);
        sum.release();
        cv::add(g_white[i].mul(weight_1[i]), g_contrast[i].mul(weight_2[i]), sum);
        fused_image_g.push_back(sum);
        sum.release();
        cv::add(r_white[i].mul(weight_1[i]), r_contrast[i].mul(weight_2[i]), sum);
        fused_image_r.push_back(sum);
        sum.release();
    }

    //Reconstructing from pyramid
    CaptureFrame b_channel = algo.pyr_reconstruct(fused_image_b, levels);
    CaptureFrame g_channel = algo.pyr_reconstruct(fused_image_g, levels);
    CaptureFrame r_channel = algo.pyr_reconstruct(fused_image_r, levels);

    //Merging the final output channels to form a single image
    std::vector<cv::Mat> recovered_channels;
    recovered_channels.push_back(b_channel.retrieve_image().clone());
    recovered_channels.push_back(g_channel.retrieve_image().clone());
    recovered_channels.push_back(r_channel.retrieve_image().clone());

    cv::Mat fusion;
    cv::merge(recovered_channels, fusion);
    fusion.convertTo(fusion, CV_8UC3);

    //Clearing all the variables used
    fused_image_b.clear();
    fused_image_g.clear();
    fused_image_r.clear();
    recovered_channels.clear();
    channels.clear();
    b_white.clear();
    g_white.clear();
    r_white.clear();
    b_contrast.clear();
    g_contrast.clear();
    r_contrast.clear();
    weight_1.clear();
    weight_2.clear();

    //Returning fused image back to function.
    CaptureFrame output(fusion, "Fused image");
    return output;
}

void DehazeEnhance::video_enhance(std::string method, CaptureFrame video1)
{
    logger.debug_mode = debug_mode;
    cv::Mat image;
    CaptureFrame video;
    video1.frame_extraction();
    image = video1.retrieve_image().clone();
    cv::VideoWriter outputVideo1;
    std::string outfile = filename.substr(0, filename.size()-4) + "_" + method + ".mp4";
    cv::Size S1 = cv::Size((int)image.cols, // Acquire input size
                           (int)image.rows);
    int ex = static_cast<int>(video1.retrieve_video().get(CV_CAP_PROP_FOURCC)); // Get Codec Type- Int form
    outputVideo1.open(outfile, ex, video1.retrieve_video().get(CV_CAP_PROP_FPS), S1, true);

    //Converting percentage values to pixel values for Region of interest
    roi.x = roi_percent.x * image.cols / 100;
    roi.width = roi_percent.width * image.cols / 100;
    roi.y = roi_percent.y * image.rows / 100;
    roi.height = roi_percent.height * image.rows / 100;

    if (!outputVideo1.isOpened())
    {
        std::cout << "Could not open the output video for write: " << std::endl;
        return;
    }
    CaptureFrame output;
    if (method == "DCP")
    {
        logger.log_info("Dark Channel Prior method for video enhance");
        for (;;)
        {
            try
            {
                video1.frame_extraction();
                // image = resize_image(video,50);
                video.reload_image_shallow(video1.retrieve_image()(roi), "region of video");
            }
            catch (...)
            {
                logger.log_warn("End of video reached");
                break;
            }
            output.clear();

            output = dark_channel_prior(video, 0); //Dark channel algorithm

            cv::waitKey(3);

            outputVideo1 << video1.retrieve_image().clone(); //write into video file
            if (dev_mode)
            {
                viewer.single_view_uninterrupted(video1, 50); //Display the result
            }
            if (cv::waitKey(10) > 0)
                break;
        }
        logger.log_info("Video writing completed");
    }

    if (method == "fusion")
    {
        logger.log_info("Fusion method for video enhance");

        CaptureFrame compare;
        for (;;)
        {

            try
            {
                video1.frame_extraction();
                // image = resize_image(video,50);//Optional resize
                video.reload_image_shallow(video1.retrieve_image()(roi), "region of video");
            }
            catch (...)
            {
                logger.log_warn("End of video reached");
                break;
            }
            output.clear();

            output = fusion(video, 0); //Fusion algorithm

            cv::waitKey(3);

            outputVideo1 << video1.retrieve_image().clone(); //Writing into video file
            if (dev_mode)
            {
                viewer.single_view_uninterrupted(video1, 50); //show output
            }
            if (cv::waitKey(10) > 0)
                break; //wait for keypress to exit the program
        }
        logger.log_info("Video writing completed");
    }

    return;
}

CaptureFrame DehazeEnhance::fusion(CaptureFrame input, int mode) //Dehazing by fusion minimal mode
{

    //PREPARING INPUTS
    white_balanced_image = algo.balance_white_shallow(input); //White balanced image
    en_CLAHE = algo.CLAHE_dehaze_shallow(input);              //Contrast enhanced image

    //PREPARING THE WEIGHTS
    laplacian_contrast_1 = algo.laplacian_contrast(white_balanced_image); //Laplacian contrast weight
    laplacian_contrast_2 = algo.laplacian_contrast(en_CLAHE);
    local_contrast_1 = algo.local_contrast(white_balanced_image); //Local contrast weight
    local_contrast_2 = algo.local_contrast(en_CLAHE);
    saliency_contrast_1 = algo.saliency_contrast(white_balanced_image); //Saliency weight
    saliency_contrast_2 = algo.saliency_contrast(en_CLAHE);
    exposedness_1 = algo.exposedness(white_balanced_image); //Exposedness weight
    exposedness_2 = algo.exposedness(en_CLAHE);

    normalize_weights();

    CaptureFrame output;

    output = pyramid_fusion();

    return output;
}

CaptureFrame DehazeEnhance::dark_channel_prior(CaptureFrame input, int mode) //Dark channel prior method minimal mode
{
    //DARK CHANNEL
    dark_channel = algo.dark_channel(input, dark_channel_patch_size); //Calculate darkchannel

    //SATURATION MAP
    saturation = algo.saturation_map(input, dark_channel_patch_size);

    //AIRLIGHT ESTIMATION
    airlight_image = find_airlight(dark_channel, saturation);

    //TRANSMISSION ESTIMATION
    find_transmission(input);

    //RECOVERING IMAGE
    recovered_image = recover_image_shallow(input);

    return recovered_image;
}
CaptureFrame DehazeEnhance::comparison(CaptureFrame input, CaptureFrame enhanced) //Function to prepare comparison output if full frame enhancing is opted
{
    cv::Mat in, en;

    //getting both inputs
    in = input.retrieve_image().clone();
    en = enhanced.retrieve_image().clone();

    //Copying enhanced image to original image using mask
    en.copyTo(in, mask);

    CaptureFrame output(in, "Comparison"); //Result

    return output;
}

DehazeEnhance::DehazeEnhance()
{
    dark_channel_patch_size = 5;
    dark_channel_weight = 0.6;
    saturation_weight = 0.4;
    airlight_threshold = 0.9;

    pyramid_level = 5;

    debug_mode = false;
    dev_mode = false;

    roi_percent.x = 50;
    roi_percent.y = 0;
    roi_percent.height = 100;
    roi_percent.width = 50;
}