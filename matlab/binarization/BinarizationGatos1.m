function [image_adjust, background]=BinarizationGatos1(image,hist)
% This function generates the original image and the background for gatos
% binarization. 
% This version doesn't include wiener filter

% ------ input -------
% "image" should be in grayscale
% "hist" is the optimal histogram template to fit

% ------ output ------
% "image_adjust" is the original image for gatos thresholding
% "background" is the background image for gatos thresholding

% histogram equalization
image_adjust=histeq(image,hist);
% background estiamtion
background=BackgroundEstimation(image_adjust);