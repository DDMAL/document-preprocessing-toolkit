from gamera.plugin import *

class returnGraphWeights(PluginFunction):
    """Fills the entire image with white."""
    category = "Stable Paths Toolkit"
    return_type = Float("values")
    self_type = ImageType([ONEBIT])

class deleteStablePaths(PluginFunction):
    """Copies an image."""
    category = "Stable Paths Toolkit"
    return_type = ImageType([ONEBIT], "pathsRemoved")
    self_type = ImageType([ONEBIT])

class stablePathDetection1(PluginFunction):
    """FInds and removes all stable paths"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([ONEBIT])

class stablePathDetectionDraw(PluginFunction):
    """Draws all stable paths found."""
    category = "Stable Paths Toolkit"
    return_type = ImageType([ONEBIT], "stablePathsDrawn")
    self_type = ImageType([ONEBIT])

class findStablePaths(PluginFunction):
    """Draws the stable paths found."""
    category = "Stable Paths Toolkit"
    return_type = ImageType([ONEBIT], "pathsDrawn")
    self_type = ImageType([ONEBIT])

class displayWeights(PluginFunction):
    """Displays the image in greyscale to demonstrate weights"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([GREYSCALE])
    self_type = ImageType([ONEBIT])

class stablePaths(PluginModule):
    cpp_headers=["stable_path_staff_detection.hpp"]
    cpp_namespace=["Gamera"]
    category = "Stable_paths_toolkit"
    functions = [returnGraphWeights, deleteStablePaths, findStablePaths, stablePathDetection1, displayWeights, stablePathDetectionDraw]
    author = "Your name here"
    url = "Your URL here"
module = stablePaths()
