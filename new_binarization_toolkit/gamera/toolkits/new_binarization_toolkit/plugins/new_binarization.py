from gamera.plugin import *
import _arithmetic
import _convolution
import _edgedetect

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
    args = Args([Real("scale", [0, 1e300], default = 0.8), Real("gradient_threshold", [0, 1e300], default = 4.0)])
    pure_python = True
    def __call__(self, scale, gradient_threshold):
        canny_image = _edgedetect.canny_edge_image(self, scale, gradient_threshold)
        """
            LAPLACIAN OF GAUSSIAN (recreated from convolution.py)
        """
        smooth = _convolution.GaussianKernel(scale)
        deriv = _convolution.GaussianDerivativeKernel(scale, 2)
        fp = self.to_float()
        tmp = fp.convolve_x(deriv)
        tmp_x = tmp.convolve_y(smooth)
        tmp = fp.convolve_x(smooth)
        tmp_y = tmp.convolve_y(deriv)
        result = _arithmetic.add_images(tmp_x, tmp_y, False)
        return result
    __call__ = staticmethod(__call__)

class NewBinarization(PluginModule):
    cpp_headers=["new_binarization.hpp"]
    cpp_namespace=["Gamera"]
    category = "New_binarization_toolkit"
    functions = [test, binarize, pythonBinarize]
    author = "Your name here"
    url = "Your URL here"
module = NewBinarization()
