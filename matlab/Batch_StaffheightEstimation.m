function [staffspace,staffheight]=Batch_StaffheightEstimation(input1,win,start)

% ------- input --------
% "input1" is the directory of original binary images.  Image format is
% 'tiff'
% "win" is the window width of local projection. 30 is suggested for image
% whose size is around 1000*1000 to 2000*2000
% "start" is the first index of image to process

% ------- output -------
% "staffspace" contains the staffspace height of each processed image. It
% is a vector
% "staffheight" contains the staffline height of each processed image. It
% is a vector


pngList1 = dir([input1,'/*.tiff']);
number=length(pngList1);

staffspace=zeros(1,number);
staffheight=zeros(1,number);

for i = start:number
    image_name1=[input1,'/',pngList1(i).name];
    image=imread(image_name1);
    image=~image;

    [staffspace(i),staffheight(i)]=StaffHeightEstimation(image,win);
    
end