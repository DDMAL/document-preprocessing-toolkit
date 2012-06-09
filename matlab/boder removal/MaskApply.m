function [image_masked]=MaskApply(image, mask, label)

% this function superimposes the mask onto the image and return the
% bounding box containing the region of interest. The boreder can be filled
%with either black or white

% ---- input ----
% "image" should be in grayscale
% "mask" is a logical matrix indicating the position of region of interest
% "label" defines the color to fill into the border, 'black' or 'white'

mask1=255*(~mask);

switch label
    case 'white'
        image_masked=uint8(double(image)+double(mask1));
    case 'black'
        image_masked=uint8(double(image)-double(mask1));
end
[image_masked,bound_r,bound_c]=BoundingBox(image_masked);