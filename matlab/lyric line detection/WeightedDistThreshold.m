function connect=WeightedDistThreshold(point1,point2,ang,dist)

% This function check if the weighted distance between two candidate
% vertices is within the threshold
% The threshold function is the left part of a polynomial of degree 2. When
% x=0, y=ang; when y=0, x=dist

% ----- input --------
% the points arrage as [row col]

% ----- output -------
% "connect" returns 1 if the points pass the thresholding function, 0
% otherwise.


x=sum((point2-point1).^2)^(1/2);  % euclidean distance between the two vertices
y=atan(abs(point2(1)-point1(1))/(point2(2)-point1(2)));  % angle between the two vertices

if y<ang/dist^2*(x-dist)^2
    connect=1;
else
    connect=0;
end