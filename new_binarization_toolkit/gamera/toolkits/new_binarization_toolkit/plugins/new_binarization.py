from gamera.plugin import *

class test(PluginFunction):
    """Fills the entire image with white."""
    category = "New Binarization"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])

class binarize(PluginFunction):
    """Fills the entire image with white."""
    category = "New Binarization"
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([GREYSCALE])

class NewBinarization(PluginModule):
    cpp_headers=["new_binarization.hpp"]
    cpp_namespace=["Gamera"]
    category = "New_binarization_toolkit"
    functions = [test, binarize]
    author = "Your name here"
    url = "Your URL here"
module = NewBinarization()
