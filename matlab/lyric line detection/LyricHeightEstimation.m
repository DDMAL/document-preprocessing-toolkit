function [lyric_height]=LyricHeightEstimation(cc_label,local_minima,baseline,staffspace)

% This function estimates the lyric height by averaging the height of lyric
% fragments. Those fragments are reconstructed from the baseline fragments.
% The reconstruction is the inverse processing of marking the local minima.
% See LocalMinimum.m for details of the forward processing.

% ----- input ------
% "cc_label" contains the labels of connected components in the original
% image without staff, 
% "local_minima" conations the labeled local minimum. See the definition 
% in LocalMinimum.m
% "baseline" contains the labeled baseline fragment. See the definition in
% BaselineValidation.m
% "staffspace" is the staffspace height

% ----- output -----
% with the testing code, the function can also return the lyric fragments
% it uses to estimate the lyric height

% ------ constant ------
SCALAR_HEIGHT=3;  % tolerance on lyric fragment height is defined as SCALAR_HEIGHT*staffspace
SCALAR_CC=1;  % strip width scalar, should be the same as the one in "LocalMinimum.m"


size_image=size(cc_label);
lyric_height=0;

% ----testing codes---
% lyric_map=zeros(size_image);   % lyric fragments
% --------------------

count=0;
for m=1:size_image(1)
    for n=1:size_image(2)
        if baseline(m,n)~=0
            % reconstruct local strip and extract lyric fragment
            label_index=local_minima(m,n);
            map1=floor(cc_label./label_index)-ones(size(cc_label));
            map2=mod(cc_label,label_index);
            map1=~map1;
            map2=~map2;
            cc_k=map1&map2;
            [cc_k_patch,cc_k_bound_r,cc_k_bound_c]=BoundingBox(cc_k);
            
            size_patch=size(cc_k_patch);
            strip_width=ceil(staffspace*SCALAR_CC);
            number_block=ceil(size_patch(2)/strip_width);
            block_index=floor((n-cc_k_bound_c(1)+1)./strip_width)+1;
            
            if block_index==number_block
                strip_c=[(number_block-1)*strip_width+1,size_patch(2)];
            else
                strip_c=[(block_index-1)*strip_width+1,block_index*strip_width];
            end
            % compute height of fragment
            strip_patch=cc_k_patch(:,strip_c(1):strip_c(2));
            row=find(sum(strip_patch,2)>0,1,'first');
            height=(m-cc_k_bound_r(1)+1-row);
            if height<SCALAR_HEIGHT*staffspace
                % ----testing codes---
                % lyric_map(cc_k_bound_r(1):cc_k_bound_r(2),cc_k_bound_c(1)+strip_c(1)-1:cc_k_bound_c(1)+strip_c(2)-1)=strip_patch;
                % --------------------
                lyric_height=lyric_height+height;
                count=count+1;
            end
        end
    end
end

lyric_height=lyric_height/count;
