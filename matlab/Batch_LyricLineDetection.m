function Batch_LyricLineDetection(input1,input2,output,staffspace,start)

% ------- input --------
% "input1" is the directory of binary images without staff. Image format is
% 'tiff'
% "input2" is the directory of original binary images Image format is
% 'tiff'
% "output" is the directory to output the lyric image
% "start" is the first index of image to process

pngList1 = dir([input1,'/*.tiff']);
pngList2 = dir([input2,'/*.tiff']);
number=length(pngList1);
mkdir(output);

for i = start:number
    image_name1=[input1,'/',pngList1(i).name];
    image_name0=[input2,'/',pngList2(i).name];
    image=imread(image_name1);
    image=~image;
    image0=imread(image_name0);
    image0=~image0;

    [lyric_mask,lyric]=LyricLineDetection(image0,image,staffspace(i));
    
    len=length(pngList1(i).name);
    name=pngList1(i).name(1:len-5);
    image_name2=[output,'/',name,'_lyric.tiff'];
    imwrite(~lyric,image_name2);
    i
end