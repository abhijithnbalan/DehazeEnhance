#ifndef _dehaze_enhance_h
#define _dehaze_enhance_h

#include "view_frame.h"
#include "capture_frame.h"
#include "image_processing.h"

class DehazeEnhance : public ImageProcessing
{ 
    private:
        ViewFrame viewer;
        Logger logger;
        cv::Point max_intensity_point;
        CaptureFrame airlight_image,airlight_color;
        cv::Vec3b background_light;
        CaptureFrame red_transmission,green_transmission,blue_transmission,recovered_image;
        cv::Scalar background_color;
        CaptureFrame laplacian_contrast_1,local_contrast_1,saliency_contrast_1,exposedness_1;
        CaptureFrame laplacian_contrast_2,local_contrast_2,saliency_contrast_2,exposedness_2;
        CaptureFrame white_balanced_image;
        cv::Mat total_weight_white , total_weight_contrast;

        CaptureFrame laplace_fusion,local_fusion,saliency_fusion,exposedness_fusion;

    protected:

    public:
        CaptureFrame original_image,dark_channel,en_CLAHE,en_HE,saturation,u_darkchannel;
        void dark_channel_prior(CaptureFrame input);
        void fusion(CaptureFrame input);
        CaptureFrame show_airlight(CaptureFrame input_image);
        CaptureFrame find_airlight(CaptureFrame dark_channel,CaptureFrame saturation);
        CaptureFrame find_airlight(CaptureFrame input,int radius);
        void find_transmission(CaptureFrame image);
        CaptureFrame recover_image(CaptureFrame input_image);
        void normalize_weights();
        CaptureFrame fusion_blender();
        CaptureFrame pyramid_fusion();


};

#endif