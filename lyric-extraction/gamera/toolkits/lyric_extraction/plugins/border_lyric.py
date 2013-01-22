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


"""border removal and lyricline detection"""

from gamera.plugin import PluginFunction, PluginModule
from gamera.args import Args, ImageType, Int
from gamera.enums import ONEBIT, GREYSCALE

from gamera.toolkits.lyric_extraction.plugins.sample_histogram import sample_hist
from gamera.toolkits.border_removal.plugins.border_removal import border_removal
from gamera.toolkits.background_estimation.plugins.background_estimation import binarization


class correct_rotation(PluginFunction):
    """Corrects a possible rotation angle with the aid of skewed projections.

When the image is large, it is temporarily scaled down for performance
reasons. The parameter *staffline_height* determines how much the image
is scaled down: when it is larger than 3, the image is scaled down so that
the resulting staffline height is 2. Thus if you want to suppress the
downscaling, set *staffline_height* to one.

When *staffline_height* is given as zero, it is computed automatically
as most frequent black vertical run.

This function is adapted from Gamera MusicStaves Toolkit.
    """
    pure_python = 1
    self_type = ImageType([ONEBIT, GREYSCALE])
    args = Args([Int('staffline_height', default=0)])
    return_type = ImageType([ONEBIT, GREYSCALE], "nonrot")
    author = "Christoph Dalitz"

    def __call__(self, slh=0):
        if self.data.pixel_type != ONEBIT:
            bwtmp = self.to_onebit()
            rotatebordervalue = 255
        else:
            bwtmp = self
            rotatebordervalue = 0
        # rescale large images for performance reasons
        if slh == 0:
            slh = bwtmp.most_frequent_run('black', 'vertical')
        if (slh > 3):
            print "scale image down by factor %4.2f" % (2.0 / slh)
            bwtmp = bwtmp.scale(2.0 / slh, 0)
        # find and remove rotation
        skew = bwtmp.rotation_angle_projections(-2, 2)
        if (abs(skew[0]) > abs(skew[1])):
            print ("rot %0.4f degrees found ..." % skew[0]),
            retval = self.rotate(skew[0], rotatebordervalue)
            print "and corrected"
        else:
            print "no rotation found"
            retval = self.image_copy()
        return retval

    __call__ = staticmethod(__call__)


class border_removal_and_binarization(PluginFunction):
    """Removes color bar and other border stuff, binarizes image.
       For best performance, scale the image around 1000*1000 to 2000*2000
    """
    pure_python = 1
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([ONEBIT], "binary")

    def __call__(self):
        # border removal
        mask = border_removal(win_dil=3, win_avg=5, win_med=5,
             threshold1_scale=0.8, threshold1_gradient=6.0,
             threshold2_scale=0.8, threshold2_gradient=6.0,
             scale_length=0.25,
             terminate_time1=15, terminate_time2=23, terminate_time3=75,
             interval2=45, interval3=15)

        # binarization
        img_bw0 = binarization(mask, sample_hist, do_wiener=0, wiener_width=5, wiener_height=3, noise_variance=-1.0,
             med_size=17,
             region_size=15, sensitivity=0.5, dynamic_range=128, lower_bound=20, upper_bound=150,
             q=0.06, p1=0.7, p2=0.5)
        img_bw = img_bw0.correct_rotation(0)
        return img_bw
    __call__ = staticmethod(__call__)


class lyricline_extraction(PluginFunction):
    """Removes stafflines and extract lyriclines.
        Note: no post-processing to extract precise posistion of each lyric or deal with overlapping situation is applied.
       *binarization*
         image after border removal and binarization
       For best performance, scale the image around 1000*1000 to 2000*2000
    """
    pure_python = 1
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([ONEBIT], "lyric_mask")
    args = Args([ImageType([ONEBIT], "binarization")])

    def __call__(self, binarization):
        # border removal
        mask = border_removal(win_dil=3, win_avg=5, win_med=5,
            threshold1_scale=0.8, threshold1_gradient=6.0,
            threshold2_scale=0.8, threshold2_gradient=6.0,
            scale_length=0.25,
            terminate_time1=15, terminate_time2=23, terminate_time3=75,
            interval2=45, interval3=15)

        # binarization
        img_bw_staff0 = binarization(mask, sample_hist, do_wiener=1, wiener_width=5, wiener_height=3, noise_variance=-1.0,
            med_size=17,
            region_size=15, sensitivity=0.5, dynamic_range=128, lower_bound=20, upper_bound=150,
            q=0.06, p1=0.7, p2=0.5)
        img_bw_staff = img_bw_staff0.correct_rotation(0)

        # staff removal
        staffspace = img_bw_staff.staffspace_estimation(staff_win=30, staffspace_threshold1=5, staffspace_threshold2=10)
        staffheight = img_bw_staff.staffheight_estimation(staff_win=30, staffspace_threshold1=5, staffspace_threshold2=10)
        img_nostaff = binarization.staff_removal(staffspace, staffheight, scalar_med_width_staffspace=0.15, scalar_med_height_staffspace=0.6,
            scalar_med_width_staffheight=1.2, scalar_med_height_staffheight=5.0,
            neighbour_width=1, neighbour_height=1,
            staff_win=30, staffspace_threshold1=5, staffspace_threshold2=10)

        # lyricline detection
        return img_nostaff.lyric_line_detection(staffspace, threshold_noise=15, scalar_cc_strip=1.0,
            seg_angle_degree=30.0, scalar_seg_dist=3.5, min_group=4,
            merge_angle_degree=5.0, scalar_merge_dist=5.0,
            valid_angle_degree=20.0, scalar_valid_height=1.0, valid_min_group=8,
            scalar_height=3.0,
            fit_angle_degree=2.5, scalar_search_height=1.2,
            scalar_fit_up=1.2, scalar_fit_down=0.3)
    __call__ = staticmethod(__call__)


class BorderLyricGenerator(PluginModule):
    category = "Border and Lyric Extraction"
    cpp_headers = ["border_lyric.hpp"]
    functions = [border_removal_and_binarization,
                 lyricline_extraction,
                 correct_rotation]
    author = "Yue Phyllis Ouyang and John Ashley Burgoyne"
    url = "http://ddmal.music.mcgill.ca/"

module = BorderLyricGenerator()
