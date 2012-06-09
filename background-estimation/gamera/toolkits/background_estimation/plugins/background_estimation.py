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


"""binarization tools."""

from gamera.plugin import *
from gamera.gui import has_gui
from sample_histogram import sample_hist
import _background_estimation


class wiener2_filter(PluginFunction):
    """
    Adaptive directional filtering

    *region_width*, *region_height*
      The size of the region within which to calculate the intermediate pixel value.

    *noise_variancee*
      noise variance. If negative, estimated automatically.
    """
    return_type = ImageType([GREYSCALE, GREY16, FLOAT], "output")
    self_type = ImageType([GREYSCALE, GREY16, FLOAT])
    args = Args([Int("region_width", default=5),
         Int("region_height", default=5),
         Real("noise_variance", default=-1.0)])

    def __call__(self, region_width=5, region_height=5, noise_variance=-1.0):
        return _background_estimation.wiener2_filter(self, region_width, region_height, noise_variance)
    __call__ = staticmethod(__call__)


class background_estimation(PluginFunction):
    """
    background estimation

    *med_size*
        the kernel for median filter.
        the default value works best for image with size 1000*1000 to 2000*2000
    """
    return_type = ImageType([GREYSCALE], "output")
    self_type = ImageType([GREYSCALE])
    args = Args([Int("med_size", default=17)])

    def __call__(self, med_size=17):
        return _background_estimation.background_estimation(self, med_size)
    __call__ = staticmethod(__call__)


class optimal_histogram(PluginFunction):
    """
    returns an optimal histogram template for binarization
    """
    pure_python = 1
    self_type = ImageType([GREYSCALE, GREY16])
    return_type = FloatVector()
    doc_examples = [(GREYSCALE)]

    def __call__(image):
        return sample_hist
    __call__ = staticmethod(__call__)


class histogram_mask(PluginFunction):
    """
    Compute the histogram of the pixel values within the mask.
    Returns a Python array of doubles, with each value being a
    percentage.

    If the GUI is being used, the histogram is displayed.

    .. image:: images/histogram.png
    """
    self_type = ImageType([GREYSCALE, GREY16])
    return_type = FloatVector()
    args = Args([ImageType([ONEBIT], "mask")])
    doc_examples = [(GREYSCALE)]

    def __call__(image, mask):
        hist = _background_estimation.histogram_mask(image, mask)
        if has_gui.has_gui == has_gui.WX_GUI:
            has_gui.gui.ShowHistogram(hist, mark=image.otsu_find_threshold())
        return hist
    __call__ = staticmethod(__call__)


class equalise_histogram_mask(PluginFunction):
    """
     Normalises the histogram of the given image to match an input
     histogram within mask region
  """
    return_type = ImageType([GREYSCALE], "output")
    self_type = ImageType([GREYSCALE])
    args = Args([ImageType([ONEBIT], "mask"),
         FloatVector("reference_histogram")])

    def __call__(self, mask, reference_histogram):
        return _background_estimation.equalise_histogram_mask(self, mask, reference_histogram)
    __call__ = staticmethod(__call__)


class mask_fill(PluginFunction):
    """
    fills masked region with color
  """
    return_type = ImageType([GREYSCALE, ONEBIT], "output")
    self_type = ImageType([GREYSCALE, ONEBIT])
    args = Args([ImageType([ONEBIT], "mask"),
         Int("color")])

    def __call__(self, mask, color):
        return _background_estimation.mask_fill(self, mask, color)
    __call__ = staticmethod(__call__)


class gatos_threshold_mask(PluginFunction):
    """
    Thresholds an image according to Gatos et al.'s method. See:

    Gatos, Basilios, Ioannis Pratikakis, and Stavros
    J. Perantonis. 2004. An adaptive binarization technique for low
    quality historical documents. *Lecture Notes in Computer
    Science* 3163: 102-113.

    This version adds masking process. Only regions within the mask are binarized, the rest is filled with white color

    *background*
      Estimated background of the image.

    *binarization*
      A preliminary binarization of the image.

    *mask*
      Mask image that defines the process region

    Use the default settings for the other parameters unless you know
    what you are doing.
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([GREYSCALE])
    args = Args([ImageType([GREYSCALE], "background"),
                 ImageType([ONEBIT], "binarization"),
                 ImageType([ONEBIT], "mask"),
                 Real("q", default=0.6),
                 Real("p1", default=0.5),
                 Real("p2", default=0.8)])

    def __call__(self, background, binarization, mask, q=0.6, p1=0.5, p2=0.8):
        return _background_estimation.gatos_threshold_mask(self, \
                                            background, \
                                            binarization, \
                                            mask, \
                                            q, \
                                            p1, \
                                            p2)
    __call__ = staticmethod(__call__)


class binarization(PluginFunction):
    """

    *mask*
      Mask image that defines the process region

    *do_wiener*
      1 if adding wiener filtering before binarization, otherwise 0

    *region_width*, *region_height*, *noise_variance*
      parameters for wiener filter

    *med_size*
      The kernel for median filter.

    *region size*, *sensitivity*, *dynamic range*, *lower bound*, *upper bound*
      parameters for sauvola binarization

    *q*, *p1*, *p2*
      parameters for gatos thresholding

    the default values for wiener and median filtrs works best for image with size 1000*1000 to 2000*2000
    Use the default settings for the other parameters unless you know
    what you are doing.
    """
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([GREYSCALE])
    args = Args([ImageType([ONEBIT], "mask"),
                 FloatVector("reference_histogram"),
         Int("do_wiener", default=0),
         Int("wiener_width", default=5),
         Int("wiener_height", default=3),
         Real("noise_variance", default=-1.0),
                 Int("med_size", default=17),
         Int("region size", default=15),
                 Real("sensitivity", default=0.5),
                 Int("dynamic range", range=(1, 255), default=128),
                 Int("lower bound", range=(0, 255), default=20),
                 Int("upper bound", range=(0, 255), default=150),
                 Real("q", default=0.06),
                 Real("p1", default=0.7),
                 Real("p2", default=0.5)])

    def __call__(self, mask, reference_histogram,
         do_wiener=0, wiener_width=5, wiener_height=3, noise_variance=-1.0,
         med_size=17,
         region_size=15, sensitivity=0.5, dynamic_range=128, lower_bound=20, upper_bound=150,
         q=0.06, p1=0.7, p2=0.5):
        return _background_estimation.binarization(self, mask, reference_histogram,
                        do_wiener, wiener_width, wiener_height, noise_variance,
                        med_size, region_size, sensitivity, dynamic_range, lower_bound, upper_bound,
                        q, p1, p2)

    __call__ = staticmethod(__call__)


class BackgroundEstimationGenerator(PluginModule):
    category = "BackgroundEstimation"
    cpp_headers = ["background_estimation.hpp"]
    functions = [wiener2_filter,
         background_estimation,
         optimal_histogram,
         histogram_mask,
         equalise_histogram_mask,
         mask_fill,
         gatos_threshold_mask,
         binarization]
    author = "Yue Phyllis Ouyang and John Ashley Burgoyne"
    url = "http://gamera.dkc.jhu.edu/"

module = BackgroundEstimationGenerator()
