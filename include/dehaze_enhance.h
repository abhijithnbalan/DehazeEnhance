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
        CaptureFrame airlight_image;
        cv::Vec3b background_light;
        CaptureFrame red_transmission,green_transmission,blue_transmission;

    protected:

    public:
        CaptureFrame original_image,dark_channel,en_CLAHE,en_HE,saturation,u_darkchannel;
        int airlight;
        void application(CaptureFrame input);
        void show_airlight(CaptureFrame input_image);
        int find_airlight(CaptureFrame dark_channel,CaptureFrame saturation);
        CaptureFrame find_airlight(CaptureFrame input,int radius);
        void find_transmission(CaptureFrame image);
        CaptureFrame recover_image(CaptureFrame input_image);


};

#endif