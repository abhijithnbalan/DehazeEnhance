#ifndef _dehaze_enhance_h
#define _dehaze_enhance_h

//Including user defined headers
#include "view_frame.h"
#include "capture_frame.h"
#include "image_processing.h"
#include "timer.h"

class DehazeEnhance : public ImageProcessing
{ 
    private:
        ViewFrame viewer;//viewer can be used anywhere inside the object.
        Logger logger;//Logger support for the class
        
        cv::Point max_intensity_point;
        CaptureFrame airlight_image,airlight_color;
        cv::Vec3b background_light;
        CaptureFrame red_transmission,green_transmission,blue_transmission,recovered_image;
        cv::Scalar background_color;
        CaptureFrame laplacian_contrast_1,local_contrast_1,saliency_contrast_1,exposedness_1;//weights for white balance image
        CaptureFrame laplacian_contrast_2,local_contrast_2,saliency_contrast_2,exposedness_2;//weights for contrast enhanced image
        CaptureFrame white_balanced_image;
        cv::Mat total_weight_white , total_weight_contrast;
        CaptureFrame laplace_fusion,local_fusion,saliency_fusion,exposedness_fusion;
        cv::Rect roi;
        Timer timer1,timer2,timer3,timer4,timer5;
        
        cv::Rect crop_window;
        cv::Mat mask;
        //For Simplified DCP
        int Airlightp;          //airlight value of previous frame
        int Airlight;           //airlight value of current frame
        int ad; 
        const double alpha = 0.05;    //alpha smoothing



    protected:

    public: 

        std::string DCP_type;
        int numThreads = 4;
        int clahe_type = 0;

        std::string filename;
        bool debug_mode;
        bool dev_mode;
        //Public variables for Dark Channel Prior
        int dark_channel_patch_size;
        float saturation_weight,dark_channel_weight;
        float airlight_threshold;
        //Public variables for fusion
        int pyramid_level;
        std::string white_algo;

        cv::Rect roi_percent;
        //Publich Variables (used for final display)
        CaptureFrame original_image,dark_channel,en_CLAHE,en_HE,saturation,u_darkchannel;
        CaptureFrame naive_blending,image_fusion,weight_fusion,pyramid_blending;
        
        //Dark Channel Prior Algorithm
        void dark_channel_prior(CaptureFrame input);
        CaptureFrame dark_channel_prior(CaptureFrame input,int mode);
        CaptureFrame find_airlight(CaptureFrame dark_channel,CaptureFrame saturation);
        CaptureFrame find_airlight(CaptureFrame input,int radius);
        CaptureFrame show_airlight(CaptureFrame input_image);
        void find_transmission(CaptureFrame image);
        CaptureFrame recover_image(CaptureFrame input_image);
        CaptureFrame recover_image_shallow(CaptureFrame input_image);
        
        //basic simple videos
        CaptureFrame CLAHE_enhance(CaptureFrame);

        //Simplified DCP
        cv::Mat getMedianDarkChannel(cv::Mat src, int patch);
        int estimateA(cv::Mat DC);
        cv::Mat estimateTransmission(cv::Mat DCP, int ac);
        cv::Mat getDehazed(cv::Mat source, cv::Mat t, int al);
        cv::Mat SimplifiedDCP(cv::Mat frame);
        std::vector<CaptureFrame> getFrames(CaptureFrame vid, int length);
        std::vector<CaptureFrame> MultiThreadSimpleDCP(CaptureFrame input, int length);
        int framecount = 0;
    
       
        //Fusion Algorithm
        void fusion(CaptureFrame input);
        CaptureFrame fusion(CaptureFrame input , int mode);
        void normalize_weights();
        CaptureFrame fusion_blender();
        CaptureFrame pyramid_fusion();
        void video_enhance(std::string method ,CaptureFrame video);
        CaptureFrame comparison(CaptureFrame input, CaptureFrame enhanced);
        DehazeEnhance();

};

#endif