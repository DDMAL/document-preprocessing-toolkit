function [boundary_add_edge]=AddEdge(boundary,interval)
% This function adds edges to current edge map. Four new edges are placed along
% the four sides of the edge map, with some interval to the vertices of the map.

% ---- input ----
% "boundary" is the edge map, binary matrix.

% "interval" defines the distance between the vertex of new edge and the
% vertex of the edge map. 
% The length of each new edge is width(height) of the map - 2*interval

% ---- output ----
% "boundary_add_edge" is the new edge map, binary matrix.

boundary_add_edge=boundary;
size_image=size(boundary);

boundary_add_edge(1,interval+1:size_image(2)-interval)=ones(1,size_image(2)-2*interval);
boundary_add_edge(size_image(1),interval+1:size_image(2)-interval)=ones(1,size_image(2)-2*interval);
boundary_add_edge(interval+1:size_image(1)-interval,1)=ones(size_image(1)-2*interval,1);
boundary_add_edge(interval+1:size_image(1)-interval,size_image(2))=ones(size_image(1)-2*interval,1);