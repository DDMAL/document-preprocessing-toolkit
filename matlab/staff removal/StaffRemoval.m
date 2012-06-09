function [image_nostaff, staff]=StaffRemoval(image, staffspace, staffheight)

% This function removes the staff. However, it is not a standard staff
% removal algorithm, as it also removes part of the  outline of notes and lyric.

% ----- input ------
% "image" should be a logical matrix

% ----- output -----
% "image_nostaff" returns the image after staff removal
% "staff" returns the staff image

% ------ constant ------
SCALAR_MED1=[5 1.2];      % the scalar for the kernel of median filter, when the parameter is staffheight
SCALAR_MED2=[0.6 0.6/4];  % the scalar for the kernel of median filter, when the parameter is staffspace
WIN_STAFF=[1 1];          % window size for staff recover


% directional median filter
if staffheight>1
    kernel=ceil(SCALAR_MED1*staffheight);
% when staffline height is too small, define the kernel based on staffspace
% height
else
    kernel=round(SCALAR_MED2*staffspace);
end
image_nostaff0=medfilt2(image,kernel);

% staff reconstruction
staff=StaffRecover(image, image_nostaff0, WIN_STAFF);

% subtract staff from original image
image_nostaff=xor(image,staff);

