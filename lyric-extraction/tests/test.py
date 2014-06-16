from gamera.core import *
from gamera.toolkits.lyric_extraction.plugins import lyric_extractor_helper
from gamera.plugins import pil_io
import time
import sys
import os
from PIL import Image
from PIL import ImageFont
from PIL import ImageDraw
from PIL import ImageColor

# arg 1 is source file
# arg 2 is destination folder
# arg 3 is the minimum y threshold
# arg 4 is the number of searches to do at each height division
# arg 5-6 are the negative and positive height bounds around which to search
# arg 7 number of extra lines to add for each line drawn; one pixel above the original (just use 0 for a normal line)
# arg 8 is same as arg 7 on adds lines below
# example: ./source_file ./dest_dir 10 4 10 10
# will do 10 * 4 = 40 searches, each line starting from h_i = height/10 * (i + 0.5) for i
# in [0,10) and searching to end_h_j = (h_i) - 10 + (10 + 10)* (j + 0.5) / 4 for
# j in [0,4)

init_gamera()

# Ensure that image is rgb
img = load_image(sys.argv[1]).to_rgb()

# This can be replaced with a more finely tuned binarisation algorithm
onebit = img.to_onebit()

# Local peaks in the horizontal projection below this threshold value are
# ignored
minimum_y_threshold = int(sys.argv[3])

# The line over which we count the number of black pixels is pivoted this many
# times
num_searches        = int(sys.argv[4])

# On the most extreme pivot of the line, the highest end of the line (relative
# to the screen) or the lowest y-coordinate in the line (using the image
# coordinate system, hence "negative_bound") is at the y-coordinate of the local
# peak in the horizontal projection, minus the negative_bound
negative_bound      = int(sys.argv[5])

# On the most extreme pivot of the line, the lowest end of the line (relative to
# the screen) or the highest y-coordinate in the line (using the image
# coordinate system, hence "positive_bound") is at the y-coordinate of the local
# peak in the horizontal projection, plus the positive_bound
postive_bound       = int(sys.argv[6])

# The number of parallel lines to add above the original intercept line.  This simulates "thickness" above
# the line.  Use 0 for just no extra lines.
thickness_above     = int(sys.argv[7])

# The number of parallel lines to add below the original intercept line.  This simulates "thickness" above
# the line.  Use 0 for just no extra lines.
thickness_below     = int(sys.argv[8])

# Find the connected-components in the image
ccs = onebit.cc_analysis()

# Find the horizontal projections of the image. This can be replaced with an
# algorithm that considers a weighted region around the row instead of just a
# single row of the image
horizontal_projections = onebit.projection_rows()

# Find peaks in the horizontal projection
# At each peak draw "num_searches" lines, pivoting from positive bound to
# negative bound
# Choose the blackest line at each horizontal projection peak
# Output the two-point pairs for each line found (this is outputted as a list of
# point pairs, see _find_blackest_lines.py in find_lyrics.py)
lines = lyric_extractor_helper._find_blackest_lines(onebit,
                              horizontal_projections,
                              minimum_y_threshold,
                              num_searches,
                              negative_bound,
                              postive_bound,
                              10,
                              thickness_above,
                              thickness_below)

print "Blackest lines:", lines

# Convert the point pairs to (slope,y-intercept) pairs
mb_lines = [lyric_extractor_helper.slope_intercept_from_points(p0,p1) for p0, p1 in lines]

# The algorithm could perhaps find too many lines. At this point it would be
# good to have a user interface that allowed a user to delete erroneous lines.
# The settings supplied to the algorithm can be tweaked to find a correct number
# of lines, but I doubt a set of such parameters can be found for all images.

print "Number of ccs before", len(ccs)

#print "Ids of ccs before"
#for cc in ccs:
#  print cc.label

# Remove the connected components intersected by the lines, this effectively
# removes the lyrics
newccs = lyric_extractor_helper.remove_ccs_intersected_by_lines(ccs, mb_lines)

#print "Ids of ccs after"
#for cc in newccs:
#  print cc.label

print "Number of newccs after", len(newccs)

# Draw lines on the image to show which lines were found
for m, b in mb_lines:
  print "line: " + str(m) + ", " + str(b)
  print onebit.lr.x, m * onebit.lr.x + b
  img.draw_line(FloatPoint(onebit.ul.x, onebit.ul.x * m + b),
                FloatPoint(onebit.lr.x, m * onebit.lr.x + b), 
                RGBPixel(0,0,0))

# Assert that set doesn't remove ccs
print "set() doesn't remove ccs", len(set(ccs)) == len(ccs)
print "set() doesn't remove newccs", len(set(newccs)) == len(newccs)

# Here we highlight which ccs were removed, so we want to highlight the
# complement of the remaining set of ccs
#print "Highlighting ccs"
for cc in set(ccs) - set(newccs):
#  print "highlighting: ", cc.label
  img.highlight(cc, RGBPixel(255,0,0))

# Prepare a path where the resulting image will be saved to
pathprepend = sys.argv[2]

imagedir = pathprepend + "/blackest_line_annotated" + time.strftime("%y-%m-%d")

try:
  os.mkdir(imagedir)
except OSError as e:
  print "Directory already exists, but that's cool."

imagepath = imagedir + "/" + time.strftime("%H_%M_%S")\
    +"-"+os.path.basename(sys.argv[1])

# Annotate ccs
pilimg = img.to_pil()
drawer = ImageDraw.Draw(pilimg)
font = ImageFont.truetype("/System/Library/Fonts/Menlo.ttc", 12)

for cc in ccs:
  drawer.text((cc.ul.x,cc.ul.y), str(cc.label), (0,0,255), font=font)

# Save the image
pilimg.save(imagepath)
