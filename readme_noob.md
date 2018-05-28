# DEHAZE AND ENHANCE
--------------------

**Planys Technologies**


## __Overview__


### Dark Channel Prior
![DCP ProgramFlow](documentation/DarkChannel_flowchart.png)

### Fusion

![fusion ProgramFlow](documentation/Fusion_flowchart.png)

_Underwater images and videos suffer from quality loss because of color shift(the blue,green,yellow,grey hue according to the water type) and less contrast(backward and forward scattering in water). This makes the image blurry and to have untrue colors_

_This program will recover the original color and contrast from the underwater image and thus enhances the image_

## This Program has two Algorithms for underwater image enhancement
### 1. Dark Channel Prior
### 2. Fusion

## DARK CHANNEL PRIOR 

DCP algorithm uses the mathematical model for the  hazed images and find air light and transmission map from given image and recover the dehazed image using these information. This method uses dark channel to find air light and transmission map.

### STEPS

1. Find Airlight(Background color)
2. Find transmission map for each channels.
3. Recover Dehazed image.



## 1. Finding Airlight

Airlight is defined as the color most hazed pixel in the image.
One method to find the background light is to take the most brightest pixel in the picture. However in the presence of sunlight or any reflective surface, this will result in bad estimation and will affect the subsequent processes considerably. 

Hazed pixel will be having higher values in dark channel and for haze-free pixels, dark channel will approach zero(with an exception for sky). Also, for background light, the value of saturation will be higher.​

So in this method, We are filtering higher values from fused image of dark channel and saturation map after a basic histogram equalization.

Finding the air light is the crucial step of this method. Subsequent steps are completely mathematical. So the quality of output and the extent of dehazing depends on obtaining the correct background light.

## 2. Finding transmission map

After finding background color, it is used to find transmission map for all the channels in the image. 

## 3. Recovering Image

Transmission maps are used to recover each channels of the image and they are are fused to get the final image.

## Fusion

 Fusion algorithm uses two enhanced images from the input image and fuses them together to form the final enhanced image.
 
 Underwater images suffer quality loss mainly in two ways

 1. Contrast loss because of forward and backward scattering
 2. Color shift becuase of preferential absorption of color underwater.

 Fusion creates contrast enhanced image and a white balanced(color balanced) image and fuses them together using 4 different weights normalized.

 ### Steps

 1. Prepare enhanced inputs
 2. Prepare weights for each inputs
 3. Normalize the weights
 4. Fuse the images with normalized weight

 ## 1. Prepare Inputs

 Here, contrast enhanced image will be obtained after applying a CLAHE filter. CLAHE enhanced the contrast preserving contrast after a histogram equalization.

 Color enhanced image is obtained by applying white balancing algorithm on the image. There are two algorithms used for white balancing.
 1.  Simple white balancing
 2. Greyworld white balancing

 ## 2. Prepare Weights

  Four different weights are used in this method
  1. Laplacian contrast
  2. Local contrast
  3. Saliency contrast
  4. Exposedness

These set of four weights will be created for both the images.

## 3. Normalize weights

Four weights will be normalized to one weight which will be used for blending. Simple averaging is used for now.

## 4. Fusing images

The last part of creating the output. Pyramid blending is used with user defined pyramid limit. It takes the input images and their normalized weights and blend them together through pyramid transformations.

