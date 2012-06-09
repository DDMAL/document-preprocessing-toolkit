function [baseline_group]=BaselineMerge(baseline_seg0,staffspace)

% This function merges adjacent baseline segments if they are close enough.
% The weighted distance is defined by both distance and direction

% ------ input -------
% "baseline_seg0" contains the labeled potential baseline segments. It is
% represented by local minimum vertices that are contained in potential
% baseline segments. The label indicates which segment the vertex is in.
% "staffspace" is the staffspace height

% ------ output ------
% "baseline_group" contains the labeled potential baseline segments after
% merging. It has the same structure with input, but is re-labeled.

% ------ constant ----
SCALAR=5;    % the maximum distance in x-direction between two segments is defined as SCALAR*staffspace
ANG=tan(5/180*pi);   % ANG defines the maximum difference in direction between two segments


baseline_seg1=baseline_seg0;
size_image=size(baseline_seg1);

number_line=2;
% define the candidate region to search
box=[floor(SCALAR*staffspace*tan(ANG)) floor(SCALAR*staffspace)];

baseline_group=zeros(size_image);

for n=1:size_image(2)
    for m=1:size_image(1)
        if baseline_seg1(m,n)~=0
            % extract the k-th baseline segment
            k=baseline_seg1(m,n);
            map1=floor(baseline_seg1./k)-ones(size(baseline_seg1));
            map2=mod(baseline_seg1,k);
            map1=~map1;
            map2=~map2;
            cc_k=map1&map2;
            number_k=sum(sum(cc_k));
            if number_k==0
                continue;
            end
            
            % compute the right-most x-coordinate of the baseline segment
            % and the averge y-coordinate
            n1=find(sum(cc_k,1),1,'last');
            m1=round(sum(sum(cc_k,2).*[1:size_image(1)]')/sum(sum(cc_k)));
    
            cc_k1=cc_k;

            sign=1;  % sign=1, if a new segment is added into the baseline. Otherwise, sign=0
            while sign==1
                sign=0;
                for n2=n1+1:min(size_image(2),n1+box(2))
                    for m2=m1-box(1):min(size_image(1),m1+box(1))
                        if baseline_seg1(m2,n2)~=0
                            % extract the k2-th baseline segment
                            k2=baseline_seg1(m2,n2);
                            map1=floor(baseline_seg1./k2)-ones(size(baseline_seg1));
                            map2=mod(baseline_seg1,k2);
                            map1=~map1;
                            map2=~map2;
                            % combine the new segment into the baseline
                            cc_k2=map1&map2;
                            cc_k1=cc_k1|cc_k2;
                            % remove the new segment from the segment map
                            baseline_seg1=baseline_seg1-k2*cc_k2;
                            % update the data of baseline
                            n1=find(sum(cc_k,1),1,'last');
                            m1=round(sum(sum(cc_k,2).*[1:size_image(1)]')/sum(sum(cc_k)));
                            sign=1;
                        end
                    end
                end
            end
            % label the new baseline
            baseline_group=baseline_group+number_line*cc_k1;
            % remove the k-th segment from the segment map
            baseline_seg1=baseline_seg1-k*cc_k;
            number_line=number_line+1;
        end
    end
end

