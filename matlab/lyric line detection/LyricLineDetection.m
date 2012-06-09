function [lyric_mask,lyric]=LyricLineDetection(image,image_nostaff,staffspace)

% This is the main function of lyric line detection. It detects the lyric
% region, but does nothing about the overlap, ascent and descent stuff.

% ----- input ------
% "image" is the original image, a logical matrix.
% "image_nostaff" is the image after staff removal, a logical matrix.
% "staffspace" is the staffspace height

% ----- output -----
% "lyric_mask" returns the region of lyric, a logical matrix
% "lyric" returns the image after applying the mask onto the original image

cc_label=bwlabel(image_nostaff,8);

% Detect and validate the baseline fragments
[local_minima,lyric_baseline]=BaselineDetection(cc_label,staffspace);

% Estimate the average lyric height by the lyric fragments computed from
% baseline fragments
[lyric_height]=LyricHeightEstimation(cc_label,local_minima,lyric_baseline,staffspace);

% Estimate the lyric region by line fit the baseline fragments and dilate
% it with the lyric height
[lyric_mask,lyric]=LyricLineFit(image,lyric_baseline,floor(lyric_height));