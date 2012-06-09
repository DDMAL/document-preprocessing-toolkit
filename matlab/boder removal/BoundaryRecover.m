function [roi]=BoundaryRecover(boundary,terminate)

% This function reconstructs the closed boundary from the edge map
% "boundary"

% ---- input ---------
% "boundary" is the edge map, logical matrix.
% "terminate" is the maximum number of iterations in the first round.

% ---- output --------
% "roi" indicates the region inside the closed boundary, logical matrix.

% ---- constant ------
SCALAR2=1.5; % scalar of terminate time in 2nd round
SCALAR3=5;  % scalar of terminate time in 3rd round
INTERVAL2=2*terminate;  % interval between the end-point of the edge added and the image border in 2nd round
INTERVAL3=terminate;  % interval between the end-point of the edge added and the image border in 3rd round


% 1st round
[roi,success]=BoundaryRegularize(boundary,terminate);

% 2nd round
if success==0
    boundary_add_edge=AddEdge(boundary,INTERVAL2);
    [roi,success]=BoundaryRegularize(boundary_add_edge,floor(SCALAR2*terminate));
end

% 3rd round
if success==0
    boundary_add_edge=AddEdge(boundary,INTERVAL3);
    [roi,success]=BoundaryRegulize(boundary_add_edge,SCALAR3*terminate);
end