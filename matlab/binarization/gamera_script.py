"""
Sample Gamera script for running Gatos-style binarisations.
"""
import os, fnmatch
from gamera.core import *
init_gamera()
from gamera.toolkits import musicstaves

DIR = "/Network/Servers/borges.mt.lan/Volumes/home/ouyangy/Project/TestImages/DIAMM_local_scaled/Converted Files"
# chdir(DIR)

DIR1=DIR+"/test_adjust3"
DIR2=DIR+"/test_background2"
DIR3=DIR+"/test_bw2"
FILELIST1=os.listdir(DIR1)
FILELIST2=os.listdir(DIR2)
ext = "*.png"

Gatos_q = 0.06
Gatos_p1 = 0.7
Gatos_p2 = 0.5

for index in range(len(FILELIST1)):
    if fnmatch.fnmatch(FILELIST1[index], ext):
         print index
         FILENAME1=DIR1+"/"+FILELIST1[index]
         FILENAME2=DIR2+"/"+FILELIST2[index]
         SAVENAME1=FILELIST1[index]
         SAVENAME2=SAVENAME1[0:len(SAVENAME1)-4-7]+".tiff"
         SAVENAME=DIR3+"/"+SAVENAME2
         image = load_image(FILENAME1) 
         background = load_image(FILENAME2)
         sauvola = image.sauvola_threshold()
         bw = image.gatos_threshold(background, sauvola, q=Gatos_q, p1=Gatos_p1, p2=Gatos_p2)
         bw_r = bw.correct_rotation(0)
         bw_r.save_image(SAVENAME)

