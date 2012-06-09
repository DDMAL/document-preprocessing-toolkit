function [image_adjust, background]=BinarizationGatos2(image,hist)
% This function generates the original image and the background for gatos
% binarization. 
% This version includes wiener filter

% ------ input -------
% "image" should be in grayscale
% "hist" is the optimal histogram template to fit

% ------ output ------
% "image_adjust" is the original image for gatos thresholding
% "background" is the background image for gatos thresholding

% ----- constant -------
KERNEL=[3 5];    % horizontal kernel for wiener filter

% denoise by wiener filter
adjust=wiener2(image,KERNEL);
% histogram equalization
image_adjust=histeq(adjust,hist);
% background estiamtion
background=BackgroundEstimation(image_adjust);