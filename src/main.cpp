//Standard inlcludes
#include <unistd.h>
#include <string.h>
#include <sstream>
//User defined includes
#include "capture_frame.h"
#include "view_frame.h"
#include "dehaze_enhance.h"

int main(int argc, char **argv) //The main Function.
{
    int success = chdir("..");//Going back to root directory.(executed from bin).
    Logger logger;//Logger object for main function.

    logger.log_warn("Image Dehazing and Enhancement");//Name of the programe.

    logger.log_debug("Working directory switched to one directory back");
   
    //Changing directory for accessing files. another workaround is giving full path for each files.
    if(success != 0)//If the directory changing is unsuccessful.
    {
        logger.log_error("Unable to change working directory");//Logging error. This will be shown in console.
        return -1;    
    } 
    DehazeEnhance de_en;//DehazeEnhance object. This will be used for the subsequent operations.

    CaptureFrame input_image;//Image container for loading and processing image.
    input_image.capture_image(argv[1],"Input Image");//Read image.
    logger.log_debug("Image loaded");

    de_en.dark_channel_prior(input_image);//Enhancing by Dark channel prior method.
    // de_en.fusion(input_image);//Enhancing by fusion method.

    return 0;
}


