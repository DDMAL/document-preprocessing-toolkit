from gamera.plugin import *
import _clear

class clear(PluginFunction):
    """Fills the entire image with white."""
    category = "Stable Paths Toolkit"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])
    
class ClearModule(PluginModule):
    cpp_headers=["clear.hpp"]
    cpp_namespace=["Gamera"]
    category = "Stable_paths_toolkit"
    functions = [clear]
    author = "Your name here"
    url = "Your URL here"
module = ClearModule()
