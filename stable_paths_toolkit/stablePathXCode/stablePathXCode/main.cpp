//
//  main.cpp
//  stablePathXCode
//
//  Created by Ian Karp on 2015-06-03.
//  Copyright (c) 2015 Ian Karp. All rights reserved.
//
#include <iostream>
#include "stable_path_staff_detection.hpp"

using namespace std;

int main()
{
    //Image *test1 = load_PNG
    
    OneBitImageData image_data1(Dim(500, 500));
    OneBitImageView image_view1(image_data1);
    
    int width = image_view1.ncols();
    //int height = image_view1.nrows();
    
    stableStaffLineFinder slf1 (image_data1);

    OneBitImageView *new1 = slf1.myCloneImage(image_view1);
    OneBitImageView *blank = slf1.clear(image_view1);
    slf1.lineDraw(new1);
//    slf1.constructGraphWeightsView(new1);
//    slf1.findStaffLineHeightandDistanceFinal(new1);
    vector <vector<Point> > validStaves;
    //slf1.findAllStablePathsView(new1, 0, width-1, validStaves);
    //slf1.deletePaths(validStaves, new1);
    slf1.stableStaffDetection(validStaves, *new1);
    slf1.drawPaths(validStaves, blank);
    
    cout << "Hello World" << endl;
    return 0;
}