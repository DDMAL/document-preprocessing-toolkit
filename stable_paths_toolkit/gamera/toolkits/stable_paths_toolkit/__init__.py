"""
Toolkit setup

This file is run on importing anything within this directory.
Its purpose is only to help with the Gamera GUI shell,
and may be omitted if you are not concerned with that.
"""

from gamera import toolkit
import wx
from gamera.toolkits.stable_paths_toolkit import main

# Let's import all our plugins here so that when this toolkit
# is imported using the "Toolkit" menu in the Gamera GUI
# everything works.

from gamera.toolkits.stable_paths_toolkit.plugins import clear
from gamera.toolkits.stable_paths_toolkit.plugins import stable_path_staff_detection

# You can inherit from toolkit.CustomMenu to create a menu
# for your toolkit.  Create a list of menu option in the
# member _items, and a series of callback functions that
# correspond to them.  The name of the callback function
# should be the same as the menu item, prefixed by '_On'
# and with all spaces converted to underscores.
class Stable_paths_toolkitMenu(toolkit.CustomMenu):
    _items = ["Stable_paths_toolkit Toolkit",
              "Stable_paths_toolkit Toolkit 2"]
    def _OnStable_paths_toolkit_Toolkit(self, event):
        wx.MessageDialog(None, "You clicked on Stable_paths_toolkit Toolkit!").ShowModal()
        main.main()
    def _OnStable_paths_toolkit_Toolkit_2(self, event):
        wx.MessageDialog(None, "You clicked on Stable_paths_toolkit Toolkit 2!").ShowModal()
        main.main()
stable_paths_toolkit_menu = Stable_paths_toolkitMenu()
