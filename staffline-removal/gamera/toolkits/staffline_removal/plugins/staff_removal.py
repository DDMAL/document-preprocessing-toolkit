#
#
# Copyright (C) 2008Yue Phyllis Ouyang and John Ashley Burgoyne
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


"""Staff removal tools."""

from gamera.plugin import *
import _staff_removal


class directional_med_filter_bw(PluginFunction):
    """
    Returns the regional intermediate value of an image as a FLOAT.
    The shape of window is not necessarily a square.
    This function currently only works on binary image.

    *region_width*, *region_height*
      The size of the region within which to calculate the intermediate pixel value.
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([ONEBIT])
    args = Args([Int("region_width", default=5),
                 Int("region_height", default=5)])

    def __call__(self, region_width=5, region_height=5):
        return _staff_removal.directional_med_filter_bw(self, region_width, region_height)
    __call__ = staticmethod(__call__)


class staffspace_estimation(PluginFunction):
    """
    Returns the staffspace height of music score.

    *staff_win*
        width of each vertical strip. Local projection is done within each strip.

    *staffspace_threshold1*, *staffspace_threshold2*
        staffspace_threshold1 is the minimum height of staffline height. If the estimation is underneath this threshold,
        chose the next peak whose staffspace is over staffspace_threshold2
    """
    return_type = Int("output")
    self_type = ImageType([ONEBIT])
    args = Args([Int("staff_win", default=30),
                 Int("staffspace_threshold1", default=5),
                 Int("staffspace_threshold2", default=10)])

    def __call__(self, staff_win=30, staffspace_threshold1=5, staffspace_threshold2=10):
        return _staff_removal.staffspace_estimation(self, staff_win, staffspace_threshold1, staffspace_threshold2)
    __call__ = staticmethod(__call__)


class staffheight_estimation(PluginFunction):
    """
    Returns the staffline height of music score.

    *staff_win*
        width of each vertical strip. Local projection is done within each strip.

    *staffspace_threshold1*, *staffspace_threshold2*
        staffspace_threshold1 is the minimum height of staffline height. If the estimation is underneath this threshold,
        chose the next peak whose staffspace is over staffspace_threshold2
    """
    return_type = Int("output")
    self_type = ImageType([ONEBIT])
    args = Args([Int("staff_win", default=30),
                 Int("staffspace_threshold1", default=5),
                 Int("staffspace_threshold2", default=10)])

    def __call__(self, staff_win=30, staffspace_threshold1=5, staffspace_threshold2=10):
        return _staff_removal.staffheight_estimation(self, staff_win, staffspace_threshold1, staffspace_threshold2)
    __call__ = staticmethod(__call__)


class staff_removal(PluginFunction):
    """
    Removes stafflines from music scores.
    Note: it is specially for lyric line detection and cannot be used directly for standard staff removal.

    Main process: directional median filter -> reconsider pixels in neighbourhood of potential non-staff pixels

    *staffspace*
        staffspace height. If negative, estimated automatically.

    *staffheight*
        staff height. If negative, estimated automatically.

    *scalar_med_width_staffspace*, *scalar_med_height_staffspace*
        (scalar_med_width_staffspace*staffspace, scalar_med_height_staffspace*staffspace):
        region size for median filter estimated from staffspace.

    *scalar_med_width_staffheight*, *scalar_med_height_staffheight*
        (scalar_med_width_staffheight*staffheight, scalar_med_height_staffheight*staffheight):
        region size for median filter estimated from staffheight.

    *neighbour_width*, *neighbour_height*
        region size defined as neighbourhood of a pixel.

    *staff_win*
        width of each vertical strip. Local projection is done within each strip.

    *staffspace_threshold1*, *staffspace_threshold2*
        staffspace_threshold1 is the minimum height of staffline height. If the estimation is underneath this threshold,
        chose the next peak whose staffspace is over staffspace_threshold2
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([ONEBIT])
    args = Args([Int("staffspace", default=-1),
                 Int("staffheight", default=-1),
                 Real("scalar_med_width_staffspace", default=0.15),
                 Real("scalar_med_height_staffspace", default=0.6),
                 Real("scalar_med_width_staffheight", default=1.2),
                 Real("scalar_med_height_staffheight", default=5.0),
                 Int("neighbour_width", default=1),
                 Int("neighbour_height", default=1),
                 Int("staff_win", default=30),
                 Int("staffspace_threshold1", default=5),
                 Int("staffspace_threshold2", default=10)])

    def __call__(self, staffspace=-1, staffheight=-1,
                 scalar_med_width_staffspace=0.15, scalar_med_height_staffspace=0.6,
                 scalar_med_width_staffheight=1.2, scalar_med_height_staffheight=5.0,
                 neighbour_width=1, neighbour_height=1,
                 staff_win=30, staffspace_threshold1=5, staffspace_threshold2=10):
        return _staff_removal.staff_removal(self, staffspace, staffheight,
                                            scalar_med_width_staffspace, scalar_med_height_staffspace,
                                            scalar_med_width_staffheight, scalar_med_height_staffheight,
                                            neighbour_width, neighbour_height,
                                            staff_win, staffspace_threshold1, staffspace_threshold2)
    __call__ = staticmethod(__call__)


class StaffRemovalGenerator(PluginModule):
    category = "StaffRemoval"
    cpp_headers = ["staff_removal.hpp"]
    functions = [directional_med_filter_bw,
                 staffspace_estimation,
                 staffheight_estimation,
                 staff_removal]
    author = "Yue Phyllis Ouyang and John Ashley Burgoyne"
    url = "http://gamera.dkc.jhu.edu/"

module = StaffRemovalGenerator()


