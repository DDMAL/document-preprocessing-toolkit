function Batch_Binarization2(input,hist,output1,output2)

% ------- input --------
% "input" is the directory of original grayscale images. Image format is
% 'png'
% "output1" is the directory to output the image after image enhancement
% "output2" is the directory to output the background

pngList = dir([input,'/*.png']);
number=length(pngList);
mkdir(output1);
mkdir(output2);

for i = 1:number
    image_name1=[input,'/',pngList(i).name];
    image=imread(image_name1);
    [image_adjust, background]=BinarizationGatos2(image,hist);
    
    len=length(pngList(i).name);
    name=pngList(i).name(1:len-4);
    image_name2=[output1,'/',name,'_adjust.png'];
    image_name3=[output2,'/',name,'_back.png'];
    imwrite(uint8(image_adjust),image_name2);
    imwrite(uint8(background),image_name3);
    i
end