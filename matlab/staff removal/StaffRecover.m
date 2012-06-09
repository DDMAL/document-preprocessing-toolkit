function [staff]=StaffRecover(image, image_nostaff, win)

% This function reconstructs the staff by considering the distance of staff
% pixels too non-staff pixel within a small window of each non-staff pixel.
% If it is close enough to the non-staff pixel, re-label it as non-staff
% pixel

% ------ input --------
% "image" is the original image, a logical matrix
% "image_nostaff" is the image after rough staff removal
% "win" defines the half size of local window as [row column].


size_image=size(image_nostaff);

staff1=xor(image,image_nostaff);
image_nostaff1=padarray(image_nostaff,win);
staff2=padarray(staff1,win);

for m=1:size_image(1)+win
    for n=1:size_image(2)+win
        if staff2(m,n)==1
            connect=sum(sum(image_nostaff1(m-win(1):m+win(1),n-win(2):n+win(2))));
            if connect>1
                staff2(m,n)=0;
            end
        end
    end
end

staff=staff2(1+win(1):size_image(1)+win(1),1+win(2):size_image(2)+win(2));