#
#
# Copyright (C) 2014 Schulich School of Music - DDMAL
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

"""lyric removal"""

from gamera.plugin import PluginFunction, PluginModule
from gamera.args import Args, ImageType, Int, Class, Float, Real
from gamera.enums import ONEBIT, RGB
import lyric_extractor_helper

class extract_lyrics(PluginFunction):  
    """
    Takes in a binarised image and attempts to remove lyrics by processing horizontal
    projections (see find_blackest_lines).  Whatever lines are found from find_blackest_lines
    are superimposed onto the connected components of the image.  Those CCs are then removed
    from the binarised image.

    Parameters:

      minimum_y_threshold: the minimum value that may be considered a local peak
      in the horizontal projection.

      num_searches: the number of searches to do around each local peak

      negative_bound: how far below the local peak to start the line search (this
      value is positive! so the value of negative_bound=10 will start searching 10
      pixels below the peak-point (or -10 pixels. To make this even more
      confusing, the negative direction is actually upward when talking about
      images, but you already knew this from reading the Gamera documentation).

      positive_bound: how far above the local peak to start the line search

      delta: see the delta parameter for the peakdet function above
    """
    pure_python = 1
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([ONEBIT])
    args = Args([Int("minimum_y_threshold", default=10),
                 Int("num_searches", default=4),
                 Int("negative_bound", default=10),
                 Int("postive_bound", default=10)])

    def __call__(self, minimum_y_threshold=10, num_searches=4, negative_bound=10, postive_bound=10):
        result = lyric_extractor_helper.extract_lyric_ccs(self, minimum_y_threshold=10, num_searches=4, negative_bound=10, postive_bound=10)
        for cc in set(result[0]) - set(result[1]):
            cc.fill_white()
        return self

    __call__ = staticmethod(__call__)

class segment_by_colour(PluginFunction):
    """
    Same as 'extract_lyrics', only the lyrics and neumes are highlighted by the specified colours.

    Parameters:

      minimum_y_threshold: the minimum value that may be considered a local peak
      in the horizontal projection.

      num_searches: the number of searches to do around each local peak

      negative_bound: how far below the local peak to start the line search (this
      value is positive! so the value of negative_bound=10 will start searching 10
      pixels below the peak-point (or -10 pixels. To make this even more
      confusing, the negative direction is actually upward when talking about
      images, but you already knew this from reading the Gamera documentation).

      positive_bound: how far above the local peak to start the line search

      delta: see the delta parameter for the peakdet function above

      neume_colour: RGB of neume highlighting; default is [0, 255, 0]

      lyric_colour: RGB of lyric highlighting; default is [255, 0, 0]
    """
    pure_python = 1
    return_type = ImageType([RGB], "output")
    self_type = ImageType([ONEBIT])
    args = Args([Int("minimum_y_threshold", default=10),
                 Int("num_searches", default=4),
                 Int("negative_bound", default=10),
                 Int("postive_bound", default=10),
                 Class("neume_colour", list, default=[0,255,0]),
                 Class("lyric_colour", list, default=[255,0,0])])

    def __call__(self, minimum_y_threshold=10, num_searches=4, negative_bound=10, postive_bound=10, neume_colour=[0, 255, 0], lyric_colour=[255, 0, 0]):
        from gamera.core import RGBPixel

        # Do analysis.
        result = lyric_extractor_helper.extract_lyric_ccs(self, minimum_y_threshold=10, num_searches=4, negative_bound=10, postive_bound=10)

        # Check color input.
        neumeColour = RGBPixel(neume_colour[0], neume_colour[1], neume_colour[2]) if len(neume_colour) else RGBPixel(0, 255, 0)
        lyricColour = RGBPixel(lyric_colour[0], lyric_colour[1], lyric_colour[2]) if len(lyric_colour) else RGBPixel(255, 0, 0)

        # Prepare output image.
        returnImage = self.to_rgb()

        # Do highlighting.
        for cc in set(result[0]) - set(result[1]):
            returnImage.highlight(cc, lyricColour)
        for cc in set(result[1]):
            returnImage.highlight(cc, neumeColour) 
        return returnImage

    __call__ = staticmethod(__call__)

class find_blackest_lines(PluginFunction):
    """
    Operates on a binarised image and a list of its horizontal projections. Finds
    local peaks in the horizontal projections of the image. This means it adds up
    the number of times black is seen in a row and stores the value for each row
    in an array. It then finds local peaks in this array. It then draws a bunch of
    lines on the image, all pivoting around the centres of the horizontal
    projections. It discards all but the lines that cross the most black pixels
    and returns the start and end points of these lines, e.g.
    [ [(x00,y00),(x01,y01)], [(x10,y10),(x11,y11)], ... [(xn0,yn0),(xn1,yn1)] ].

    Parameters:

      minimum_y_threshold: the minimum value that may be considered a local peak
      in the horizontal projection.

      num_searches: the number of searches to do around each local peak

      negative_bound: how far below the local peak to start the line search (this
      value is positive! so the value of negative_bound=10 will start searching 10
      pixels below the peak-point (or -10 pixels. To make this even more
      confusing, the negative direction is actually upward when talking about
      images, but you already knew this from reading the Gamera documentation).

      positive_bound: how far above the local peak to start the line search

      delta: see the delta parameter for the peakdet function above
    """
    self_type = ImageType([ONEBIT])
    args = Args([ Class('horizontal_projections', list),
                  Int("minimum_y_threshold"), Int("num_searches"),
                  Int("negative_bound"), Int("positive_bound"),
                  Int("delta") ])
    return_type = Float("area")
    pure_python = True

    @staticmethod
    def __call__(self, horizontal_projections, minimum_y_threshold, num_searches,
        negative_bound, positive_bound, delta=10):
      return lyric_extractor_helper._find_blackest_lines(self, horizontal_projections,
          minimum_y_threshold, num_searches, negative_bound, positive_bound,
          delta)

class count_black_under_line(PluginFunction):
    """
    Returns the number of pixels beneath a given line that are black.
    The arguments 'slope' and 'y_intercept' correspond to the m and b in the
    equation of a line, y = m * x + b, respectively. I don't know if this works
    properly because I've had it count white pixels as black ones...
    """
    self_type = ImageType([ONEBIT])
    return_type = Int("num_black_pixels")
    args = Args([Real("slope"), Real("y_intercept")])
    doc_examples = [(ONEBIT,)]

class count_black_under_line_points(PluginFunction):
    """
    Returns the number of pixels beneath a given line that are the given colour.
    The arguments x0, y0 are the start coordinates of the line and the arguments
    x1, y1 are the end coordinates of the line.
    """
    self_type = ImageType([ONEBIT])
    return_type = Int("num_black_pixels")
    args = Args([Real("x0"), Real("y0"), Real("x1"), Real("y1")])
    doc_examples = [(ONEBIT,)]

class LyricExtractor(PluginModule):
    category = "Border and Lyric Extraction"
    cpp_headers = ["find_lyrics.hpp"]
    functions = [find_blackest_lines, segment_by_colour, extract_lyrics, count_black_under_line_points, count_black_under_line]
    author = "Nicholas Esterer"
    url = "nicholas.esterer@gmail.com"

module = LyricExtractor()
