//
//  main.cpp
//  stablePathXCode
//
//  Created by Ian Karp on 2015-06-03.
//  Copyright (c) 2015 Ian Karp. All rights reserved.
//
#include <iostream>
#include <dirent.h>
#include <fstream>
#include "stable_path_staff_detection.hpp"
#include "png_support.hpp"
#include "tiff_support.hpp"
#include <time.h>
#include <string>
#include <sstream>

using namespace std;

void getFileNames(string dirPath, vector <string> &fileNames)
{
    DIR *currDir = NULL;
    currDir = opendir(dirPath.c_str());
    struct dirent *pent = NULL;
    int counter = 0;
    
    if (currDir == NULL)
    {
        cout <<"The path you've chosen is not valid, sorry" <<endl;
        return;
    }
    
    while ((pent = readdir(currDir)))
    {
        if (pent == NULL)
        {
            cout <<"Pent is throwing a fit..." <<endl;
            return;
        }
        
        if (strcmp(pent->d_name,".") && strcmp(pent->d_name,".."))
        {
            fileNames.push_back(pent->d_name);
            cout << fileNames[counter] <<endl;
            counter++;
        }
    }
}

void runTestsOnFiles(vector <string> &inputFiles, string inputDir, string outPutDir)
{
    int size = inputFiles.size();
    ofstream myfile ("/Users/ian/Documents/Code/document-preprocessing-toolkit/stable_paths_toolkit/Testing/example.csv", ios::app);
    string line;
    time_t globalStart;
    
    if (!myfile.is_open())
    {
        cout <<"File could not be opened" <<endl;
        return;
    }
    
    for (int file = 0; file < size; file++)
    {
        string imgPath = inputDir + "/" + inputFiles[file];
        ImageInfo *imgInfo = tiff_info(imgPath.c_str());
        OneBitImageData image_data2(Dim(imgInfo->m_ncols, imgInfo->m_nrows));
        OneBitImageView image_view2(image_data2);
        tiff_load_onebit(image_view2, *imgInfo, (imgPath.c_str()));
        globalStart = time (0);
        stringstream timeTaken;
        RGBImageView *new2 = stablePathDetection(image_view2, false, false, false, false, 0, 0);
        save_tiff(*new2, (outPutDir + inputFiles[file]).c_str());
//        string timeTaken = to_string((time (0)) - globalStart);
        timeTaken << ((time (0)) - globalStart);
        myfile << inputFiles[file] + "," + (timeTaken.str()) <<endl;
    }
}

int main()
{
    vector<string> filenames;
//    ImageInfo *imgInfo = tiff_info("/Users/ian/Documents/Code/document-preprocessing-toolkit/stable_paths_toolkit/Grey.tiff");
//    GreyScaleImageData image_data_grey(Dim(imgInfo->m_ncols, imgInfo->m_nrows));
//    GreyScaleImageView image_view_grey(image_data_grey);
//    tiff_load_greyscale(image_view_grey, *imgInfo, "/Users/ian/Documents/Code/document-preprocessing-toolkit/stable_paths_toolkit/Grey.tiff");
//    
//    stableStaffLineFinderGrey slf1 (image_view_grey);
    getFileNames("/Users/ian/Documents/Code/document-preprocessing-toolkit/stable_paths_toolkit/Testing/Input", filenames);
    runTestsOnFiles(filenames, "/Users/ian/Documents/Code/document-preprocessing-toolkit/stable_paths_toolkit/Testing/Input", "/Users/ian/Documents/Code/document-preprocessing-toolkit/stable_paths_toolkit/Testing/Output/");
                    //Image *img = load_PNG("/Users/ian/Documents/Code/document-preprocessing-toolkit/stable_paths_toolkit/test1.png", 0);
                    //img->m_image_data();
                    //(img->data())->get(Point(0, 0));
                    //OneBitImageView *trialImg(img);
//    ImageInfo *imgInfo = tiff_info("/Users/ian/Documents/Code/document-preprocessing-toolkit/stable_paths_toolkit/SegFaultCauser1.tiff");
//    OneBitImageData image_data2(Dim(imgInfo->m_ncols, imgInfo->m_nrows));
//    OneBitImageView image_view2(image_data2);"/Users/ian/Documents/Code/document-preprocessing-toolkit/stable_paths_toolkit/Testing/Input"
//
//    tiff_load_onebit(image_view2, *imgInfo, "/Users/ian/Documents/Code/document-preprocessing-toolkit/stable_paths_toolkit/SegFaultCauser1.tiff");
//    
//    
//    OneBitImageData image_data1(Dim(10, 10));
//    OneBitImageView image_view1(image_data1);
    
                        //int width = image_view1.ncols();
                        //int height = image_view1.nrows();
                        
                    //    stableStaffLineFinder slf1 (image_view2);
                    //    stableStaffLineFinder slf2 (image_view1);

                    //    OneBitImageView *new1 = slf2.myCloneImage(image_view1);
                    //    OneBitImageView *blank = slf1.clear(image_view2);
                    //    slf2.lineDrawSmall(new1);
                    //    save_tiff(*new1, "/Users/ian/Documents/Code/document-preprocessing-toolkit/stable_paths_toolkit/justpathsTIFFOutput.tif");
                    //    stableStaffLineFinder slf3 (*new1);
                    //    slf1.constructGraphWeightsView(new1);
                    //    slf1.findStaffLineHeightandDistanceFinal(new1);
                    //    vector <vector<Point> > validStaves;
                        //slf1.findAllStablePathsView(new1, 0, width-1, validStaves);
                        //slf1.deletePaths(validStaves, new1);
                    //    slf1.stableStaffDetection(validStaves);
                    //    slf1.drawPaths(validStaves, blank);
                    //    RGBImageView *new2 = deletionStablePathDetection(image_view2);
    
    
//    RGBImageView *new2 = trimmedStablePaths(image_view2, true, true, true);
//    save_tiff(*new2, "/Users/ian/Documents/Code/document-preprocessing-toolkit/stable_paths_toolkit/RotationSlopeTestTIFFOutput.tif");
//    
//    cout << "Hello World" << 3 / 4 <<endl;
    return 0;
}