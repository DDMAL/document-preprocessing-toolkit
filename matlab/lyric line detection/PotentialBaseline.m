function [baseline]=PotentialBaseline(local_minima,staffspace)

% This function detects the potential baseline segments. Vertices are
% connected if their weighted distance is not over a threshold.

% ------ input --------
% "local_minma" contains the labeled minimum vertices map, an integer matrix
% "staffspace" is the staffspace height

% ------ output -------
% "baseline" returns the labeled potential baseline segments. It is
% represented by local minimum vertices that are contained in potential
% baseline segments. The label indicates which segment the vertex is in.

% ----- constant ------
ANG=30*pi/180;  % threshold for direction
DIST=3.5*staffspace;   % threshold for distance
GROUP=3; % minimum number of vertices to create a new potential baseline segment


number_line=2;
box=[floor(tan(ANG)*DIST) floor(DIST)]; % the region of candidate vertex to be connected
local_minima2=padarray(local_minima,box);
size_map=size(local_minima2);
baseline=zeros(size_map);

for m=1:size_map(1)
    for n=1:size_map(2)
        % go through each unconnected vertex
        if local_minima2(m,n)~=0
            vertex_array=zeros(GROUP,2);  % keep track with the first GROUP vertices
            vertex_array(1,:)=[m n];
            sign=1; % sign=1, if a new vertex is added into the segment. Otherwise, sign=0
            count=1; % the number of vertices within a segment
            m1=m;
            n1=n;
            while sign==1
                connect=0;
                % search the candidate region for the left-most vertex that pass
                % the WeightedDistThreshold function
                for n2=n1+1:n1+box(2)
                    for m2=m1-box(1):m1+box(1)
                        if local_minima2(m2,n2)~=0
                            connect=WeightedDistThreshold([m1 n1],[m2 n2],ANG,DIST);
                            if connect==1
                                break;
                            end
                        end
                    end
                    if connect==1
                        break;
                    end
                end
                % when there's no vertex to add, end the baseline segment
                if connect==0
                    sign=0;
                else
                    % store the first GROUP vertices to vertex_array
                    if count<GROUP+1
                        vertex_array(count,:)=[m2 n2];
                        count=count+1;
                    % if the baseline is longer than GROUP, directly add
                    % new vertex into the segment, and remove the vertex
                    % from the local minimum map
                    else
                        baseline(m2,n2)=number_line;
                        local_minima2(m2,n2)=0;
                    end
                    m1=m2;
                    n1=n2;
                end
            end
            % remove the first GROUP vertices from the local minimum map
            if count==GROUP+1
                for t=1:GROUP
                    baseline(vertex_array(t,1),vertex_array(t,2))=number_line;
                    local_minima2(vertex_array(t,1),vertex_array(t,2))=0;
                end
                number_line=number_line+1;
            end
        end
    end
end
            
baseline=baseline(1+box(1):size_map(1)-box(1),1+box(2):size_map(2)-box(2));
            