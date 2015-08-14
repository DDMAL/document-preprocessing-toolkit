from gamera.plugin import *

class returnGraphWeights(PluginFunction):
    """Fills the entire image with white."""
    category = "Stable Paths Toolkit"
    return_type = Float("values")
    self_type = ImageType([ONEBIT])

class deleteStablePaths(PluginFunction):
    """Experimental and used for testing. Deletes one iteration of stable paths."""
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
    """Experimental and used for testing. Draws the stable paths found."""
    category = "Stable Paths Toolkit"
    return_type = ImageType([ONEBIT], "pathsDrawn")
    self_type = ImageType([ONEBIT])

class deletionStablePathDetection(PluginFunction):
    """Runs stable path staff line deletion, subtracts the imperfect image, then reruns stable staff line detection again and returns the stafflines. Does not trim."""
    category = "Stable Paths Toolkit"
    return_type = ImageType([RGB], "deletionStablePathDetection")
    self_type = ImageType([ONEBIT])

class displayWeights(PluginFunction):
    """Experimental and used for testing. Displays the image in greyscale to demonstrate weights"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([GREYSCALE])
    self_type = ImageType([ONEBIT])

class testForVerticalBlackPercentage(PluginFunction):
    """Experimental and used for testing. Should display dark/light bars based on black percentage of staff systems. WHo know if it will be useful"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([GREYSCALE])
    self_type = ImageType([ONEBIT])

class drawColorfulStablePaths(PluginFunction):
    """Displays the stable paths in sets of matching colors to demonstrate the systems found. No trimming."""
    category = "Stable Paths Toolkit"
    return_type = ImageType([RGB])
    self_type = ImageType([ONEBIT])

class stablePathDetection(PluginFunction):
    """Displays the trimmed stable paths
with_trimming:
    Trims staff sets where white space or ornamentations are found.
with_deletion: 
    If checked, the image will be processed once, keeping what it finds to be the staves and then the code is run again. More accurate for images with a lot of lyrics or ornamentation.
with_staff_fixing: 
    Uses the slopes of staff sets to fix staff lines that differ wildly from the slope at specific intervals
enable_strong_staff_pixels: 
    Experimental method that reduces the weights of vertical runs that are the exact width of staffline_height and are exactly staffspace_height away from the closest black pixel"""
    category = "Stable Paths Toolkit"
    return_type = ImageType([RGB])
    self_type = ImageType([ONEBIT])
    args = Args([Bool('with_trimming', default = True), Bool('with_deletion', default = False), Bool('with_staff_fixing', default = False), Bool('enable_strong_staff_pixels', default = False)])

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
    functions = [stablePathDetection, drawAllGraphPaths, overlayStaves, subimageTrimmedStablePaths, setOfStablePathPoints, deleteStablePaths, findStablePaths, removeStaves, deletionStablePathDetection, displayWeights, drawAllStablePaths, drawColorfulStablePaths]
    author = "Your name here"
    url = "Your URL here"
module = stablePaths()
