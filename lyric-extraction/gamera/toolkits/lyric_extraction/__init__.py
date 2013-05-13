"""
This is so you can import the toolkit into the namespace of the image object
(the image object will now be bound to these methods) by calling

from gamera.toolkits import lyric_extraction
"""

from gamera import toolkit
from gamera.toolkits.lyric_extraction.plugins.border_lyric import \
BorderLyricGenerator
from gamera.toolkits.lyric_extraction.plugins.lyricline import \
LyricLineGenerator
