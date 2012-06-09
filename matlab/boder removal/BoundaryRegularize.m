function [roi,success]=BoundaryRegularize(boundary,terminate_time)

% This function returns the foreground region of image. The edges keep 
% growing and meet each other till a closed boundary of the forground is
% found. 
% To check if there's a closed bounday, flood fill the edge map by function
% "imfill". If there is one, the checking region should remain unflooeded,
% that is, zero.
% It assumes that the foreground region covers the central part of the
% image, and hence two small region in the center of the image are used as 
% samples to check if a closed

% ----- input -----
% "boundary" is the edge map, logical matrix
% "terminate_time" defines the maximum number of iterations in edge growing

% ----- output ----
% "roi" indicates the position of foreground. it is a logical matrix
% "success" equals 1 if a closed boundary is successfully reconstructed. it
% equals 0 otherwise


boundary_new=boundary;

% define the regions for checking
size_image=size(boundary);
central_size=[floor(size_image(1)/9), floor(size_image(2)/9)];
central_vertex1=3*central_size;
central_vertex2=4*central_size;
central_vertex3=6*central_size;
central_vertex4=7*central_size;

% clean up the regions for checking
boundary_new(central_vertex1(1):central_vertex2(1)-1,central_vertex1(2):central_vertex2(2)-1)=zeros(central_size(1),central_size(2));
boundary_new(central_vertex3(1):central_vertex4(1)-1,central_vertex3(2):central_vertex4(2)-1)=zeros(central_size(1),central_size(2));

% check if there's a closed boundary
fill_test=imfill(boundary_new,8);
sign1=sum(sum(fill_test(central_vertex1(1):central_vertex2(1)-1,central_vertex1(2):central_vertex2(2)-1)));
sign2=sum(sum(fill_test(central_vertex3(1):central_vertex4(1)-1,central_vertex3(2):central_vertex4(2)-1)));
sign=sign1+sign2;   % sign=0 when there is a closed boundary


count=0;  % count the number of iterations
while sign~=0
    % dilate the edges
    boundary_new=bwmorph(boundary_new,'dilate');
    % clean up the regions for checking
    boundary_new(central_vertex1(1):central_vertex2(1)-1,central_vertex1(2):central_vertex2(2)-1)=zeros(central_size(1),central_size(2));
    boundary_new(central_vertex3(1):central_vertex4(1)-1,central_vertex3(2):central_vertex4(2)-1)=zeros(central_size(1),central_size(2));
    
    if count>terminate_time
        break;
    end
    % prevent edges meet the image border
    boundary_new(:,1)=zeros(size_image(1),1);
    boundary_new(:,size_image(2))=zeros(size_image(1),1);
    boundary_new(1,:)=zeros(1,size_image(2));
    boundary_new(size_image(1),:)=zeros(1,size_image(2));
    % check if there's a closed boundary
    fill_test=imfill(boundary_new,8);
    sign1=sum(sum(fill_test(central_vertex1(1):central_vertex2(1)-1,central_vertex1(2):central_vertex2(2)-1)));
    sign2=sum(sum(fill_test(central_vertex3(1):central_vertex4(1)-1,central_vertex3(2):central_vertex4(2)-1)));
    sign=sign1+sign2;
    count=count+1;
end

if count>terminate_time
    success=0;
else
    success=1;
end

% compute the skeleton of the dilated edge map
boundary_new=bwmorph(boundary_new,'close');

for k=1:count
    boundary_new=bwmorph(boundary_new,'skel');
    boundary_new=bwmorph(boundary_new,'spur');
end

% merge regions of interest if there are two
roi=ones(size_image(1),size_image(2))-boundary_new;
roi1=bwselect(roi,central_vertex1(2),central_vertex1(1),4);
roi2=bwselect(roi,central_vertex4(2),central_vertex4(1),4);
if roi1==roi2
    roi=roi1;
else
    roi=roi1+roi2;
end

% denoise
roi=bwmorph(roi,'dilate');
roi=bwmorph(roi,'dilate');
roi=bwmorph(roi,'dilate');
roi=bwmorph(roi,'erode');
roi=bwmorph(roi,'erode');
roi=bwmorph(roi,'erode');

% fill the region of interest
roi=imfill(roi,'holes');
