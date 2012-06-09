function [background]=BackgroundEstimation(image)

% This function estimates the background of image by median filter and
% flood fill. 

% ----- input -----
% "image" should be in grayscale

% ----- constant -----
WIN_MED=[17 17];   % the kernel for median filter. It works with image 
                      % whose size is around 1000*1000 to 2000*2000
                      
                      
size_image=size(image);
% median filter
image_med=medfilt2(image,WIN_MED);

% flood fill
image_med_pad=padarray(image_med,[1 1],0); % prevent imfill filling the background into a single level
background_pad=imfill(image_med_pad,'holes');
background=background_pad(2:size_image(1)+1,2:size_image(2)+1);
