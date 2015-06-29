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
    """Finds and removes all stable paths. Unless you have already computed *staffline_height* and *staffspace_height*, leave them as 0. If left as 0 they will be computed automatically."""
    category = "Stable Paths Toolkit"
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([ONEBIT])
    args = Args([Int('staffline_height', default=0),\
                 Int('staffspace_height', default=0)])

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

class deletionStablePathDetection(PluginFunction):
    """Runs stable path staff line deletion, subtracts the imperfect image, then reruns stable staff line detection again and returns the stafflines"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([RGB], "deletionStablePathDetection")
    self_type = ImageType([ONEBIT])

class displayWeights(PluginFunction):
    """Displays the image in greyscale to demonstrate weights"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([GREYSCALE])
    self_type = ImageType([ONEBIT])

class testForVerticalBlackPercentage(PluginFunction):
    """Should display dark/light bars based on black percentage of staff systems. WHo know if it will be useful"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([GREYSCALE])
    self_type = ImageType([ONEBIT])

class drawColorfulStablePaths(PluginFunction):
    """Displays the stable paths in sets of matching colors to demonstrate the systems found"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([RGB])
    self_type = ImageType([ONEBIT])

class trimmedStablePathsWithDeletion(PluginFunction):
    """Displays the trimmed stable paths"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([RGB])
    self_type = ImageType([ONEBIT])

class trimmedStablePathsWithoutDeletion(PluginFunction):
    """Displays the trimmed stable paths"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([RGB])
    self_type = ImageType([ONEBIT])

class stablePaths(PluginModule):
    cpp_headers=["stable_path_staff_detection.hpp"]
    cpp_namespace=["Gamera"]
    category = "Stable_paths_toolkit"
    functions = [returnGraphWeights, trimmedStablePathsWithDeletion, trimmedStablePathsWithoutDeletion, deleteStablePaths, testForVerticalBlackPercentage, findStablePaths, stablePathDetection1, deletionStablePathDetection, displayWeights, stablePathDetectionDraw, drawColorfulStablePaths]
    author = "Your name here"
    url = "Your URL here"
module = stablePaths()
