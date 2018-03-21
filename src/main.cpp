#include "capture_frame.h"
#include "view_frame.h"
#include "dehaze_enhance.h"
#include <unistd.h>
#include <string.h>
#include <sstream>

int main(int argc, char **argv) //The main Function
{
    int success = chdir("..");
    Logger logger;
    logger.log_warn("Image Dehazing and Enhancement");

    logger.log_debug("Working directory switched to one directory back");
   
    //Changing directory for accessing files. another workaround is giving full path for each files.
    if(success != 0)
    {
        logger.log_error("Unable to change working directory");
        std::cout<<"couldn't change the directory/\n";
        return -1;    
    } 
    DehazeEnhance de_en;
    CaptureFrame input_image;
    input_image.capture_image(argv[1],"Input Image");
    // mosaic.use_dehaze = true;
    logger.log_warn("application started");
    // de_en.dark_channel_prior(input_image);
    de_en.fusion(input_image);
    input_image.clear();

    return 0;
}


