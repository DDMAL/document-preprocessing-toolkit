function [local_minima]=LocalMinimum(cc_label,staffspace,threshold)

% This function marks the local minimum vertices of each connected component.
% The local minima is defined as a vertex whose y-coordinate is the global
% minimum inside a strip of a connected component, and whose x-coordinate
% is the center of the strip

% ----- input ------
% "cc_label" contains the labels of connected components, an integer matrix
% "staffspace" is the staffspace height
% "threshold" defines the the minimun area of connected components not
% treated as noise

% ----- output -----
% "local_minima" contains the labels of local minima. The label is the same
% as the one of corresponding connected component. The matrix is an integer
% matrix.

% ----- constant -----
SCALAR_CC=1; % strip width is defined as SCALAR_CC*staffspace

local_minima=zeros(size(cc_label));
number=max(max(cc_label));

for k=2:number
    if mod(k,100)==0
        k
    end
    % extract the k-th connected component
    map1=floor(cc_label./k)-ones(size(cc_label));
    map2=mod(cc_label,k);
    map1=~map1;
    map2=~map2;
    cc_k=map1&map2;
    [cc_k_patch,cc_k_bound_r,cc_k_bound_c]=BoundingBox(cc_k);
    
    if sum(sum(cc_k_patch))>threshold  % filter out noise
        % compute local minimum inside the patch
        size_patch=size(cc_k_patch);
        local_min_patch=zeros(size_patch);
        % divide the patch if it is longer than staffspace*SCALAR_CC
        strip_width=ceil(staffspace*SCALAR_CC);
        number_strip=ceil(size_patch(2)/strip_width);
        
        
        % compute the global minimum in y-direction inside each strip
        % the x-coordinate of the minimum vertex is the center of the strip
        for n=1:number_strip-1
            strip_patch=cc_k_patch(:,(n-1)*strip_width+1:n*strip_width);
            row=find(sum(strip_patch,2)>0,1,'last');
            col=floor((n-0.5)*strip_width);
            local_min_patch(row,col)=k;

        end
        strip_patch=cc_k_patch(:,(number_strip-1)*strip_width+1:size_patch(2));
        row=find(sum(strip_patch,2)>0,1,'last');
        col=(number_strip-1)*strip_width+ceil(0.5*size(strip_patch,2));
        local_min_patch(row,col)=k;
    
        local_minima(cc_k_bound_r(1):cc_k_bound_r(2),cc_k_bound_c(1):cc_k_bound_c(2))=local_minima(cc_k_bound_r(1):cc_k_bound_r(2),cc_k_bound_c(1):cc_k_bound_c(2))+local_min_patch;
    end
end
    