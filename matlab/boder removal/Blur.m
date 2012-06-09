function [image1_fill_med,image2_fill_med]=Blur(image)

% This function denoises the image. Small dark patterns as notes, staves 
%and lyrics are treated as noise. First, patterns are filled with local
%foreground color. Then the image is blurred by an averaging filter.
%Finally, the image is further denoised and edges are regained by
%a median filter

% ----- input -----
% "image" is a grayscale image.

% ----- output ----
% "image1_fill_med" is the blurry version with dilation
% "image2_fill_med" is the blurry version without dilation

% ----- constant -----
WIN_DIL=3;      % radius of the kernel of dilation operator
WIN_MED=[5 5];  % kernel of median filter
WIN_AVG=5;      % radius of the average filter


% ------- blurry version with dilation ------------
% Pad the image
size_image=size(image);
image1_pad=padarray(image,[WIN_DIL, WIN_DIL],'replicate'); 

% Denoise
str= strel('ball',WIN_DIL,WIN_DIL);
image1_pad=imdilate(image1_pad,str);
image1_pad=imerode(image1_pad,str);
image1_fill=imfill(image1_pad,'holes');  % fill dark patterns with local foreground colour
image1_fill_avg=filter2(fspecial('average',WIN_AVG),image1_fill);
image1_fill_med=medfilt2(image1_fill_avg,WIN_MED);

% Return the central part, so that the  output image is the same size as
% the input image
image1_fill_med=image1_fill_med(WIN_DIL+1:size_image(1)+WIN_DIL,WIN_DIL+1:size_image(2)+WIN_DIL);


% ------- blurry version without dilation ------------
% Pad the image
image2_pad=padarray(image,[WIN_DIL,WIN_DIL],'replicate'); 

% Denoise
image2_fill=imfill(image2_pad,'holes');  % fill dark patterns with local foreground colour
image2_fill_avg=filter2(fspecial('average',WIN_AVG),image2_fill);
image2_fill_med=medfilt2(image2_fill_avg,WIN_MED);
image2_fill_med=imfill(image2_fill_med,'holes');  % fill dark patterns with local foreground colour

% Return the central part, so that the  output image is the same size as
% the input image
image2_fill_med=image2_fill_med(WIN_DIL+1:size_image(1)+WIN_DIL,WIN_DIL+1:size_image(2)+WIN_DIL);
