function [local_minima,lyric_baseline]=BaselineDetection(cc_label,staffspace)

% ------ input -------
% "cc_label" contains the labels of connected components in the connected
% component map.
% "staffspace" is the staffspace height

% ------ output ------
% "lyric_baseline contains the labels of lyric baseline fragments. It consists of
% local minimum vertices that are considered as part of the baseline. It is
% a integer matrix

% ------ constant ----
THRESHOLD_NOISE=15;    % the minimun area of connected components. If the area of a
                       % connected component is underneath this threshold,
                       % it is treated as noise.


% mark local minima
[local_minima]=LocalMinimum(cc_label,staffspace,THRESHOLD_NOISE);
% detect potential baseline segments
[baseline_seg0]=PotentialBaseline(local_minima,staffspace);
%[baseline_seg0]=LocalMinimumConnection(local_minima,staffspace);
% merge adjacent segments
[baseline_seg]=BaselineMerge(baseline_seg0,staffspace);
% label baseline fragments after validating potential baseline fragments
[lyric_baseline]=BaselineValidation(baseline_seg,staffspace);
