function [mask,lyric]=LyricLineFit(image,baseline,height)

% This function reconstructs the lyric region by linear fitting the
% baseline and dilating the baseline with the lyric height. 
% It does nothing about the overlap, ascent and descent stuff.

% ------ input ---------
% "image" is original image with staff, a logical matrix
% "baseline" contains the baseline fragments. It could be a logical matrix
% or an integer matrix
% "height" is the lyric height

% ------ output --------
% "mask" indicates the position of lyric lines. It is a logical matrix same
% size as the original image
% "lyric" returns the extracted lyric lines
% With the testing codes, the function can also returns the labeled
% baseline

% ------ constant ------
ANG=4/180*pi;    % the maximum skew of lyric line
SCALAR_UP=1.2;   % the height of lyric line region above baseline is defined as SCALAR_UP*height
SCALAR_DOWN=0.3; % the height of lyric line region beneath baseline is defined as SCALAR_UP*height


baseline1=logical(baseline);
size_image=size(baseline1);

% ----testing codes---
% baseline_group=zeros(size_image);
% --------------------

mask=zeros(size_image);

for m=1:size_image(1)
    for n=1:size_image(2)
        if baseline1(m,n)~=0
            % extract the next baseline
            k=min(m+floor(1.2*height),size_image(1));
            patch=baseline1(1:k,:);
            % ----testing codes----
            % baseline_group(1:k,:)=baseline_group(1:k,:)+patch*number_line;
            % ---------------------
            % remove the new line from the baseline fragment map
            baseline1(1:k,:)=zeros(k,size_image(2));

            number_k=sum(sum(patch));
            [y,x]=find(patch,number_k);
            if number_k<2  % if there's only 1 vertex in the line, abandon that line.
                break;
            end
            % least square line fitting
            poly_para=polyfit(x,y,1);
            if abs(poly_para(1))>tan(ANG) 
                break;
            end
            % dilate the baseline with lyric height
            for t=1:size_image(2)
                s=round(poly_para*[t 1]');
                s1=max(1,floor(s-SCALAR_UP*height));
                s2=min(s+floor(SCALAR_DOWN*height),size_image(1));
                mask(s1:s2,t)=ones(s2-s1+1,1);
            end
        end
    end
end

% apply the mask on to the original image
lyric=image&mask;
