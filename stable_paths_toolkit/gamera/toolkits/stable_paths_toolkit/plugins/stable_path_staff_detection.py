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

class removeStaves(PluginFunction):
    """Finds and removes all stable paths. Unless you have already computed *staffline_height* and *staffspace_height*, leave them as 0. If left as 0 they will be computed automatically."""
    category = "Stable Paths Toolkit"
    return_type = ImageType([ONEBIT], "output")
    self_type = ImageType([ONEBIT])
    args = Args([Int('staffline_height', default=0),\
                 Int('staffspace_height', default=0)])

class drawAllStablePaths(PluginFunction):
    """Draws all stable paths found, including those from before being post processed."""
    category = "Stable Paths Toolkit"
    return_type = ImageType([ONEBIT], "stablePathsDrawn")
    self_type = ImageType([ONEBIT])

class drawAllGraphPaths(PluginFunction):
    category = "Stable Paths Toolkit"
    return_type = ImageType([ONEBIT], "graphPathsDrawn")
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

class trimmedStablePaths(PluginFunction):
    """Displays the trimmed stable paths"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([RGB])
    self_type = ImageType([ONEBIT])
    args = Args([Bool('with_deletion', default = False), Bool('with_staff_fixing', default = False), Bool('enable_strong_staff_pixels', default = False)])

class subimageTrimmedStablePaths(PluginFunction):
    """Displays the trimmed stable paths for a subset of the image"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([RGB])
    self_type = ImageType([ONEBIT])
    args = Args([Point('topLeft'), Point('bottomRight'), Bool('with_deletion', default = False), Bool('with_staff_fixing', default = False), Bool('enable_strong_staff_pixels', default = False)])

class setOfStablePathPoints(PluginFunction):
    """Returns point values from sets of stable paths"""
    category = "Stable Paths Toolkit"
    return_type = Class("staffpoints")
    self_type = ImageType([ONEBIT])

class overlayStaves(PluginFunction):
    """Overlays the found staves from one image onto another image"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([RGB])
    self_type = ImageType([RGB])
    args = Args([ImageType(ONEBIT, 'Primary Image')])

class stablePaths(PluginModule):
    cpp_headers=["stable_path_staff_detection.hpp"]
    cpp_namespace=["Gamera"]
    category = "Stable_paths_toolkit"
    functions = [returnGraphWeights, trimmedStablePaths, drawAllGraphPaths, overlayStaves, subimageTrimmedStablePaths, setOfStablePathPoints, deleteStablePaths, testForVerticalBlackPercentage, findStablePaths, removeStaves, deletionStablePathDetection, displayWeights, drawAllStablePaths, drawColorfulStablePaths]
    author = "Your name here"
    url = "Your URL here"
module = stablePaths()
