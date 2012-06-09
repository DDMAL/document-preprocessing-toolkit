function [boundary]=BoundaryCombine(boundary1,boundary2)

% This function searchs the 2nd edge map for long edges and combines them into the 1st edge map 

% ----- input --------
% "boundary1" is the 1st edge map, logical matrix
% "boundary2" is the 2nd edge map, logical matrix

% ----- constant -----
SCALAR=0.25;   % the threshold for the minimum edge length is SCALAR*max(size_image)


boundary=boundary1;
size_image=size(boundary1);

% compute the number of connected components
boundary2(1:2,1:2)=zeros(2);
boundary2(1,1)=1;
cc2_label=bwlabel(boundary2,8);
cc2_number=max(max(cc2_label));

% define the threshold of minimum edge length
threshold=SCALAR*max(size_image);

for k=2:cc2_number
    % extract the k-th connnected component
    map1=floor(cc2_label./k)-ones(size(cc2_label));
    map2=mod(cc2_label,k);
    map1=~map1;
    map2=~map2;
    cc2_k=map1&map2;
    % compute the maximum length of straight segments
    sum_row=sum(cc2_k,1);
    sum_col=sum(cc2_k,2);
    length_row=find(sum_row,1,'last')-find(sum_row,1,'first');
    length_col=find(sum_col,1,'last')-find(sum_col,1,'first');
    if max(length_row,length_col)>threshold
        boundary=boundary+cc2_k;
    end
end

boundary=logical(boundary);