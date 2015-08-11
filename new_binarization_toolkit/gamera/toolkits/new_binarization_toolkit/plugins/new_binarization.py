from gamera.plugin import *
import _arithmetic
import _convolution

class test(PluginFunction):
    """Fills the entire image with white."""
    category = "New Binarization"
    self_type = ImageType([ONEBIT, GREYSCALE, GREY16, FLOAT, RGB])

class binarize(PluginFunction):
    """Fills the entire image with white."""
    category = "New Binarization"
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([GREYSCALE])

class pythonBinarize(PluginFunction):
    """Attempt to get binarization working using python code"""
    category = "New Binarization"
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([GREYSCALE])
    pure_python = True
    def __call__(self, scale = 1.0):
        return self
    __call__ = staticmethod(__call__)

class NewBinarization(PluginModule):
    cpp_headers=["new_binarization.hpp"]
    cpp_namespace=["Gamera"]
    category = "New_binarization_toolkit"
    functions = [test, binarize, pythonBinarize]
    author = "Your name here"
    url = "Your URL here"
module = NewBinarization()
