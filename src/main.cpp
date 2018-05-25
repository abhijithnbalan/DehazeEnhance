//Standard inlcludes
#include <unistd.h>
#include <string.h>
#include <sstream>
//User defined includes
#include "capture_frame.h"
#include "view_frame.h"
#include "dehaze_enhance.h"
#include "timer.h"
//for json support -- rapidjason is used
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"
#include <fstream>

int main(int argc, char **argv) //The main Function.
{
    bool debug_mode;
    std::string running_mode, execution_mode;
    int roi_x, roi_y, roi_height, roi_width;
    int pyramid_limit;
    int dark_channel_patch_size;
    float saturation_weight, dark_channel_weight, airlight_threshold;

    int success = chdir(".."); //Going back to root directory.(executed from bin).
    Logger logger;//Logger object for main function.
 
    logger.log_info("Image Dehazing and Enhancement"); //Name of the programe.
    
        logger.log_debug("Working directory switched to one directory back");
    
    //Changing directory for accessing files. another workaround is giving full path for each files.
    if (success != 0) //If the directory changing is unsuccessful.
    {
        logger.log_error("Unable to change working directory"); //Logging error. This will be shown in console.
        return -1;
    }
    
    try
    {
        std::ifstream ifs("configure.json");

        if (ifs.fail())
            throw(20);
        rapidjson::IStreamWrapper isw(ifs);
        rapidjson::Document configuration_file;
        configuration_file.ParseStream(isw);

        debug_mode = configuration_file["Program Execution"]["DebugMode"].GetBool();
        running_mode = configuration_file["Program Execution"]["RunningMode"].GetString(); //Reading data from json
        execution_mode = configuration_file["Program Execution"]["ExeMode"].GetString();

        roi_x = configuration_file["General Settings"]["RegionOfInterest_x"].GetInt();
        roi_y = configuration_file["General Settings"]["RegionOfInterest_y"].GetInt();
        roi_width = configuration_file["General Settings"]["RegionOfInterest_width"].GetInt();
        roi_height = configuration_file["General Settings"]["RegionOfInterest_height"].GetInt();

        pyramid_limit = configuration_file["Fusion"]["PyramidLimit"].GetInt();

        dark_channel_patch_size = configuration_file["Dark Channel Prior"]["DarkChannelPatchSize"].GetInt();
        saturation_weight = configuration_file["Dark Channel Prior"]["SaturationWeight"].GetFloat();
        dark_channel_weight = configuration_file["Dark Channel Prior"]["DarkChannelWeight"].GetFloat();
        airlight_threshold = configuration_file["Dark Channel Prior"]["AirlightLimit"].GetFloat();
    }
    catch (...)
    {
        logger.log_error("Unable to open or read json file");
        std::cout << "Error in file opening or reading \"laser_calibration_values.json\" file\n";
        exit(0);
    }

    printf("\n\n\t\t\tConfiguration File\n\n"
           "Execution Settings\n"
           "Running the Program in %s in %s with Debug status : %d \n\n"
           "General Settings\n"
           "The Region of Interset is selected as (%d,%d,%d,%d)\n\n"
           "Fusion Settings\n"
           "Pyramid Limit is taken as %d\n\n"
           "Dark Channel Prior Settings\n"
           "Dark Channel Patch Size is taken as %d\n"
           "Weightage given for saturation is %.2f and for dark channel is %.2f\n"
           "Threshold for Airlight is %.2f\n\n ",
           running_mode.c_str(), execution_mode.c_str(), debug_mode, roi_x, roi_y, roi_width, roi_height, pyramid_limit, dark_channel_patch_size, saturation_weight, dark_channel_weight, airlight_threshold);
    logger.debug_mode = debug_mode;

    std::cout<<cv::getBuildInformation();
    Timer timer1;

    
    DehazeEnhance de_en; //DehazeEnhance object. This will be used for the subsequent operations.

    de_en.roi_percent.x = roi_x;
    de_en.roi_percent.y = roi_y;
    de_en.roi_percent.height = roi_height;
    de_en.roi_percent.width = roi_width;

    de_en.debug_mode = debug_mode;
    de_en.dev_mode = (execution_mode == "DEV") ? true : false;
    de_en.pyramid_level = pyramid_limit;
    de_en.dark_channel_patch_size = dark_channel_patch_size;
    de_en.dark_channel_weight = dark_channel_weight;
    de_en.saturation_weight = saturation_weight;
    de_en.airlight_threshold = airlight_threshold;

    logger.log_info("Parameters Loaded");

    CaptureFrame input_image; //Image container for loading and processing image.
    if (running_mode == "commandline")
    {
        logger.log_info("Command line mode");
        if (argc >= 4)
        {
            de_en.filename = argv[2];
            if (std::string(argv[1]) == "image")
            {
                input_image.capture_image(argv[2], "Input Image"); //Read image.
                logger.log_debug("Image loaded");

                if (std::string(argv[3]) == "dcp" || std::string(argv[3]) == "DCP")
                {
                    logger.log_warn("Image Enhancement using Dark Channel Prior");
                    de_en.dark_channel_prior(input_image); //Enhancing by Dark channel prior method.
                }

                else if (std::string(argv[3]) == "fusion")
                {
                    logger.log_warn("Image Enhancement using Fusion ");
                    de_en.fusion(input_image); //Enhancing by fusion method.
                }
                else
                    logger.log_error("Unidentified algorithm. Supported algorithms are DCP and fusion");
            }
            else if (std::string(argv[1]) == "video")
            {
                input_image.capture_video(argv[2], "video");
                logger.log_debug("video loaded");

                if (std::string(argv[3]) == "dcp" || std::string(argv[3]) == "DCP")
                {
                    logger.log_warn("Video Enhancement using Dark Channel Prior");
                    timer1.timer_init();
                    de_en.video_enhance("DCP", input_image);
                    timer1.timer_end();
                    std::cout<<"Total time taken : "<<timer1.execution_time/60<<" minutes";
                }

                else if (std::string(argv[3]) == "fusion")
                {
                    logger.log_warn("Video Enhancement using Fusion");
                    
                    timer1.timer_init();
                    de_en.video_enhance("fusion", input_image);
                    timer1.timer_end();
                    std::cout<<"Total time taken : "<<timer1.execution_time/60<<" minutes";
                }
                else
                    logger.log_error("Unidentified algorithm. Supported algorithms are DCP and fusion");
            }
            else
                logger.log_error("Unidentified input. supported inputs are image and video");
        }
        else
        {
            logger.log_error("Insufficient arguments Expected arguments in format : image/video file_location algorithm");
        }
    }
    else if (running_mode == "GUI")
    {
        //Code for GUI mode
    }
    else if (running_mode == "automated")
    {
        //Code for automated mode
    }
    else
    {
        logger.log_error("Undefined Running mode  available modes :(commandline,GUI,automated)");
        return 0;
    }

    return 1;
}
