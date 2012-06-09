"""
Toolkit setup

This file is run on importing anything within this directory.
Its purpose is only to help with the Gamera GUI shell,
and may be omitted if you are not concerned with that.
"""

from gamera import toolkit
from gamera.toolkits.lyric_extraction.plugins.border_lyric import BorderLyricGenerator
from gamera.toolkits.lyric_extraction.plugins.lyricline import LyricLineGenerator