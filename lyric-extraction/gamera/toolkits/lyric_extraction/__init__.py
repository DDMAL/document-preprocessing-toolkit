# This is here so that the namespaces of ./plugins/*.py are imported simply by
# calling:
#
# from gamera.toolkits import lyric_extraction
#
# n. esterer may 2013

from gamera import toolkit
from gamera.toolkits.lyric_extraction.plugins.lyricline import\
LyricLineGenerator
from gamera.toolkits.lyric_extraction.plugins.border_lyric import\
BorderLyricGenerator

