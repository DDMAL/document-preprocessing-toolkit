function [boundary]=EdgeDetection(image1,image2,threshold1,threshold2)

% This function detect the edges by canny edge detectore and combing two
% edge maps into one

% ----- input ------
% image1 and image2 are grayscale images
% threshold is the threshold set for canny edge detector

% ----- output -----
% boundary is a logical matrix


% canny edge detector
boundary1=edge(uint8(image1),'canny',threshold1);
boundary2=edge(uint8(image2),'canny',threshold2);

% denoise
boundary2=bwmorph(boundary2,'close');

% combine two edge maps into one
boundary=BoundaryCombine(boundary1,boundary2);
