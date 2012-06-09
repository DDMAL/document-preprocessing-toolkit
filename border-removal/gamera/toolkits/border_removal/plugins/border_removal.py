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


# TODO: Add GREY16 compatibility.
# TODO: Add 8 connectivity to flood_fill_holes_grey and flood_fill_bw.

"""border removal tools."""

from gamera.plugin import *
import _border_removal


class med_filter(PluginFunction):
    """
    Returns the regional intermediate value of an image as a FLOAT.

    *region_size*
      The size of the region within which to calculate the intermediate pixel value.
    """
    return_type = ImageType([FLOAT], "output")
    self_type = ImageType([GREYSCALE, GREY16, FLOAT])
    args = Args([Int("region size", default=5)])
    doc_examples = [(GREYSCALE,), (GREY16,), (FLOAT,)]

    def __call__(self, region_size=5):
        return _border_removal.med_filter(self, region_size)
    __call__ = staticmethod(__call__)


class flood_fill_holes_grey(PluginFunction):
    """
    Fills holes in an image by flood fill.

    Algorithm reference: Luc Vincent, "Morphological Grayscale Reconstruction
    In Image Analysis: Applications and Efficient Algorithms", IEEE Transactions
    on Image Processing, vol.2, no.2, April 1993, pp. 176-201

    Note: this function only works on greyscale image.
    """
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([GREYSCALE], "output")

    def __call__(self):
        return _border_removal.flood_fill_holes_grey(self)
    __call__ = staticmethod(__call__)


class flood_fill_bw(PluginFunction):
    """
    Flood fill on binary image.

    It is different from filling holes in the way generating marker and mask images.

    Algorithm reference: Luc Vincent, "Morphological Grayscale Reconstruction
    In Image Analysis: Applications and Efficient Algorithms", IEEE Transactions
    on Image Processing, vol.2, no.2, April 1993, pp. 176-201
    """
    self_type = ImageType([ONEBIT])
    return_type = ImageType([ONEBIT], "output")

    def __call__(self):
        return _border_removal.flood_fill_bw(self)
    __call__ = staticmethod(__call__)


class paper_estimation(PluginFunction):
    """
    Returns the estimation of background paper by removing foreground pen strokes.

    Note: the default parameter works on image with standard area 114000 (rows*cols)

    Main process: filling hols -> mean filter -> median filter

    *sign*
        An extra smoothing process(dilation+erosion) is applied at the begining, when sign=1.
        An extra edge-preserving process(filling holes) is applied at the end, when sign=0.

    *dil_win*
        region size for dilation+erosion.

    *avg_win*
        region size for mean filter.

    *med_win*
        region size for median filter.
    """
    return_type = ImageType([GREYSCALE], "output")
    self_type = ImageType([GREYSCALE])
    args = Args([Int("sign", default=1),
                 Int("dil_win", default=3),
                 Int("avg_win", default=5),
                 Int("med_win", default=5)])

    def __call__(self, sign=1, win_dil=3, win_avg=5, win_med=5):
        return _border_removal.paper_estimation(self, sign, win_dil, win_avg, win_med)
    __call__ = staticmethod(__call__)


class edge_detection(PluginFunction):
    """
    Detects and combines edges from two images in different levels of smoothness.

    *image2*
        the image of same subject as current image, but in lower level of smoothness.
        (the result of "paper_estimation" with sign=0)

    *threshold1_scale*
        scale for canny edge detector on current image. See Edge->canny_edge_image for details.

    *threshold1_gradient*
        gradient for canny edge detector on current image. See Edge->canny_edge_image for details.

    *threshold2_scale*
        scale for canny edge detector on image2. See Edge->canny_edge_image for details.

    *threshold2_gradient*
        gradient for canny edge detector on image2. See Edge->canny_edge_image for details.

    *transfer_para*
        edge tranfer parameter.
        Ther higher it is, the more edges in image2 will be combined into final edge map.
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([GREYSCALE])
    args = Args([ImageType([GREYSCALE], "image2"),
                 Real("threshold1_scale", default=0.8),
                 Real("threshold1_gradient", default=6.0),
                 Real("threshold2_scale", default=0.8),
                 Real("threshold2_gradient", default=6.0),
                 Real("tranfer_parameter", default=0.25)])

    def __call__(self, image2,
                 threshold1_scale, threshold1_gradient,
                 threshold2_scale, threshold2_gradient,
                 scale_length=0.25):
        return _border_removal.edge_detection(self, image2,
                                              threshold1_scale, threshold1_gradient,
                                              threshold2_scale, threshold2_gradient,
                                              scale_length)
    __call__ = staticmethod(__call__)


class boundary_reconstruct(PluginFunction):
    """
    Reconstructs boundary of music score based on edge map from edg_detection.

    *terminate_time1*
        maximum numbers of iterations in 1st round.

    *terminate_time2*
        maximum numbers of iterations in 2nd round.

    *terminate_time3*
        maximum numbers of iterations in 3rd round.

    *interval2*
        interval for edge adding in 2nd round.

    *interval3*
        interval for edge adding in 3rd round.
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([ONEBIT])
    args = Args([Int("terminate_time1", default=15),
                 Int("terminate_time2", default=23),
                 Int("terminate_time3", default=75),
                 Int("interval2", default=45),
                 Int("interval3", default=15)])

    def __call__(self,
                 terminate_time1=15, terminate_time2=23, terminate_time3=75,
                 interval2=45, interval3=15):
        return _border_removal.boundary_reconstruct(self,
                                                    terminate_time1, terminate_time2, terminate_time3,
                                                    interval2, interval3)
    __call__ = staticmethod(__call__)


class border_removal(PluginFunction):
    """
    Returns the mask of music score region.

    Gathers paper_estimation, edge_detection and boundary_reconstruct functions.
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([GREYSCALE])
    args = Args([Int("dil_win", default=3),
                 Int("avg_win", default=5),
                 Int("med_win", default=5),
                 Real("threshold1_scale", default=0.8),
                 Real("threshold1_gradient", default=6.0),
                 Real("threshold2_scale", default=0.8),
                 Real("threshold2_gradient", default=6.0),
                 Real("tranfer_parameter", default=0.25),
                 Int("terminate_time1", default=15),
                 Int("terminate_time2", default=23),
                 Int("terminate_time3", default=75),
                 Int("interval2", default=45),
                 Int("interval3", default=15)])

    def __call__(self,
                 win_dil=3, win_avg=5, win_med=5,
                 threshold1_scale=0.8, threshold1_gradient=6.0,
                 threshold2_scale=0.8, threshold2_gradient=6.0,
                 scale_length=0.25,
                 terminate_time1=15, terminate_time2=23, terminate_time3=75,
                 interval2=45, interval3=15):
        return _border_removal.border_removal(self,
                                              win_dil, win_avg, win_med,
                                              threshold1_scale, threshold1_gradient,
                                              threshold2_scale, threshold2_gradient,
                                              scale_length,
                                              terminate_time1, terminate_time2, terminate_time3,
                                              interval2, interval3)
    __call__ = staticmethod(__call__)


class BorderRemovalGenerator(PluginModule):
    category = "BorderRemoval"
    cpp_headers = ["border_removal.hpp"]
    functions = [med_filter,
                 flood_fill_holes_grey,
                 flood_fill_bw,
                 paper_estimation,
                 edge_detection,
                 boundary_reconstruct,
                 border_removal]
    author = "Yue Phyllis Ouyang and John Ashley Burgoyne"
    url = "http://gamera.dkc.jhu.edu/"

module = BorderRemovalGenerator()
