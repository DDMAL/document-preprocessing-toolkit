function [patch,bound_r,bound_c]=BoundingBox(image)

proj_r=sum(image,2);
proj_c=sum(image,1);

bound_r=[find(proj_r,1,'first'), find(proj_r,1,'last')];
bound_c=[find(proj_c,1,'first'), find(proj_c,1,'last')];

patch=image(bound_r(1):bound_r(2),bound_c(1):bound_c(2));
