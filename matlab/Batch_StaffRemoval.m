function Batch_StaffRemoval(input1,output,staffspace,staffheight,start)

% ------- input --------
% "input1" is the directory of original binary images.  Image format is
% 'tiff'
% "output" is the directory to output the image without staff
% "staffspace" contains the staffspace height of each image. It is a vector
% "staffheight" contains the staffline height of each image. It is a vector
% "start" is the first index of image to process


pngList1 = dir([input1,'/*.tiff']);
number=length(pngList1);
mkdir(output);

for i = start:number
    image_name1=[input1,'/',pngList1(i).name];
    image=imread(image_name1);
    image=~image;

    [image_nostaff, staff]=StaffRemoval(image, staffspace(i), staffheight(i));
    
    len=length(pngList1(i).name);
    name=pngList1(i).name(1:len-5);
    image_name2=[output,'/',name,'.tiff'];
    imwrite(~image_nostaff,image_name2);
    i
end