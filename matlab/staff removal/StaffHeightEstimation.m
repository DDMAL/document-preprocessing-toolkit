function [staffspace,staffheight]=StaffHeightEstimation(image,win)
% This function estimates both staffline height and staffspace height. The 
% algorithm works based on vertical run-length coding with local
% projection.

% ----- input -------
% "image" should be a logical matrix.
% "win" defines the width of each vertical strip. Local projection is done
% within each strip.

% ----- output ------
% "staffspace" returns the staffspace height
% "staffheight" returns the staffline height

% ----- constant ----
STAFFSPACE1=5;  % the minimum height of staffline height. If the estimation is underneath this threshold, 
                % chose the next peak whose staffspace is over STAFFSPACE2 
STAFFSPACE2=10;

black=1;
white=0;

size_image=size(image);
hist_black=zeros(1,size_image(1)); % histogram for black run-length, staffline height
hist_white=zeros(1,size_image(1)); % histogram for white run-length, staffspace height

for n=1:size_image(2)-win
    % local projection
    patch=round(sum(image(:,n:n+win),2)/win);
    % check whether the first run-length is white or black
    if patch(1)==white
        sign=white;
    else
        sign=black;
    end
    pos=1;
    for m=1:size_image(1)
        % from a white segment to black segment
        if (sign==white)&&(patch(m)==black)
            height=m-pos;
            if pos~=1
                hist_white(height)=hist_white(height)+1;
            end
            sign=black;
            pos=m;
        % from a black segment to white segment
        elseif (sign==black)&&(patch(m)==white)
            height=m-pos;
            hist_black(height)=hist_black(height)+1;
            sign=white;
            pos=m;
        end
    end
end

% find the peak of white run-length to estimate the staffspace height
[value,pos1]=max(hist_white);
% when the estimation is underneath STAFFSPACE1, chose the next peak
% whose staffspace is over STAFFSPACE2
if pos1<STAFFSPACE1
    [value,index]=sort(hist_white,'descend');
    index2=find(index>STAFFSPACE2,1,'first');
    pos1=index(index2);
end
staffspace=pos1;
% find the peak of black run-length to estimate the staffline height
[value,pos2]=max(hist_black);
staffheight=pos2;
