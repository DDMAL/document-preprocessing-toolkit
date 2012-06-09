function Batch_BorderRemoval(input,output,label)

% ------- input --------
% "input" is the directory of original color images.  Image format is
% 'jpg'
% "output" is the directory to output the image after applied mask
% "label" defines the color to fill in the border. It can be 'black' or
% 'white'


jpgList = dir([input,'/*.jpg']);
number=length(jpgList);
mkdir(output);

for i = 1:number
    image_name1=[input,'/',jpgList(i).name];
    image=imread(image_name1);
    image=rgb2gray(image);
    mask=BorderRemoval(image);
    [image_masked]=MaskApply(image, mask,label);
    
    len=length(jpgList(i).name);
    name=jpgList(i).name(1:len-4);
    image_name2=[output,'/',name,'_mask.png'];
    imwrite(uint8(image_masked),image_name2);
    i
end