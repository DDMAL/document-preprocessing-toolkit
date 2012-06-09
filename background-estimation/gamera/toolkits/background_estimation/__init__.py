"""
Toolkit setup

This file is run on importing anything within this directory.
Its purpose is only to help with the Gamera GUI shell,
and may be omitted if you are not concerned with that.
"""

from gamera import toolkit
from gamera.gui import has_gui
from gamera.toolkits.background_estimation.toolkit_icon import toolkit_icon
from gamera.toolkits.background_estimation.plugins.background_estimation import BackgroundEstimationGenerator


import wx
