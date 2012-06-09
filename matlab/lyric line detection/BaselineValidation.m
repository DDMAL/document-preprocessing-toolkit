function [baseline]=BaselineValidation(baseline_seg,threshold)

% This function validate the baseline segments by checking if there are
% enough local minima within a line.
% The baseline is computed by least square line fitting.

% ----- input ------
% "baseline_seg" contains the labeled potential baseline segments. It is
% represented by local minimum vertices that are contained in potential
% baseline segments. The label indicates which segment the vertex is in.
% "threshold" defines the tolerance in y-direction of the best fit
% baseline. It is always set to staffspace.

% ----- output -----
% "baseline" contains the labeled baseline segments after
% validation. It has the same structure with input, but is re-labeled.

% ----- constant -----
ANG=30/180*pi;  % the maximum skew of baseline
THRESHOLD1=3;   % the minimum number of verices in a baseline other than the segment being checked
THRESHOLD2=6;   % the minimum nubmber of vertices in a segment to skip the line-fitting validation


number_line=2;
size_image=size(baseline_seg);
baseline=zeros(size_image);
number=max(max(baseline_seg));

for k=2:number
    % extract the k-th segment
    map1=floor(baseline_seg./k)-ones(size(baseline_seg));
    map2=mod(baseline_seg,k);
    map1=~map1;
    map2=~map2;
    cc_k=map1&map2;
    number_k=sum(sum(cc_k));
    
    % least square line fitting
    [y,x]=find(cc_k,number_k);
    x=[x ones(number_k,1)];
    line_para=lsqlin(x,y);
    
    count=0;
    if abs(line_para)<tan(ANG) % check the skew
        % count the number of vertices that can be add into this baseline
        for m=1:size_image(1)
            for n=1:size_image(2)
                if (baseline_seg(m,n)~=0)&&(baseline_seg(m,n)~=k)
                    var=abs(m-n*line_para(1)-line_para(2));
                    if var<threshold
                       cc_k(m,n)=1;
                       count=count+1;
                    end
                end
            end
        end
    end
    if (count>THRESHOLD1)||(number_k>THRESHOLD2)
        baseline=baseline+cc_k*number_line;
        number_line=number_line+1;
    end
end
            