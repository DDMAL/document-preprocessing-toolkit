function mask=BorderRemoval(image)

% This function is the main function for border removal. 

% ----- input -----
% "image" should be grayscale.

% ----- output ----
% "mask" is a logical matrix same size as image.

% ----- constant ----
TERMINATE=15;   % terminate time for boundary reconstruction
AREA_STANDARD=380*300;   % image is scaled down to this size before border removal
THRESHOLD1=[0.14 0.16];   % threshold of canny edge detector for image with dilation
THRESHOLD2=[0.11 0.13];   % threshold of canny edge detector for image without dilation


% resize image
size_image=size(image);
scalar=(AREA_STANDARD/(size_image(1)*size_image(2)))^(1/2);
image_resize=imresize(image,scalar);

% blur image
[image1_fill_med,image2_fill_med]=Blur(image_resize);

% detect edge and combine two edge maps
[boundary]=EdgeDetection(image1_fill_med,image2_fill_med,THRESHOLD1,THRESHOLD2);

% reconstruct the boundary of the page
[roi]=BoundaryRecover(boundary,TERMINATE);

% resize image
mask=imresize(roi,1/scalar);
mask=mask(1:size_image(1),1:size_image(2));