#
#
# Copyright (C) 2008 Yue Phyllis Ouyang and John Ashley Burgoyne
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#


# TODO: Add post-processing to extract precise posistion of each lyric and deal with overlapping situation.

"""Lyrice line detectioon tools."""

from gamera.plugin import PluginFunction, PluginModule
from gamera.args import Args, ImageType, Int, Real
from gamera.enums import ONEBIT

import _lyricline


class baseline_detection(PluginFunction):
    """
    Detects the baseline of lyrics.
    Returns the local minimum vertex map of lyric baseline.

    *staffspace*
        staffspace height.

    *thershold noise*
        minimum area of connected component not considered as noise.

    *scalar_cc_strip*
        long connected components are broked into strips with certain width, scalar_cc_strip*staffspace.

    *seg_angle_degree*
        tolerance on angle between two adjacent local minimum vertices of the same line (in degree).

    *scalar_seg_dist*
        scalar_seg_dist*staffspace: tolerance on distance (in x-axis) between two adjacent local minimum vertices of the same line.

    *min_group*
        minimun number of local minimum vertices in a potential baseline segment.

    *merge_angle_degree*
        tolerance on angle between two adjacent potential baseline segments to merge into one line (in degree).

    *scalar_merge_dist*
        scalar_merge_dist*staffspace: tolerance on distance (in x-axis) between two adjacent potential baseline segments to merge into one line.

    *valid_angle_degree*
        tolerance on angle between baseline and horizon(in degree).

    *scalar_valid_height*
        scala_valid_height*staffspace: tolerance on distance (in y-axis) between local minimum vertices and the estimated baseline they belong to.

    *valid_min_group*
        minimun number of local minimum vertices in a baseline.
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([ONEBIT])
    args = Args([Real("staffspace"),
                 Int("threshold_noise", default=15),
                 Real("scalar_cc_strip", default=1.0),
                 Real("seg_angle_degree", default=30.0),
                 Real("scalar_seg_dist", default=3.5),
                 Int("min_group", default=4),
                 Real("merge_angle_degree", default=5.0),
                 Real("scalar_merge_dist", default=5.0),
                 Real("valid_angle_degree", default=20.0),
                 Real("scalar_valid_height", default=1.0),
                 Int("valid_min_group", default=8)])

    def __call__(self, staffspace,
                 threshold_noise=15, scalar_cc_strip=1.0,
                 seg_angle_degree=30.0, scalar_seg_dist=3.5, min_group=4,
                 merge_angle_degree=5.0, scalar_merge_dist=5.0,
                 valid_angle_degree=30.0, scalar_valid_height=1.0, valid_min_group=8):
        return _lyricline.baseline_detection(self, staffspace,
                                             threshold_noise, scalar_cc_strip,
                                             seg_angle_degree, scalar_seg_dist, min_group,
                                             merge_angle_degree, scalar_merge_dist,
                                             valid_angle_degree, scalar_valid_height, valid_min_group)
    __call__ = staticmethod(__call__)


class lyric_height_estimation(PluginFunction):
    """
    Returns the estimation of average lyric height.

    *baseline*
        local minimum vertex map of lyric baseline.

    *staffspace*
        staffspace height.

    *scalar_cc_strip*
        it should be the same as the parameter used in baseline_detection.

    *scalar_height*
        scala_valid_height*staffspace: maximum height of potential lyric.
    """
    return_type = Real("output")
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT], "baseline"),
                 Real("staffspace"),
                 Real("scalar_cc_strip", default=1.0),
                 Real("scalar_height", default=3.0)])

    def __call__(self, baseline, staffspace,
                 scalar_cc_strip=1.0, scalar_height=3.0):
        return _lyricline.lyric_height_estimation(self, baseline, staffspace,
                                             scalar_cc_strip, scalar_height)
    __call__ = staticmethod(__call__)


class lyric_line_fit(PluginFunction):
    """
    Returns the mask of lyric lines.
    The lines are estimated by linear least square fitting from local minimum vertex map of lyric baseline.

    *baseline*
        local minimum vertex map of lyric baseline.

    *lyric_height*
        estimation of average lyric height.

    *fit_angle_degree*
        tolerance on angle between lyric line and horizon(in degree).

    *scalar_search_height*
        scala_search_height*lyric_height: tolerance on distance (in y-axis) between local minimum vertices that belong to a single lyric line.

    *scalar_fit_up*
        scalar_fit_up*lyric_height: height of lyric portion above baseline.

    *scalar_fit_down*
        scalar_fit_down*lyric_height: height of lyric portion beneath baseline.

    Note: no post-processing to extract precise posistion of each lyric or deal with overlapping situation is applied.
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([ONEBIT])
    args = Args([ImageType([ONEBIT], "baseline"),
                 Real("lyric_height"),
                 Real("fit_angle_degree", default=2.5),
                 Real("scalar_search_height", default=1.2),
                 Real("scalar_fit_up", default=1.2),
                 Real("scalar_fit_down", default=0.3)])

    def __call__(self, baseline, lyric_height,
                 fit_angle_degree=2.5, scalar_search_height=1.2,
                 scalar_fit_up=1.2, scalar_fit_down=0.3):
        return _lyricline.lyric_line_fit(self, baseline, lyric_height,
                                            fit_angle_degree, scalar_search_height,
                                            scalar_fit_up, scalar_fit_down)
    __call__ = staticmethod(__call__)


class lyric_line_detection(PluginFunction):
    """
    Returns the mask of lyric lines.

    Gathers baseline_detection, lyric_height_estimation and lyric_line_fit functions.

    Note: no post-processing to extract precise posistion of each lyric or deal with overlapping situation is applied.
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([ONEBIT])
    args = Args([Real("staffspace"),
                 Int("threshold_noise", default=15),
                 Real("scalar_cc_strip", default=1.0),
                 Real("seg_angle_degree", default=30.0),
                 Real("scalar_seg_dist", default=3.5),
                 Int("min_group", default=4),
                 Real("merge_angle_degree", default=5.0),
                 Real("scalar_merge_dist", default=5.0),
                 Real("valid_angle_degree", default=20.0),
                 Real("scalar_valid_height", default=1.0),
                 Int("valid_min_group", default=8),
                 Real("scalar_height", default=3.0),
                 Real("fit_angle_degree", default=2.5),
                 Real("scalar_search_height", default=1.2),
                 Real("scalar_fit_up", default=1.2),
                 Real("scalar_fit_down", default=0.3)])

    def __call__(self, staffspace,
                 threshold_noise=15, scalar_cc_strip=1.0,
                 seg_angle_degree=30.0, scalar_seg_dist=3.5, min_group=4,
                 merge_angle_degree=5.0, scalar_merge_dist=5.0,
                 valid_angle_degree=20.0, scalar_valid_height=1.0, valid_min_group=8,
                 scalar_height=3.0,
                 fit_angle_degree=2.5, scalar_search_height=1.2,
                 scalar_fit_up=1.2, scalar_fit_down=0.3):
        return _lyricline.lyric_line_detection(self, staffspace,
                                             threshold_noise, scalar_cc_strip,
                                             seg_angle_degree, scalar_seg_dist, min_group,
                                             merge_angle_degree, scalar_merge_dist,
                                             valid_angle_degree, scalar_valid_height, valid_min_group,
                                             scalar_height,
                                             fit_angle_degree, scalar_search_height,
                                            scalar_fit_up, scalar_fit_down)
    __call__ = staticmethod(__call__)


class LyricLineGenerator(PluginModule):
    category = "Lyric Line Extraction"
    cpp_headers = ["lyricline.hpp"]
    functions = [baseline_detection,
                 lyric_height_estimation,
                 lyric_line_fit,
                 lyric_line_detection]
    author = "Yue Phyllis Ouyang and John Ashley Burgoyne"
    url = "http://ddmal.music.mcgill.ca/"

module = LyricLineGenerator()
