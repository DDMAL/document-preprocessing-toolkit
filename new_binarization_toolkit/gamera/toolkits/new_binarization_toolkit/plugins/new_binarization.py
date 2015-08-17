from gamera.plugin import *
import math
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

def squareRoot(image):
    image2 = image.image_copy()
    for row in range(image.nrows):
        for col in range(image.ncols):
            image2.set([col, row], int(round(math.sqrt(image.get([col, row])))))
    return image2

def checkThreshold(image, threshold_value):
    image2 = image.image_copy()
    for row in range(image.nrows):
        for col in range(image.ncols):
            if image.get([col, row]) > threshold_value:
                image2.set([col, row], 0)
            else:
                image2.set([col, row], 255)
    return image2

class pythonBinarize(PluginFunction):
    """Attempt to get binarization working using python code"""
    category = "New Binarization"
    self_type = ImageType([GREYSCALE])
    return_type = ImageType([GREYSCALE])
    args = Args([Real("scale", [0, 1e300], default = 0.8), Real("gradient_threshold", [0, 1e300], default = 4.0)])
    pure_python = True
    def __call__(self, scale, gradient_threshold):
        canny_image = _edgedetect.canny_edge_image(self, scale, gradient_threshold)
        canny_image_onebit = canny_image.to_onebit()
        
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
        lap = _arithmetic.add_images(tmp_x, tmp_y, False)

        """
           Bias high-confidence background pixels
        """
        gsmooth_image = self.convolve_xy(_convolution.GaussianKernel(scale), border_treatment = 3)
        img2 = _arithmetic.subtract_images(self, gsmooth_image, False)
        squared_img2 = _arithmetic.multiply_images(img2, img2, False)
        gsmooth_img2 = img2.convolve_xy(_convolution.GaussianKernel(scale), border_treatment = 3)
        gsmooth_squared_img2 = squared_img2.convolve_xy(_convolution.GaussianKernel(scale), border_treatment = 3)
        rms = squareRoot(gsmooth_squared_img2)
        threshold_img = checkThreshold(rms, 12)
        
        return lap
    __call__ = staticmethod(__call__)

class NewBinarization(PluginModule):
    cpp_headers=["new_binarization.hpp"]
    cpp_namespace=["Gamera"]
    category = "New_binarization_toolkit"
    functions = [test, binarize, pythonBinarize]
    author = "Your name here"
    url = "Your URL here"
module = NewBinarization()
