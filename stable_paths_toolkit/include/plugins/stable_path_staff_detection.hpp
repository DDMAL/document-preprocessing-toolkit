
  //=================== STABLE PATH FUNCTIONS ===================
  //=============================================================

  /*
    Preprocessing:
        *1. Compute staffspaceheight and stafflineheight (Need to relax values so that similar staff spaces/line heights are taken together. Also need to make it so only most common
                value from each column is considered)
        *2. Compute weights of the graph

    Main Cycle:
        *1. Compute stable paths
        *2. Validate paths with blackness and shape
        *3. Erase valid paths from image
        *4. Add valid paths to list of stafflines
        *5. End of cycle if no valid path was found

    Postprocessing
        1. Uncross stafflines
        2. Organize stafflines in staves
        3. Smooth and trim stafflines

        Notes:
            - Big difference is that in original code the values are in grayscale
            - Currently being implemented only for one bit images
    */
#ifndef STABLE_PATH_STAFF_DETECTION
#define STABLE_PATH_STAFF_DETECTION

#include <vector>
#include <algorithm>
#include <string>
#include <stdexcept>
#include "gameramodule.hpp"
#include "gamera.hpp"
#include "png.h"
#include "knnmodule.hpp"
#include "plugins/arithmetic.hpp"
#include <time.h>

using namespace std;
using namespace Gamera;

#define CUSTOM_STAFF_LINE_HEIGHT 0
#define CUSTOM_STAFF_SPACE_HEIGHT 0
#define ALLOWED_DISSIMILARITY 3
#define ALLOWED_THICKNESS_OF_STAFFLINE_DELETION 2
#define ALLOWED_DISSIMILARITY_STAFF_LINE_HEIGHT_IN_WEIGHT_CONSTRUCTION 1
#define ALLOWED_VERTICAL_BLACK_PERCENTAGE .99
#define ALLOWED_VERTICAL_HIT_PERCENTAGE .50
#define ALLOWED_OFFSET_NEARHIT 1
#define SMOOTH_STAFF_LINE_WINDOW 2
#define SLOPE_TOLERANCE 1.3


//Code heavily based on stableStaffLineFinder.h
class stableStaffLineFinder {
public:
    typedef int weight_t;
    enum e_NEIGHBOUR {NEIGHBOUR4 = 0, NEIGHBOUR8};
    typedef enum e_NEIGHBOUR NEIGHBOUR;

    struct NODE
    {
        Point previous;
        weight_t weight;
        Point start;
    };
    
    struct NODEGRAPH
    {
        weight_t weight_up;
        weight_t weight_hor;
        weight_t weight_down;
    };
    
    struct BVAL
    {
        int breakVal;
        int pixVal;
    };
    
    struct SLOPEBVAL
    {
        int breakVal;
        bool start;
    };

    //Values taken from stableStaffLineFinder.cpp lines 106-107
    const double MIN_BLACK_PER = 0.25;
    static const weight_t TOP_VALUE = (INT_MAX/2);

    int* verRun; //length of vertical run of same color pixels. 
    int* verDistance; //Minimum distance of a pixel of the same color NOT in the same run of pixels of the same color
    NODE* graphPath;
    NODEGRAPH* graphWeight;

    int staffLineHeight;
    int staffSpaceDistance;
    time_t globalStart;

    typedef ImageData<OneBitPixel> OneBitImageData;
    typedef ImageView<OneBitImageData> OneBitImageView;

    typedef ImageData<GreyScalePixel> GreyScaleImageData;
    typedef ImageView<GreyScaleImageData> GreyScaleImageView;
    
    typedef ImageData<RGBPixel> RGBImageData;
    typedef ImageView<RGBImageData> RGBImageView;
    
    OneBitImageData *primaryImageData; //Will be the data for the image being input from the python side
    OneBitImageView *primaryImage; //Will be the view for the image being input from the python side
    int imageWidth;
    int imageHeight;

    //=========================================================================================
    //                          Image Cloners/Eroders and Point Functions
    //=========================================================================================

    template <class T>
    Point getPoint(int x, T &image) //Returns the point value based on the int value x
    {
        int xValue = x % image.ncols();
        int yValue = (x - xValue) / image.ncols();
        return Point(xValue, yValue);
    }

    Point getPointView(int x, int width, int height) //Returns the point value based on the int value x
    {
        int xValue = x % width;
        int yValue = (x - xValue) / width;
        return Point(xValue, yValue);
    }

    template<class T>
    OneBitImageView* clear(T& image)
    {
        OneBitImageData* dest_data = new OneBitImageData(image.size());
        OneBitImageView* dest_view = new OneBitImageView(*dest_data);

        for (size_t r = 0; r < image.nrows(); r++)
        {
            for (size_t c = 0; c < image.ncols(); c++)
            {
                dest_view->set(Point(c, r), 0);
            }
        }

        return dest_view;
    }

    template<class T>
    GreyScaleImageView* clearGrey(T& image)
    {
        GreyScaleImageData* dest_data = new GreyScaleImageData(image.size());
        GreyScaleImageView* dest_view = new GreyScaleImageView(*dest_data);

        for (size_t r = 0; r < image.nrows(); r++)
        {
            for (size_t c = 0; c < image.ncols(); c++)
            {
                dest_view->set(Point(c, r), 255);
            }
        }

        return dest_view;
    }

    template<class T>
    OneBitImageView* myCloneImage(T &image)
    {
        OneBitImageData* dest_data = new OneBitImageData(image.size());
        OneBitImageView* dest_view = new OneBitImageView(*dest_data);

        for (size_t r = 0; r < image.nrows(); r++)
        {
            for (size_t c = 0; c < image.ncols(); c++)
            {
                dest_view->set(Point(c, r), image.get(Point(c, r)));
            }
        }

        return dest_view;
    }
    
//    template<class T>
//    OneBitImageView* myCloneImagePNG(T &image)
//    {
//        int width = image.png_get_image_width();
//        int height = image.png_get_image_height();
//        
//        OneBitImageData* dest_data = new OneBitImageData(Dim(width, height));
//        OneBitImageView* dest_view = new OneBitImageView(*dest_data);
//        
//        for (size_t r = 0; r < height; r++)
//        {
//            for (size_t c = 0; c < width; c++)
//            {
//                //dest_view->set(Point(c, r), image.get(Point(c, r)));
//            }
//        }
//        
//        return dest_view;
//    }

    void myVerticalErodeImage(OneBitImageView * img, int width, int height)
    {
        unsigned char pel_prev;
        unsigned char pel_curr;
        unsigned char pel_next;
        
        for (int c = 0; c < width; c++)
        {
            pel_prev = img->get(getPointView(c, width, height));
            pel_curr = img->get(getPointView(c, width, height));
            
            for (int r = 0; r < height - 1; r++)
            {
                int curr_pixel = (r * width) + c;
                //printf("Current Point: (%d, %d) Current Pixel Value: (%d/ %d)", c, r, curr_pixel, width*height);
                int next_row_pixel = ((r + 1) * width) + c;
                pel_next = img->get(getPointView(next_row_pixel, width, height));
                
                if (pel_prev || pel_curr || pel_next)
                {
                    img->set(getPointView(curr_pixel, width, height), 1);
                }
                
                pel_prev = pel_curr;
                pel_curr = pel_next;
            }
            
            if (pel_prev || pel_curr)
            {
                img->set(getPointView(((height - 1) * width) + c, width, height), 1);
            }
            else
            {
                img->set(getPointView(((height - 1) * width) + c, width, height), 0);
            }
        }
    }

    void printPoint(Point p)
    {
        printf("(%lu, %lu)\n", p.x(), p.y());
    }
    
    template<class T, class U>
    OneBitImageView* subtractImage(T &initialImage, U &deleteImage)
    {
        int width = initialImage.ncols();
        int height = initialImage.nrows();
        OneBitImageView *result = myCloneImage(initialImage);
        
        for (int h = 0; h < height; h++)
        {
            for (int w = 0; w < width; w++)
            {
                if (initialImage.get(Point(w, h)) == deleteImage.get(Point(w, h)))
                {
                    result->set(Point(w, h), 0);
                }
            }
        }
        
        return result;
    }

    //=========================================================================================
    //=============================Primary Functions===========================================
    //=========================================================================================
    
    template<class T>
    stableStaffLineFinder(T &image) //Initializes the stableStaffLineFinder class and its values
    {
        primaryImage = myCloneImage(image);
        imageWidth = image.ncols();
        imageHeight = image.nrows();
        
        staffLineHeight = CUSTOM_STAFF_LINE_HEIGHT; //Set to 0 unless specified by user
        staffSpaceDistance = CUSTOM_STAFF_SPACE_HEIGHT; //Set to 0 unless specified by user
        graphPath = new NODE[imageWidth * imageHeight];
        graphWeight = new NODEGRAPH[imageWidth * imageHeight];
        verRun = new int[imageWidth * imageHeight];
        verDistance = new int[imageWidth * imageHeight];
        memset (verDistance, 0, (sizeof(int) * imageWidth * imageHeight));
        
        constructGraphWeights();
    }

    ~stableStaffLineFinder ()
    {
        //myReleaseImage(&img);
        delete graphPath;
        delete graphWeight;
        //delete img;
        delete verRun;
        delete verDistance;
        //printf ("\tGLOBAL TIME %d\n\n", time (0)-globalStart);
    }
    
    void constructGraphWeights()
    {
        unsigned char WHITE = 0;
        int cols = primaryImage->ncols();
        int rows = primaryImage->nrows();
        
        //Find vertical run values
        // ***USE VECTOR ITERATORS WITH ROW ON THE OUTSIDE TO INCREASE PERFORMANCE IF THERE'S TIME***
        cout <<"About to find vertical runs" <<endl;
        
        for (int c = 0; c < cols; c++)
        {
            int run = 0;
            unsigned char val = WHITE;
            
            for (int r = 0; r < rows; r++)
            {
                unsigned char pel = primaryImage->get(Point(c, r));
                
                if (pel == val)
                {
                    run++;
                }
                else
                {
                    int len = run;
                    
                    for (int row = r - 1; len > 0; len--, row--)
                    {
                        verRun[(row * cols) + c] = run;
                    }
                    val = !val; //Changes value from 0 to 1 or from 1 to
                    run = 1;
                }
            }
            
            if (run > 0)
            {
                //Last run on the column
                int len = run;
                
                for (int row = rows - 1; len > 0; len--, row--)
                {
                    verRun[(row * cols) + c] = run;
                }
            }
        }
        cout <<"Done finding vertical runs" <<endl;
        
        //Find Vertical Distance
//        for (int c = 0; c < cols; c++)
//        {
//            for (int r = 0; r < rows; r++)
//            {
//                //cout <<"Finding vertical distance of Point(" <<c <<", " <<r <<")" <<endl;
//                unsigned char pel = primaryImage->get(Point(c, r));
//                int row = r;
//                unsigned char curr_pel = pel;
//                
//                while ((row > 0) && (curr_pel == pel))
//                {
//                    row--;
//                    curr_pel = primaryImage->get(Point(c, row));
//                }
//                
//                int run1 = 1;
//                
//                while ((row > 0) && (curr_pel != pel))
//                {
//                    row--;
//                    curr_pel = primaryImage->get(Point(c, row));
//                    run1++;
//                }
//                
//                row = r;
//                curr_pel = pel;
//                
//                while ((row < rows - 1) && (curr_pel == pel))
//                {
//                    row++;
//                    curr_pel = primaryImage->get(Point(c, row));
//                }
//                
//                int run2 = 1;
//                
//                while ((row < rows - 1) && (curr_pel != pel))
//                {
//                    row++;
//                    curr_pel = primaryImage->get(Point(c, row));
//                    run2++;
//                }
//                
//                verDistance[(r * cols) + c] = min(run1, run2);
//            }
//        }
        
        if ((!staffLineHeight) && (!staffSpaceDistance)) //No values yet assigned to staffLineHeight or staffSpaceDistance
        {
            findStaffLineHeightAndDistance();
        }
        
        //Find Graph Weights
        for (int r = 0; r < rows; r++)
        {
            for (int c = 0; c < cols - 1; c++)
            {
                graphWeight[(r * cols) + c].weight_hor = weightFunction(primaryImage, Point(c, r), Point(c + 1, r), NEIGHBOUR4);
                
                if (r > 0)
                {
                    graphWeight[(r * cols) + c].weight_up = weightFunction(primaryImage, Point(c, r), Point(c + 1, r - 1), NEIGHBOUR8);
                }
                else
                {
                    graphWeight[(r * cols) + c].weight_up = TOP_VALUE;
                }
                
                if (r < rows - 1)
                {
                    graphWeight[(r * cols) + c].weight_down = weightFunction(primaryImage, Point(c, r), Point(c + 1, r + 1), NEIGHBOUR8);
                }
                else
                {
                    graphWeight[(r * cols) + c].weight_down = TOP_VALUE;
                }
            }
        }
        
        // for (int x = 0; x < rows * cols; x++)
        // {
        //     printf("Value: %d -> verDistance: %d -> verRun: %d\n", x, verDistance[x], verRun[x]);
        // }
    }
    
    weight_t weightFunction(OneBitImageView *image, Point pixelVal1, Point pixelVal2, NEIGHBOUR neigh)
    {
        unsigned int pel1 = primaryImage->get(pixelVal1); //Gets the pixel value of Point 1
        unsigned int pel2 = primaryImage->get(pixelVal2); //Gets pixel value of Point 2
        int width = imageWidth;
        
//        int dist1 = verDistance[(pixelVal1.y() * width) + pixelVal1.x()]; //Vertical Distance taken from array of values created in constructGraphWeights
//        int dist2 = verDistance[(pixelVal2.y() * width) + pixelVal2.x()];
        int vRun1 = verRun[(pixelVal1.y() * width) + pixelVal1.x()]; //Vertical Runs taken from array of values created in constructGraphWeights
        int vRun2 = verRun[(pixelVal2.y() * width) + pixelVal2.x()];
        
        int pel = max(pel1, pel2); //pel set to 1 if either pixel is black
        
        //Weights for a 4-Neighborhood
        int y1 = 4; //Black pixels
        int y0 = 8; //White pixels
        
        if (neigh == NEIGHBOUR8) //Weights for an 8-Neighborhood
        {
            y1 = 6; //Black
            y0 = 12; //White
        }
        
        int y = (pel == 0 ? y0:y1);
        
        if ( (pel) && ( (min(vRun1, vRun2) <= (staffLineHeight * ALLOWED_DISSIMILARITY_STAFF_LINE_HEIGHT_IN_WEIGHT_CONSTRUCTION))))
        {
            --y;
        }
        
//        if (max(dist1, dist2) > ((2 * staffLineHeight) + staffSpaceDistance))
//        {
//            y++;
//        }
        
        return y;
    }
    
    void findStaffLineHeightAndDistance()
    {
        int width = imageWidth;
        int height = imageHeight;
        vector<int> values;
        vector<int> runs[2];
        vector<int> sum2runs;
        runs[0].resize(height + 1, 0);
        runs[1].resize(height + 1, 0);
        
        for (int c = 0; c < width; c++)
        {
            for (int r = 0; r < height; r++)
            {
                unsigned int pel = primaryImage->get(Point(c, r));
                
                if (pel) //Pixel is black
                {
                    ++runs[1][verRun[(r * width) + c]];
                    r += verRun[(r * width) + c] - 1;
                }
                else //Pixel is white
                {
                    ++runs[0][verRun[(r * width) + c]];
                    r += verRun[(r * width) + c] - 1;
                }
            }
        }
        
        //Find most repeated
        {
            int maxcounter = 0;
            
            for (int i = 0; i < runs[0].size(); i++)
            {
                if (runs[0][i] >= maxcounter)
                {
                    maxcounter = runs[0][i];
                    staffSpaceDistance = i;
                }
            }
        }
        
        {
            int maxcounter = 0;
            
            for (int i = 0; i < runs[1].size(); i++)
            {
                if (runs[1][i] >= maxcounter)
                {
                    maxcounter = runs[1][i];
                    staffLineHeight = i;
                }
            }
        }
        
        cout <<"Staff Height = " <<staffLineHeight <<" Staff Distance = " <<staffSpaceDistance <<endl;
    }
    
    OneBitImageView* stableStaffDetection(vector <vector <Point> > &validStaves)
    {
        OneBitImageView *imageCopy = myCloneImage(*primaryImage);
        OneBitImageView *imgErode = myCloneImage(*primaryImage);
        OneBitImageView *imageErodedCopy = myCloneImage(*primaryImage);
        myVerticalErodeImage(imgErode, imageWidth, imageHeight);
        myVerticalErodeImage(imageErodedCopy, imageWidth, imageHeight);
        
        vector<int> npaths;
        
        bool first_time = 1;
        double blackPerc;
        vector<Point> bestStaff;
        
        int nrows = imageHeight;
        int ncols = imageWidth;
        
        while(1)
        {
            vector <vector<Point> > stablePaths;
            int curr_n_paths = 0;
            printf("About to findAllStablePaths\n");
            findAllStablePaths(imageCopy, 0, ncols - 1, stablePaths);
            printf("Finished findAllStablePaths. Size = %lu\n", stablePaths.size());
            
            if (first_time && (stablePaths.size() > 0))
            {
                first_time = 0;
                bestStaff.clear();
                size_t bestSumOfValues = INT_MAX;
                size_t bestStaffIdx = 0;
                vector<size_t> allSumOfValues;
                
                for (size_t c = 0; c < stablePaths.size(); c++)
                {
                    size_t sumOfValues = sumOfValuesInVector(stablePaths[c], imgErode);
                    
                    if ((sumOfValues / (1.0 * (stablePaths[c].size()))) > MIN_BLACK_PER) //Checks to make sure percentage of black values are larger than the minimum black percentage allowed
                    {
                        allSumOfValues.push_back(sumOfValues);
                    }
                    
                    if (sumOfValues < bestSumOfValues)
                    {
                        bestSumOfValues = sumOfValues;
                        bestStaffIdx = c;
                    }
                }
                
                if (!allSumOfValues.size())
                {
                    cout <<"There are no valid stable paths in your image. Sorry.\n";
                    return imageCopy;
                }
                
                vector<size_t> copy_allSumOfValues = allSumOfValues;
                sort(allSumOfValues.begin(), allSumOfValues.end());
                size_t medianSumOfValues = allSumOfValues[allSumOfValues.size()/2];
                int i;
                
                for (i = 0; i < allSumOfValues.size(); i++)
                {
                    printf("copy_allSumOfValues[%d] = %lu\n", i, copy_allSumOfValues[i]);
                    
                    if (copy_allSumOfValues[i] == medianSumOfValues)
                    {
                        break;
                    }
                }
                
                bestStaff = stablePaths[i];
                
                double bestBlackPerc = medianSumOfValues/(1.0 * bestStaff.size());
                blackPerc = max(MIN_BLACK_PER, bestBlackPerc * 0.8);
                cout <<"bestBlackPerc: " <<bestBlackPerc <<" blackPerc: " <<blackPerc <<endl;
            }
            
            for (size_t i = 0; i < stablePaths.size(); i++)
            {
                vector<Point> staff = stablePaths[i];
                
                if (tooMuchWhite(staff, imgErode, blackPerc))
                {
                    cout <<"Too white, line being removed\n";
                    continue;
                }
                
                double dissimilarity = staffDissimilarity(bestStaff, staff);
                printf ("\tDissimilarity = %f, staffSpaceDistance = %d\n", dissimilarity, staffSpaceDistance);
                if (dissimilarity > (ALLOWED_DISSIMILARITY * staffSpaceDistance))
                {
                    printf ("\tToo Dissimilar. Dissimilarity = %f, staffSpaceDistance = %d\n", dissimilarity, staffSpaceDistance);
                    continue;
                }
                
                validStaves.push_back(staff);
                curr_n_paths++;
                
                int path_half_width2 = max(staffLineHeight, staffSpaceDistance/2);
                
                //Erasing paths from our image, must create a copy of our image
                for (size_t i = 0; i < staff.size(); i++)
                {
                    //printf("Staff Size: %lu\n", staff.size());
                    int col = staff[i].x();
                    int row = staff[i].y();
                    
                    //ERASE PATHS ALREADY SELECTED!
                    for (int j = -path_half_width2; j <= path_half_width2; j++)
                    {
                        // printf("path_half_width2 = %d, j = %d\n", path_half_width2, j);
                        // printf("Currently erasing lines\n");
                        if ( ((row + j) > nrows - 1) || ((row + j) < 0) )
                        {
                            continue;
                        }
                        
//                        If a vertical run of pixels that is less than some value times the staffLineHeight is along the path, set it to white
                        if (verRun[((row + j) * ncols) + col] < ((ALLOWED_THICKNESS_OF_STAFFLINE_DELETION * staffLineHeight) + 1)) //1 os added for specific case where stafflineheight is 1 and fluctuates by 2 or 3 pixels
                        {
                            imageCopy->set(Point(col, (row + j)), 0);
                            imgErode->set(Point(col, (row + j)), 0);
                        }
                        
//                        Trial method to get rid of problem where imgErode stablePaths are not deleted. 
//                        if (verRun[(row * ncols) + col] < (ALLOWED_THICKNESS_OF_STAFFLINE_DELETION * staffLineHeight))
//                        {
//                            imageCopy->set(getPointView(((row + j) * ncols) + col, ncols, nrows), 0);
//                            imgErode->set(getPointView(((row + j) * ncols) + col, ncols, nrows), 0);
//                        }
                        
//                        imageCopy->set(getPointView(((row + j) * ncols) + col, ncols, nrows), 0);
//                        imgErode->set(getPointView(((row + j) * ncols) + col, ncols, nrows), 0);

                        if ( ((row + j) > nrows - 1) || ((row + j) < 0 ) )
                        {
                            continue;
                        }
                        
                        if (col == ncols - 1)
                        {
                            continue;
                        }
                        
                        if (row + j - 1 > 0) //Change weight of row right above the first row being deleted
                        {
                            graphWeight[((row + j - 1) * ncols) + col].weight_down = 12;
                        }
                        
                        if (row + j + 1 < nrows - 1) //Change weight of row right below the first row being deleted
                        {
                            graphWeight[((row + j + 1) * ncols) + col].weight_up = 12;
                        }
                        
                        if (row + j > 0)
                        {
                            graphWeight[((row + j) * ncols) + col].weight_up = 12;
                        }
                        else
                        {
                            graphWeight[((row + j) * ncols) + col].weight_up = TOP_VALUE;
                        }
                        
                        graphWeight[((row + j) * ncols) + col].weight_hor = 8;
                        
                        if (row + j < nrows - 1)
                        {
                            graphWeight[((row + j) * ncols) + col].weight_down = 12;
                        }
                        else
                        {
                            graphWeight[((row + j) * ncols) + col].weight_down = TOP_VALUE;
                        }
                    }
                }
            }
            
            npaths.push_back(curr_n_paths);
            
            if (curr_n_paths == 0)
            {
                //return imageCopy;
                break;
            }
        }
        
        postProcessing(validStaves, imageErodedCopy);
        printf ("TOTAL = %lu TOTAL STAFF LINES\n", validStaves.size());
        delete imgErode;
        delete imageErodedCopy;
        
        return imageCopy;
    }
    
    int findAllStablePaths(OneBitImageView *image, int startCol, int endCol, vector <vector<Point> > &stablePaths)
    {
        stablePaths.clear();
        int width = image->ncols();
        int height = image->nrows();
        vector<int> startRow_i;
        
        int endCol_i = width - 1 - startCol;
        int startCol_i = width - 1 - endCol;
        
        for (int row = 0; row < height; row++)
        {
            graphPath[(row * width) + startCol_i].weight = static_cast<weight_t>(0);
            graphPath[(row * width) + startCol_i].start = Point(startCol_i, row);
        }
        
        for (int col = startCol_i + 1; col <= endCol_i; col++) //Creates path from right to left
        {
            for (int row = 0; row < height; row++)
            {
                weight_t weight10, weight20, weight30;
                weight_t value1, value2, value3;
                weight10 = weight20 = weight30 = TOP_VALUE;
                value1 = value2 = value3 = TOP_VALUE;
                
                if (row > 0)
                {
                    weight10 = graphWeight[(row * width) + width - 1 - col].weight_up;
                    value1 = weight10 + graphPath[((row - 1) * width) + (col - 1)].weight;
                }
                
                weight20 = graphWeight[(row * width) + width - 1 - col].weight_hor;
                value2 = weight20 + graphPath[(row * width) + (col - 1)].weight;
                
                if (row < (height - 1))
                {
                    weight30 = graphWeight[(row * width) + width - 1 - col].weight_down;
                    value3 = weight30 + graphPath[(row + 1) * width + (col - 1)].weight;
                }
                
                if ((value3 <= value2) && (value3 <= value1))
                {
                    graphPath[(row * width) + (col)].previous = Point(col - 1, row + 1);
                    graphPath[(row * width) + (col)].weight = value3;
                    graphPath[(row * width) + (col)].start = graphPath[(row + 1) * width + (col - 1)].start;
                }
                else if ((value2 <= value1) && (value2 <= value3))
                {
                    graphPath[(row * width) + (col)].previous = Point(col - 1, row);
                    graphPath[(row * width) + (col)].weight = value2;
                    graphPath[(row * width) + (col)].start = graphPath[((row + 0) * width) + (col - 1)].start;
                }
                else
                {
                    graphPath[(row * width) + (col)].previous = Point(col-1, row-1);
                    graphPath[(row * width) + (col)].weight = value1;
                    graphPath[(row * width) + (col)].start = graphPath[(row - 1) * width + (col - 1)].start;
                }
            }
        }
        
        for (int i = 0; i < height; i++)
        {
            startRow_i.push_back(graphPath[(i * width) + endCol_i].start.y());
        }
        
        for (int row = 0; row < height; row++)
        {
            graphPath[row*width + startCol].weight = static_cast<weight_t>(0.0);
            graphPath[row*width + startCol].start = Point(startCol, row);
        }
        
        for (int col = startCol+1; col <= endCol; col++)
        {
            for (int row = 0; row < height; row++)
            {
                weight_t weight10, weight20, weight30;
                weight_t value1, value2, value3;
                weight10 = weight20 = weight30 = TOP_VALUE;
                value1 = value2 = value3  = TOP_VALUE;
                
                if (row > 0)
                {
                    weight10 = graphWeight[(row-1)*width + col-1].weight_down;
                    value1 = weight10 + graphPath[(row-1)*width + (col-1)].weight;
                }
                
                weight20 = graphWeight[row*width + col-1].weight_hor;
                value2 = weight20 + graphPath[(row+0)*width + (col-1)].weight;
                
                if (row < height-1)
                {
                    weight30 = graphWeight[(row+1)*width + col-1].weight_up;
                    value3 = weight30 + graphPath[(row+1)*width + (col-1)].weight;
                }
                
                if ((value3)<= (value2) && (value3)<= (value1))
                {
                    graphPath[(row)*width + (col)].previous = Point(col-1, row+1);
                    graphPath[(row)*width + (col)].weight = value3;
                    graphPath[(row)*width + (col)].start = graphPath[(row+1)*width + (col-1)].start;
                }
                else if ((value2)<= (value1) && (value2)<= (value3))
                {
                    graphPath[(row)*width + (col)].previous = Point(col-1, row);
                    graphPath[(row)*width + (col)].weight = value2;
                    graphPath[(row)*width + (col)].start = graphPath[(row+0)*width + (col-1)].start;
                }
                else
                {
                    graphPath[(row)*width + (col)].previous = Point(col-1, row-1);
                    graphPath[(row)*width + (col)].weight = value1;
                    graphPath[(row)*width + (col)].start = graphPath[(row-1)*width + (col-1)].start;
                }
            }
        }
        
        for (int i = 0; i < height; i++)
        {
            int startR = graphPath[i*width + endCol].start.y();
            
            if (startRow_i[startR] == i)
            {
                Point p = Point(endCol, i);
                vector<Point> contour;
                contour.resize(endCol - startCol + 1);
                int pos = endCol - startCol;
                contour[pos] = p;
                pos--;
                
                while (p.x() != startCol)
                {
                    p = graphPath[(p.y() * width) + p.x()].previous;
                    contour[pos] = p;
                    pos--;
                }
                
                stablePaths.push_back(contour);
            }
        }
        return 0;
    }
    
    vector <vector <vector<Point> > > postProcessing(vector <vector<Point> > &validStaves, OneBitImageView *imageErodedCopy)
    {
        vector <vector <vector<Point> > > setsOfValidStaves;
        cout <<"Postprocessing Image\n";
        
        if (!validStaves.size())
        {
            return setsOfValidStaves;
        }
        
        //Uncross Lines
        for (int c = validStaves[0][0].x(); c <= validStaves[0][validStaves[0].size() - 1].x(); c++)
        {
            vector<Point> column;
            
            for (int nvalid = 0; nvalid < validStaves.size(); nvalid++)
            {
                column.push_back(validStaves[nvalid][c]);
            }
            
            sort(column.begin(), column.end());
            
            for (int nvalid = 0; nvalid < validStaves.size(); nvalid++)
            {
                validStaves[nvalid][c] = column[nvalid];
            }
        }
        
        vector<Point> medianStaff;
        computeMedianStaff(validStaves, medianStaff);
        
        //staffDistance similar to staffLineHeightValue
        int maxStaffDistance;
        
        if (abs(staffSpaceDistance - staffLineHeight) < (0.5 * max(staffSpaceDistance, staffLineHeight)))
        {
            maxStaffDistance = max(3 * staffSpaceDistance, 3 * staffLineHeight);
        }
        else
        {
            maxStaffDistance = max(2 * staffSpaceDistance, 2 * staffLineHeight);
        }
        
        printf("maxStaffDistance = %d\n", maxStaffDistance);
        
        //Organize in sets
        int start = 0;
        
        for (size_t nvalid = 0; nvalid < validStaves.size(); nvalid++)
        {
            if ((nvalid == validStaves.size() - 1) || (simplifiedAvgDistance(validStaves[nvalid], validStaves[nvalid + 1]) > maxStaffDistance))
            {
                vector <vector<Point> > singleSet;
                
                for (int i = start; i <= nvalid; i++)
                {
                    singleSet.push_back(validStaves[i]);
                }
                
//                if (singleSet.size() >= 2) //Paper writers wanted more complex rules to validate sets
//                {
//                    setsOfValidStaves.push_back(singleSet);
//                    printf("SET SIZE = %lu\n", singleSet.size());
//                }
                
                //I want to include systems with only one staff. There will have to be rules written to make sure it actually should be included
                setsOfValidStaves.push_back(singleSet);
                printf("SET SIZE = %lu\n", singleSet.size());
                
                start = nvalid + 1;
            }
        }
        
        int ncolsEroded = imageErodedCopy->ncols();
        int nrowsEroded = imageErodedCopy->nrows();
        
        //Undocumented Operation
        for (int i = 0; i < setsOfValidStaves.size(); i++)
        {
            vector <vector<Point> > &setOfStaves = setsOfValidStaves[i];
            
            for (int nvalid = 0; nvalid < setOfStaves.size(); nvalid++)
            {
                for (int deltacolumn = 2, sgn = 1; deltacolumn < ncolsEroded; deltacolumn++, sgn = (-1) * sgn)
                {
                    int c = (ncolsEroded / 2) + ((deltacolumn >> 1) * sgn);
                    int y = setOfStaves[nvalid][c].y();
                    int x = setOfStaves[nvalid][c].x();
                    int my = medianStaff[c].y();
                    int y0 = setOfStaves[nvalid][ncolsEroded / 2].y();
                    int my0 = medianStaff[ncolsEroded / 2].y();
                    double alpha = 0;
                    unsigned char pel =  imageErodedCopy->get(Point(x, y));
                    
                    if (!pel)
                    {
                        alpha = pow((abs(c - (ncolsEroded / 2)) / (double(ncolsEroded / 2))), 1 / 4.0);
                    }
                    
                    int delta = static_cast<int>( (1 - alpha) * (y - y0) + (alpha * (my - my0)) );
                    y = y0 + delta;
                    int prev_y = setOfStaves[nvalid][c - sgn].y();
                    
                    if ((y - prev_y) > 1)
                    {
                        y = prev_y + 1;
                    }
                    
                    if ((y - prev_y) < -1)
                    {
                        y = prev_y - 1;
                    }
                    
                    setOfStaves[nvalid][c] = Point(setOfStaves[nvalid][c].x(), min(max(y, 0), nrowsEroded - 1));
                }
            }
        }
        
        //Trim and smooth
        vector <vector <vector<Point> > >::iterator set_it = setsOfValidStaves.begin();
        
        while (set_it != setsOfValidStaves.end())
        {
            vector <vector<Point> > &setOfStaves = *set_it; //setsOfValidStaves[i]
            
            //Compute the median staff in terms of color
//            vector<unsigned char> medianStaff;
//            
//            for (int c = 0; c < ncolsEroded; c++)
//            {
//                vector <unsigned char> medianValue;
//                
//                for (int i = 0; i < setOfStaves.size(); i++)
//                {
//                    int x = setOfStaves[i][c].x();
//                    int y = setOfStaves[i][c].y();
//                    unsigned char pel = imageErodedCopy->get(getPointView((y * ncolsEroded) + x, ncolsEroded, nrowsEroded));
//                    medianValue.push_back(pel);
//                }
//                
//                sort(medianValue.begin(), medianValue.end());
//                medianStaff.push_back(medianValue[medianValue.size() - 1]);
//            }
            //1 find start and end
            int startx = 0, endx = ncolsEroded - 1;
            
            //trimIndividualPaths(setOfStaves, (2 * staffSpaceDistance));
            //trimPath(medianStaff, (2 * staffSpaceDistance), startx, endx);
            //blackPercentageTrimEdges(setOfStaves, startx, endx);
            
            if ( (endx - startx) < maxStaffDistance) //remove whole set
            {
                set_it = setsOfValidStaves.erase(set_it);
                continue;
            }
            
            //2 trim staffs from start to nvalid
            for (int i = 0; i < setOfStaves.size(); i++)
            {
//                smoothStaffLine(setOfStaves[i], SMOOTH_STAFF_LINE_WINDOW * staffSpaceDistance);
                
                vector<Point>::iterator it = setOfStaves[i].begin();
                
                while (it->x() != startx)
                {
                    it++;
                }
                
                setOfStaves[i].erase(setOfStaves[i].begin(), it);
                
                it = setOfStaves[i].begin();
                
                while (it->x() != endx)
                {
                    it++;
                }
                
                setOfStaves[i].erase(it, setOfStaves[i].end() );
            }
            set_it++;
        }
        
        //validStaves.clear();
        
        for (int i = 0; i < setsOfValidStaves.size(); i++)
        {
            vector <vector<Point> > &setOfStaves = setsOfValidStaves[i];
            
            for (int s =0; s < setOfStaves.size(); s++)
            {
                validStaves.push_back(setOfStaves[s]);
            }
        }
        
        //return finalTrim(setsOfValidStaves, imageErodedCopy);
        return setsOfValidStaves;
    }
    
    void trimPath(vector<unsigned char> &vec, int window, int &startX, int &endX)
    {
        startX = 0;
        
        while((!vec[startX]) && (startX < (vec.size() / 2)))
        {
            startX++;
        }
        
        int i;
        int sum = 0;
        
        for (i = startX; i < (startX + window); i++)
        {
            sum += vec[i];
        }
        
        for (; i < (vec.size() / 2); i++)
        {
            sum += vec[i];
            sum -= vec[i - window];
            if (sum < (window * 1.0 * 0.9)) //Original code accounted for greyscale, may be problematic
            {
                startX = i + 1;
            }
        }
        
        endX = vec.size() - 1;
        
        while ((!vec[endX]) && (endX > (vec.size() / 2)))
        {
            endX--;
        }
        
        sum = 0;
        
        for (i = endX; i > endX - window; i--)
        {
            sum += vec[i];
        }
        
        for (; i > (vec.size() / 2); i--)
        {
            sum += vec[i];
            sum -= vec[i + window];
            
            if (sum < (window * 1 * 0.9)) //Original code accounted for greyscale
            {
                endX = i - 1;
            }
        }
    }
    
    void blackPercentageTrimEdges(vector< vector<Point> > &staffSystem, int &startX, int &endX)
    {
        //startX = 0;
        for (int col = startX; col < (imageWidth / 3); col++) //Trim up to a third of the staff system
        {
            if (verticalBlackPercentage(col, staffSystem[0][col].y(), staffSystem[staffSystem.size() - 1][col].y()) > ALLOWED_VERTICAL_BLACK_PERCENTAGE)
            {
                startX++;
            }
        }
        
        //endX = imageWidth - 1;
        for (int col = endX; col > (imageWidth - (imageWidth * (1.0 / 3.0))); col--) //Trim up to a third of the staff system
        {
            if (verticalBlackPercentage(col, staffSystem[0][col].y(), staffSystem[staffSystem.size() - 1][col].y()) > ALLOWED_VERTICAL_BLACK_PERCENTAGE)
            {
                endX--;
            }
        }
    }
    
    void smoothStaffLine(vector<Point> &staff, int halfWindowSize)
    {
        if (staff.size() < ((halfWindowSize * 2) + 1))
        {
            return;
        }
        
        vector<Point> cpStaff = staff;
        int accumY = 0;
        
        for (int i = 0; i < (halfWindowSize * 2); i++)
        {
            accumY += cpStaff[i].y();
            staff[i / 2] = Point(staff[i / 2].x(), (accumY / (i + 1)));
        }
        
        for (int i = halfWindowSize; i < (staff.size() - halfWindowSize); i++)
        {
            accumY += cpStaff[i + halfWindowSize].y();
            staff[i] = Point(staff[i].x(), (accumY / ((halfWindowSize * 2) + 1)));
            accumY -= cpStaff[i - halfWindowSize].y();
        }
        
        accumY = 0;
        
        for (int i = 0; i < (halfWindowSize * 2); i++)
        {
            accumY += cpStaff[staff.size() - 1 - i].y();
            staff[staff.size() - 1 - (i / 2)] = Point(staff[staff.size() - 1 - (i / 2)].x(), (accumY / (i + 1)));
        }
    }
    
    template<class T>
    vector <vector <vector<Point> > > returnSetsOfStablePaths(vector <vector <Point> > &validStaves, T &image)
    {
        OneBitImageView *imageCopy = myCloneImage(image);
        OneBitImageView *imgErode = myCloneImage(image);
        OneBitImageView *imageErodedCopy = myCloneImage(image);
        myVerticalErodeImage(imgErode, image.ncols(), image.nrows());
        myVerticalErodeImage(imageErodedCopy, image.ncols(), image.nrows());
        
        vector<int> npaths;
        
        bool first_time = 1;
        double blackPerc;
        vector<Point> bestStaff;
        
        int nrows = imageCopy->nrows();
        int ncols = imageCopy->ncols();
        
        while(1)
        {
            vector <vector<Point> > stablePaths;
            int curr_n_paths = 0;
            printf("About to findAllStablePaths\n");
            findAllStablePaths(imageCopy, 0, ncols - 1, stablePaths);
            printf("Finished findAllStablePaths. Size = %lu\n", stablePaths.size());
            
            if (first_time && (stablePaths.size() > 0))
            {
                first_time = 0;
                bestStaff.clear();
                size_t bestSumOfValues = INT_MAX;
                size_t bestStaffIdx = 0;
                vector<size_t> allSumOfValues;
                
                for (size_t c = 0; c < stablePaths.size(); c++)
                {
                    size_t sumOfValues = sumOfValuesInVector(stablePaths[c], imgErode);
                    
                    if ((sumOfValues / (1.0 * (stablePaths[c].size()))) > MIN_BLACK_PER) //Checks to make sure percentage of black values are larger than the minimum black percentage allowed
                    {
                        allSumOfValues.push_back(sumOfValues);
                    }
                    
                    if (sumOfValues < bestSumOfValues)
                    {
                        bestSumOfValues = sumOfValues;
                        bestStaffIdx = c;
                    }
                }
                
                if (!allSumOfValues.size())
                {
                    cout <<"There are no valid stable paths in your image. Sorry.\n";
                    return postProcessing(validStaves, imageErodedCopy);
                }
                
                vector<size_t> copy_allSumOfValues = allSumOfValues; //Still not sure why this is necessary
                sort(allSumOfValues.begin(), allSumOfValues.end());
                //Must deal with empty allSumOfValues in case of completely blank image/image with no initial stablePaths
                size_t medianSumOfValues = allSumOfValues[allSumOfValues.size()/2];
                int i;
                
                for (i = 0; i < allSumOfValues.size(); i++)
                {
                    printf("copy_allSumOfValues[%d] = %lu\n", i, copy_allSumOfValues[i]);
                    
                    if (copy_allSumOfValues[i] == medianSumOfValues)
                    {
                        break;
                    }
                }
                
                bestStaff = stablePaths[i];
                
                double bestBlackPerc = medianSumOfValues/(1.0 * bestStaff.size());
                blackPerc = max(MIN_BLACK_PER, bestBlackPerc * 0.8);
                cout <<"bestBlackPerc: " <<bestBlackPerc <<" blackPerc: " <<blackPerc <<endl;
            }
            
            for (size_t i = 0; i < stablePaths.size(); i++)
            {
                vector<Point> staff = stablePaths[i];
                
                if (tooMuchWhite(staff, imgErode, blackPerc))
                {
                    printf("Too much white\n");
                    continue;
                }
                
                double dissimilarity = staffDissimilarity(bestStaff, staff);
                printf ("\tDissimilarity = %f, staffSpaceDistance = %d\n", dissimilarity, staffSpaceDistance);
                
                if (dissimilarity > (ALLOWED_DISSIMILARITY * staffSpaceDistance))
                {
                    printf ("\tToo Dissimilar. Dissimilarity = %f, staffSpaceDistance = %d\n", dissimilarity, staffSpaceDistance);
                    continue;
                }
                
                validStaves.push_back(staff);
                curr_n_paths++;
                
                int path_half_width2 = max(staffLineHeight, staffSpaceDistance/2);
                
                //Erasing paths from our image, must create a copy of our image
                for (size_t i = 0; i < staff.size(); i++)
                {
                    //printf("Staff Size: %lu\n", staff.size());
                    int col = staff[i].x();
                    int row = staff[i].y();
                    
                    //ERASE PATHS ALREADY SELECTED!
                    for (int j = -path_half_width2; j <= path_half_width2; j++)
                    {
                        // printf("path_half_width2 = %d, j = %d\n", path_half_width2, j);
                        // printf("Currently erasing lines\n");
                        if ( ((row + j) > (nrows - 1)) || ((row + j) < 0) )
                        {
                            continue;
                        }
                        
                        //                        If a vertical run of pixels that is less than some value times the staffLineHeight is along the path, set it to white
                        //                        if (verRun[((row + j) * ncols) + col] < (ALLOWED_THICKNESS_OF_STAFFLINE_DELETION * staffLineHeight))
                        //                        {
                        //                            imageCopy->set(getPointView(((row + j) * ncols) + col, ncols, nrows), 0);
                        //                            imgErode->set(getPointView(((row + j) * ncols) + col, ncols, nrows), 0);
                        //                        }
                        
                        //                        Trial method to get rid of problem where imgErode stablePaths are not deleted.
                        //                        if (verRun[(row * ncols) + col] < (ALLOWED_THICKNESS_OF_STAFFLINE_DELETION * staffLineHeight))
                        //                        {
                        //                            imageCopy->set(getPointView(((row + j) * ncols) + col, ncols, nrows), 0);
                        //                            imgErode->set(getPointView(((row + j) * ncols) + col, ncols, nrows), 0);
                        //                        }
                        
                        imageCopy->set(getPointView(((row + j) * ncols) + col, ncols, nrows), 0);
                        imgErode->set(getPointView(((row + j) * ncols) + col, ncols, nrows), 0);
                        
                        if ( ((row + j) > nrows - 1) || ((row + j) < 0 ) )
                        {
                            continue;
                        }
                        
                        if (col == ncols - 1)
                        {
                            continue;
                        }
                        
                        if (row + j - 1 > 0) //Change weight of row right above the first row being deleted
                        {
                            graphWeight[((row + j - 1) * ncols) + col].weight_down = 12;
                        }
                        
                        if (row + j + 1 < nrows - 1) //Change weight of row right below the first row being deleted
                        {
                            graphWeight[((row + j + 1) * ncols) + col].weight_up = 12;
                        }
                        
                        if (row + j > 0)
                        {
                            graphWeight[((row + j) * ncols) + col].weight_up = 12;
                        }
                        else
                        {
                            graphWeight[((row + j) * ncols) + col].weight_up = TOP_VALUE;
                        }
                        
                        graphWeight[((row + j) * ncols) + col].weight_hor = 8;
                        
                        if (row + j < nrows - 1)
                        {
                            graphWeight[((row + j) * ncols) + col].weight_down = 12;
                        }
                        else
                        {
                            graphWeight[((row + j) * ncols) + col].weight_down = TOP_VALUE;
                        }
                    }
                }
            }
            
            npaths.push_back(curr_n_paths);
            
            if (curr_n_paths == 0)
            {
                //return imageCopy;
                break;
            }
        }
        
        return postProcessing(validStaves, imageErodedCopy);
    }
    
    //============================================================================
    //                          finalTrim
    //============================================================================
    
    vector <vector <vector <Point> > > finalTrim(vector <vector <vector <Point> > > &staffSets, OneBitImageView *image)
    {
        vector <vector <vector <Point> > > trimmedSets; //Will store the final, trimmed staff systems
        vector <vector <BVAL> > breakValues(staffSets.size()); //Will store the x values at which systems will be broken up. Assumed to store in sets of two with first number being the beginning of a desired staff system and the second number being the end of a desired staff system.
        int numSets = staffSets.size();
        
        for (int set = 0; set < numSets; set++)
        {
            getBreakPoints(staffSets[set], breakValues[set], image);
        }
        
        for (int set = 0; set < numSets; set++)
        {
            trimIndividualSet(staffSets[set], breakValues[set], trimmedSets);
        }
        
        return trimmedSets;
    }
    
    void trimIndividualSet(vector <vector <Point> > &staffSet, vector <BVAL> &breakValues, vector <vector <vector <Point> > > &trimmedSets)
    {
        vector <vector<Point> > trimmedSet;
        int endVal = breakValues.size() - 1;
        for (int breakValIndex = 0; breakValIndex < endVal; breakValIndex = (breakValIndex + 2))
        {
            if (!(breakValIndex) && (!(breakValues[breakValIndex].pixVal)))
            {
                breakValIndex++;
            }
            
            trimmedSet.clear();
            
            for (int staff = 0; staff < staffSet.size(); staff++)
            {
                trimmedSet.push_back(trimIndividualStaff(staffSet[staff], breakValues[breakValIndex].breakVal, breakValues[breakValIndex + 1].breakVal));
            }
            
            trimmedSets.push_back(trimmedSet);
        }
    }
    
    vector<Point> trimIndividualStaff(vector <Point> &staffLine, int startingX, int endingX)
    {
        vector<Point>::iterator staffIt1 = staffLine.begin();
        vector<Point>::iterator staffIt2 = staffLine.begin();
        vector<Point> trimmedStaff;
        
        while (staffIt1->x() != startingX)
        {
            staffIt1++;
        }
        
        while (staffIt2->x() != endingX)
        {
            staffIt2++;
        }
        
        trimmedStaff.insert(trimmedStaff.begin(), staffIt1, staffIt2);
        
        return trimmedStaff;
    }
    
    void getBreakPoints(vector <vector <Point> > &staffSet, vector <BVAL> &breakValues, OneBitImageView *image) //Will be used to check hits of black pixels by a set of stable paths
    {
        int startingX = staffSet[0][0].x();
        int endingX = staffSet[0][staffSet[0].size() - 1].x();
        int setSize = staffSet.size();
        int mishitCounter = 0;
        int hitCounter = 0;
        BVAL breakValue;
        
        for (int xVal = startingX; xVal <= endingX; xVal++)
        {
            double hitPercent = checkHitPercentage(staffSet, xVal, image);
            int breakValuesSize = breakValues.size();
            
            if (!breakValuesSize)
            {
                breakValue.breakVal = 0;
                breakValue.pixVal = (int)hitPercent; //Will be stored as a 1 or 0
                breakValues.push_back(breakValue);
                continue;
            }
            
            if ((breakValues[breakValuesSize - 1].pixVal) && (hitPercent <= ALLOWED_VERTICAL_HIT_PERCENTAGE))
            {
                mishitCounter++;
                
                if (hitCounter < mishitCounter)
                {
                    hitCounter = 0;
                }
            }
            else if (!(breakValues[breakValuesSize - 1].pixVal) && (hitPercent > ALLOWED_VERTICAL_HIT_PERCENTAGE))
            {
                hitCounter++;
                mishitCounter = 0;
                
//                if (hitCounter > mishitCounter)
//                {
//                    mishitCounter = 0;
//                }
            }
            
            if ((!(breakValuesSize) && (mishitCounter > staffSpaceDistance)) || ((breakValues[breakValuesSize - 1].pixVal) && (mishitCounter > staffSpaceDistance)))
            {
                breakValue.breakVal = xVal - (mishitCounter - 1);
                breakValue.pixVal = 0;
                breakValues.push_back(breakValue);
                hitCounter = 0;
            }
            else if ((!(breakValuesSize) && (hitCounter > staffSpaceDistance)) || (!(breakValues[breakValuesSize - 1].pixVal) && (hitCounter > staffSpaceDistance)))
            {
                breakValue.breakVal = xVal - (hitCounter - 1);
                breakValue.pixVal = 1;
                breakValues.push_back(breakValue);
                mishitCounter = 0;
            }
        }
        
        //Stores a final breakValue for the right edge of the page
        if (breakValues[breakValues.size() - 1].pixVal)
        {
            breakValue.breakVal = (imageWidth - 2); //Minus 2 because setsOfValidStaves does not reach right edge (To be visited)
            breakValue.pixVal = 1;
            breakValues.push_back(breakValue);
        }
        else
        {
            breakValue.breakVal = (imageWidth - 2); //Minus 2 because setsOfValidStaves does not reach right edge (To be visited)
            breakValue.pixVal = 0;
            breakValues.push_back(breakValue);
        }
    }
    
    double checkHitPercentage(vector <vector <Point> > &staffSet, int xVal, OneBitImageView *image) //returns percentage of black pixels hit by staves sharing an x value
    {
        double numOfStaves = staffSet.size();
        double numHits = 0.0; //Stores number of black pixels hit by a staves sharing an x value in one staff set
        
        for (int staff = 0; staff < numOfStaves; staff++)
        {
            if (nearHit(image, staffSet[staff][xVal]))
            {
                numHits++;
            }
        }
        
        return (numHits / numOfStaves);
    }
    
    bool nearHit(OneBitImageView *image, Point pixel)
    {
        int startingY;
        
        if (pixel.y() - staffLineHeight > 0)
        {
            startingY = pixel.y() - staffLineHeight - ALLOWED_OFFSET_NEARHIT;
        }
        else
        {
            startingY = 0;
        }
        
        for (int yVal = startingY; yVal < startingY + (2 * (staffLineHeight + ALLOWED_OFFSET_NEARHIT)) && yVal < imageHeight; yVal++)
        {
            if (image->get(Point(pixel.x(), yVal)))
            {
                return true;
            }
        }
        
        return false;
    }

    //=============================================================================
    //                          HELPER FUNCTIONS
    //=============================================================================
    double slope(double x1, double x2, double y1, double y2)
    {
        return ((y1 - y2) / (x2 - x1));
    }
    
    vector <double> findSlopes(vector <Point> &staff)
    {
        int size = staff.size();
        vector <double> slopes;
        
        for (int point = 0; point < size - staffSpaceDistance; point++)
        {
            //Slopes normally would be (y1 - y2) / (x2 - x1) but the top left corner starts at 0,0 instead of the bottom left corner
            if ((point - staffSpaceDistance) <= 0)
            {
                double y2 = static_cast<double>(staff[point + staffSpaceDistance].y());
                double y1 = static_cast<double>(staff[0].y());
                double x2 = static_cast<double>(staff[point + staffSpaceDistance].x());
                double x1 = static_cast<double>(staff[0].x());
                slopes.push_back(slope(x1, x2, y1, y2));
            }
            else
            {
                double y2 = static_cast<double>(staff[point + staffSpaceDistance].y());
                double y1 = static_cast<double>(staff[point - staffSpaceDistance].y());
                double x2 = static_cast<double>(staff[point + staffSpaceDistance].x());
                double x1 = static_cast<double>(staff[point - staffSpaceDistance].x());
                slopes.push_back(slope(x1, x2, y1, y2));
            }
        }
        
        return slopes;
    }
    
    void fixStaffSystem(vector <vector <Point> > &staffSystem)
    {
        int size = staffSystem.size();
        vector <vector <double> > staffSlopes;
        vector <vector <SLOPEBVAL> > breakVals(size);
        
        for (int staff = 0; staff < size; staff++)
        {
            staffSlopes.push_back(findSlopes(staffSystem[staff]));
        }
        
        vector <vector <double> > staffSlopesCopy;
        staffSlopesCopy = staffSlopes;
        vector <double> mostCommonSlopes;
        
        for (int staff = 0; staff < size; staff++)
        {
            sort(staffSlopesCopy[staff].begin(), staffSlopesCopy[staff].end());
            mostCommonSlopes.push_back(findMostRepresentedValueOnSortedVector(staffSlopesCopy[staff]));
        }
        
        double mostCommonSlope = findMostRepresentedValueOnSortedVector(mostCommonSlopes);
        cout <<"The most common slope is " <<mostCommonSlope <<endl;
        
        findDissimilarSlopeBreakPoints(staffSlopes, breakVals, mostCommonSlope);
        
        changePointValuesForSystem(staffSystem, breakVals, mostCommonSlope);
    }
    
    void findDissimilarSlopeBreakPoints(vector <vector <double> > &staffSlopes, vector <vector <SLOPEBVAL> > &breakVals, double mostCommonSlope)
    {
        int size = staffSlopes.size();
        
        for (int staff = 0; staff < size; staff++)
        {
            findDissimilarSlopeBreakPointsSingleStaff(staffSlopes[staff], breakVals[staff], mostCommonSlope);
        }
    }
    
    void findDissimilarSlopeBreakPointsSingleStaff(vector <double> &staff, vector <SLOPEBVAL> &breakVals, double mostCommonSlope)
    {
        SLOPEBVAL breakValue;
        int size = staff.size();
        int mishitCounter = 0;
        int hitCounter = 0;
        
        for (int point = 0; point < size; point++)
        {
            int breakValsSize = breakVals.size();
            
            if (withinTolerance(staff[point], mostCommonSlope))
            {
                hitCounter++;
            }
            else
            {
                mishitCounter++;
            }
            
            if ((mishitCounter > staffSpaceDistance) && (!breakValsSize))
            {
                breakValue.breakVal = point - mishitCounter;
                breakValue.start = true;
                breakVals.push_back(breakValue);
                hitCounter = 0;
            }
            else if ((mishitCounter > staffSpaceDistance) && !(breakVals.size() % 2))
            {
                breakValue.breakVal = point - mishitCounter;
                breakValue.start = true;
                breakVals.push_back(breakValue);
                hitCounter = 0;
            }
            else if ((hitCounter > staffSpaceDistance) && (breakVals.size() % 2))
            {
                breakValue.breakVal = point - hitCounter;
                breakValue.start = false;
                breakVals.push_back(breakValue);
                mishitCounter = 0;
            }
            
        }
    }
    
    bool withinTolerance(double slope, double mostCommonSlope)
    {
        if ((slope <= (SLOPE_TOLERANCE * mostCommonSlope)) && (slope >= (mostCommonSlope / SLOPE_TOLERANCE)))
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    
    void changePointValuesForSystem(vector <vector <Point> > &staffSystem, vector <vector <SLOPEBVAL> > &breakVals, double mostCommonSlope)
    {
        int size = staffSystem.size();
        
        for (int staff = 0; staff < size; staff++)
        {
            changePointValuesForStaff(staffSystem[staff], breakVals[staff], mostCommonSlope);
        }
    }
    
    void changePointValuesForStaff(vector <Point> &staff, vector <SLOPEBVAL> &breakVals, double mostCommonSlope)
    {
        int staffSize = staff.size();
        int breakValsSize = breakVals.size();
        
        for (int bVal = 0; bVal < breakValsSize; bVal++)
        {
            if (breakVals[bVal].start && (bVal != (breakValsSize - 1)))
            {
                fixLineSegment(staff, breakVals[bVal].breakVal, breakVals[bVal + 1].breakVal, mostCommonSlope);
            }
        }
    }
    
    void fixLineSegment(vector <Point> &staff, int start, int end, double mostCommonSlope)
    {
        int deltaX = abs(staff[end].x() - staff[start].x());
        int deltaY = abs(staff[start].y() - staff[end].y());
//        int horizontalStep = (deltaX / deltaY); //number of pixels in a "step" as line goes up or down
//        int horizontalStepCounter = 0;
        
        if (withinTolerance(slope(static_cast<double>(staff[start].x()), static_cast<double>(staff[end].x()), static_cast<double>(staff[start].y()), static_cast<double>(staff[end].y())), mostCommonSlope))
        {
            cout<<"SUCCESS!! Required slope: " <<mostCommonSlope <<" Actual slope: " <<slope((static_cast<double>(staff[start].x())), (static_cast<double>(staff[end].x())), (static_cast<double>(staff[start].y())), (static_cast<double>(staff[end].y()))) <<endl;
            
            for (int point = 0; point <= deltaX; point++)
            {
                if (deltaY)
                {
                    staff[start + point] = Point(staff[start + point].x(), staff[start].y() - (point / deltaY));
                }
                else
                {
                    staff[start + point] = Point(staff[start + point].x(), staff[start].y());
                }
            }
        }
        else
        {
            cout<<"FAILURE!! Required slope: " <<mostCommonSlope <<" Actual slope: " <<slope((static_cast<double>(staff[start].x())), (static_cast<double>(staff[end].x())), (static_cast<double>(staff[start].y())), (static_cast<double>(staff[end].y()))) <<endl;
        }
    }
    
    template <class T>
    T findMostRepresentedValueOnSortedVector(vector<T> &vec)
    {
        T run = vec[0];
        int freq = 0;
        int maxFreq = 0;
        T maxRun = run;

        for (unsigned i = 0; i < vec.size(); i++)
        {
            if (vec[i] == run)
            {
                freq++;
            }
            
            if (freq > maxFreq)
            {
                maxFreq = freq;
                maxRun = run;
            }
            
            if (vec[i] != run)
            {
                freq = 0;
                run = vec[i];
            }
        }
        
        return maxRun;
    }
    
    double staffDissimilarity(vector<Point> &staff1, vector<Point> &staff2)
    {
        unsigned long currAvg1 = 0;
        unsigned long currAvg2 = 0;
        
        for (size_t i = 0; i < staff1.size(); i++)
        {
            currAvg1 += staff1[i].y();
            currAvg2 += staff2[i].y();
        }
        
        currAvg1 /= staff1.size();
        currAvg2 /= staff2.size();
        double avgDiff = 0;
        
        for (size_t i = 0; i < staff1.size(); i++)
        {
            unsigned long curr_dif = abs(staff1[i].y() - staff2[i].y() - currAvg1 + currAvg2);
            avgDiff += (curr_dif * curr_dif);
        }
        
        avgDiff /= staff1.size();
        avgDiff = sqrt(avgDiff);
        
        return avgDiff;
    }
    
    int sumOfValuesInVector (vector<Point>& vec, OneBitImageView *image)
    {
        //size_t len = vec.size();
        int sumOfValues = 0;
        int startCol = 0;
        int endCol = image->ncols() - 1;
        
        for (int i = startCol; i <= endCol; i++)
        {
            int col = vec[i].x();
            int row = vec[i].y();
            unsigned char pel = image->get(Point(col, row));
            sumOfValues += pel;
        }
        
        return sumOfValues;
    }
    
    bool tooMuchWhite (vector<Point> &vec, OneBitImageView *image, double minBlackPerc)
    {
        int sumOfValues = sumOfValuesInVector(vec, image);
        int startCol = 0;
        int endCol = image->ncols() - 1;
        double usedSize = endCol - startCol + 1.0;
        
        if (sumOfValues < ((1.0 - minBlackPerc) * (usedSize)))
        {
            return true;
        }
        
        return false;
    }
    
    double simplifiedAvgDistance(vector<Point> &staff1, vector<Point> &staff2)
    {
        if ((staff1.size() == 0) || (staff2.size() == 0))
        {
            printf("SIZE 0\n");
            return -1;
        }
        
        int simplifiedDistance = 0;
        int m = max(staff1[0].x(), staff2[0].x());
        int M = min(staff1[staff1.size() - 1].x(), staff2[staff2.size() - 1].x());
        int idx1 = 0, idx2 = 0;
        
        if (m > M)
        {
            printf("Do not overlap\n");
            return -1;
        }
        
        while (staff1[idx1].x() != m)
        {
            idx1++;
        }
        
        while (staff2[idx2].x() != m)
        {
            idx2++;
        }
        
        while (staff1[idx1].x() != M)
        {
            int dy = abs(staff1[idx1].y() - staff2[idx2].y());
            simplifiedDistance += abs(dy);
            idx1++;
            idx2++;
        }
        
        return static_cast<double>((simplifiedDistance) / (M - m + 1));
    }
    
    void computeMedianStaff(vector <vector<Point> > &staves, vector<Point> &medianStaff)
    {
        medianStaff.clear();
        
        for (int c = staves[0][0].x(); c <= staves[0][staves[0].size() - 1].x(); c++)
        {
            vector<int> delta;
            
            for (size_t nvalid = 0; nvalid < staves.size(); nvalid++)
            {
                if (c != staves[0][0].x())
                {
                    delta.push_back(staves[nvalid][c].y() - staves[nvalid][0].y());
                }
                else
                {
                    delta.push_back(staves[nvalid][0].y());
                }
            }
            
            sort(delta.begin(), delta.end());
            int y;
            
            if (c != staves[0][0].x())
            {
                y = medianStaff[0].y() + delta[staves.size()/2];
            }
            else
            {
                y = delta[staves.size()/2];
            }
            
            medianStaff.push_back(Point(c, y));
        }
    }
    
    void deletePaths(vector <vector<Point> > &validStaves, OneBitImageView *image)
    {
        int numPaths = validStaves.size();
        int numPix = validStaves[0].size();
        printf("numPaths: %d, numPix: %d\n", numPaths, numPix);
        
        for (int i = 0; i < numPaths; i++)
        {
            for (int x = 0; x < numPix; x++)
            {
                if (verRun[((validStaves[i][x].y() * numPix) + validStaves[i][x].x())] < 5)
                {
                    image->set(validStaves[i][x], 0);
                }
            }
        }
    }
    
    void deleteOnePath(vector<Point> path, OneBitImageView *image)
    {
        int size = path.size();
        
        for (int i = 0; i < size; i++)
        {
            image->set(path[i], 0);
        }
    }
    
    void drawPaths(vector <vector<Point> > &validStaves, OneBitImageView *image)
    {
        int numPaths = validStaves.size();
        int numPix = validStaves[0].size();
        printf("numPaths: %d, numPix: %d\n", numPaths, numPix);
        
        for (int i = 0; i < numPaths; i++)
        {
            for (int x = 0; x < validStaves[i].size(); x++)
            {
                image->set(validStaves[i][x], 1);
            }
        }
    }
    
    //===================================================================================================
    //================================ Testing Functions ================================================
    //===================================================================================================

    OneBitImageView* lineDraw(OneBitImageView *image)
    {
        int height = image->nrows();
        int width = image->ncols();
        int counter = 0;
        
        for (int y = height/5; y < height - 2; y += (height / 20))
        {
            if (counter > 4)
            {
                y += (height / 10);
                counter = 0;
            }
            for (int x = width/10; x < width - 1; x++)
            {
                image->set(Point(x, y), 1);
                image->set(Point(x, y + 1), 1);
            }
            counter++;
        }
        
        return image;
    }
    
    OneBitImageView* lineDrawSmall(OneBitImageView *image)
    {
        int height = image->nrows();
        int width = image->ncols();
        int counter = 0;
        
        for (int y = 3; y < height - 2; y += 4)
        {
            for (int x = width/10; x < width - 1; x++)
            {
                image->set(Point(x, y), 1);
//                image->set(Point(x, y + 1), 1);
            }
            counter++;
        }
        
        return image;
    }
    
    double verticalBlackPercentage(int col, int startRow, int endRow)
    {
        double totalRows = endRow - startRow;
        double numBlackPixels = 0.0;
        
        for (int row = startRow; row <= endRow; row++)
        {
            numBlackPixels += primaryImage->get(Point(col, row));
        }
        
        return (1.0 - (numBlackPixels / totalRows));
    }

};

//===================================================================================================
//================================ Plugins ==========================================================
//===================================================================================================

template<class T>
float returnGraphWeights(T &image) 
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    
    return slf1.staffLineHeight;
}

template<class T>
OneBitImageView* deleteStablePaths(T &image)
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    OneBitImageView *new1 = slf1.myCloneImage(image);
    printf("Rows: %lu, Columns: %lu\n", image.nrows(), image.ncols());
    printf("findAllStablePaths: %d\n", slf1.findAllStablePaths(slf1.primaryImage, 0, image.ncols()-1, validStaves));
    slf1.deletePaths(validStaves, new1);
    
    return new1;
}

template<class T>
OneBitImageView* removeStaves(T &image, int staffline_height, int staffspace_height)
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    
    if (staffline_height)
    {
        slf1.staffLineHeight = staffline_height;
    }
    
    if (staffspace_height)
    {
        slf1.staffSpaceDistance = staffspace_height;
    }
    
    printf("Rows (image.nrows()): %lu, Rows (imageHeight): %d Columns (image.ncols()): %lu Columns (imageWidth): %d\n", image.nrows(), slf1.imageHeight, image.ncols(), slf1.imageWidth);
    
    return slf1.stableStaffDetection(validStaves);
}

template<class T>
OneBitImageView* drawAllStablePaths(T &image)
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    //OneBitImageView *new1 = slf1.myCloneImage(image);
    printf("Rows: %lu, Columns: %lu\n", image.nrows(), image.ncols());
    slf1.stableStaffDetection(validStaves);
    OneBitImageView *blank = slf1.clear(image);
    slf1.drawPaths(validStaves, blank);
    
    return blank;
}

template<class T>
GreyScaleImageView* displayWeights(T &image)
{
    stableStaffLineFinder slf1 (image);
    int ncols = image.ncols();
    int nrows = image.nrows();
    GreyScaleImageView *new1 = slf1.clearGrey(image);
    
    for (int col = 0; col < ncols - 1; col++)
    {
        for (int row = 0; row < nrows - 1; row++)
        {
            new1->set(Point(col, row), (2 * slf1.graphWeight[row*ncols+col].weight_up) + (2 * slf1.graphWeight[row*ncols+col].weight_down) + (2 * slf1.graphWeight[row*ncols+col].weight_hor));
        }
    }
    
    return new1;
}


template<class T>
OneBitImageView* findStablePaths(T &image) //Returns blank image with stable paths drawn
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    OneBitImageView *blank = slf1.clear(image);
    printf("Rows: %d, Columns: %d\n", slf1.imageHeight, slf1.imageWidth);
    printf("findAllStablePaths: %d\n", slf1.findAllStablePaths(slf1.primaryImage, 0, slf1.imageWidth - 1, validStaves));
    slf1.drawPaths(validStaves, blank);

    return blank;
}

template<class T>
RGBImageView* drawColorfulStablePaths(T &image)
{
    stableStaffLineFinder slf1 (image);
    RGBImageData *data1 = new RGBImageData(image.size());
    RGBImageView *new1 = new RGBImageView(*data1);
    vector<vector <Point> > validStaves;
    vector< vector <vector<Point> > > setsOfValidStaves;
    setsOfValidStaves = slf1.returnSetsOfStablePaths(validStaves, image);
    int redCount, blueCount, greenCount, counter;
    redCount = blueCount = greenCount = counter = 0;
    
    for (int set = 0; set < setsOfValidStaves.size(); set++)
    {
        if (counter == 0)
        {
            redCount = 255;
            greenCount = 0;
        }
        else if (counter == 1)
        {
            greenCount = 150;
            blueCount = 0;
            redCount = 0;
        }
        else if (counter == 2)
        {
            blueCount = 175;
            redCount = 0;
            counter = 0;
        }
        
        for (int staff = 0; staff < setsOfValidStaves[set].size(); staff++)
        {
            for (int line = 0; line < setsOfValidStaves[set][staff].size(); line++)
            {
                new1->set(setsOfValidStaves[set][staff][line], RGBPixel(redCount, greenCount, blueCount));
            }
        }
        
        counter++;
    }
    
    return new1;
}

template<class T>
RGBImageView* deletionStablePathDetection(T &image)
{
    stableStaffLineFinder slf1 (image);
    RGBImageData *data1 = new RGBImageData(image.size());
    RGBImageView *new1 = new RGBImageView(*data1);
    vector<vector <Point> > validStaves;
    OneBitImageView *firstPass = slf1.stableStaffDetection(validStaves);
    OneBitImageView *subtractedImage = slf1.subtractImage(image, *firstPass);
    validStaves.clear();
    stableStaffLineFinder slf2 (*subtractedImage);
    vector< vector <vector<Point> > > setsOfValidStaves;
    setsOfValidStaves = slf2.returnSetsOfStablePaths(validStaves, *subtractedImage);
    int redCount, blueCount, greenCount, counter;
    redCount = blueCount = greenCount = counter = 0;
    
    for (int set = 0; set < setsOfValidStaves.size(); set++)
    {
        if (counter == 0)
        {
            redCount = 255;
            greenCount = 0;
        }
        else if (counter == 1)
        {
            greenCount = 150;
            blueCount = 0;
            redCount = 0;
        }
        else if (counter == 2)
        {
            blueCount = 175;
            redCount = 0;
            counter = 0;
        }
        
        for (int staff = 0; staff < setsOfValidStaves[set].size(); staff++)
        {
            for (int line = 0; line < setsOfValidStaves[set][staff].size(); line++)
            {
                new1->set(setsOfValidStaves[set][staff][line], RGBPixel(redCount, greenCount, blueCount));
            }
        }
        
        counter++;
    }
    
    return new1;
}

template<class T>
GreyScaleImageView* testForVerticalBlackPercentage(T &image)
{
    stableStaffLineFinder slf1 (image);
    vector<vector <Point> > validStaves;
    GreyScaleImageView *new1 = slf1.clearGrey(image);
    OneBitImageView *firstPass = slf1.stableStaffDetection(validStaves);
    OneBitImageView *subtractedImage = slf1.subtractImage(image, *firstPass);
    validStaves.clear();
    stableStaffLineFinder slf2 (*subtractedImage);
    vector< vector <vector<Point> > > setsOfValidStaves;
    setsOfValidStaves = slf2.returnSetsOfStablePaths(validStaves, *subtractedImage);
    
    for (int set = 0; set < setsOfValidStaves.size(); set++)
    {
        for (int col = 0; col < slf1.imageWidth; col++)
        {
            for (int y = setsOfValidStaves[set][0][col].y(); y < setsOfValidStaves[set][setsOfValidStaves[set].size() - 1][col].y(); y++)
            {
                new1->set(Point(col, y), (255.0 * slf1.verticalBlackPercentage(col, setsOfValidStaves[set][0][col].y(), setsOfValidStaves[set][setsOfValidStaves[set].size() - 1][col].y())));
            }
        }
    }
    
    return new1;
}

template<class T>
RGBImageView* trimmedStablePaths(T &image, bool with_deletion, bool with_staff_fixing)
{
    if (with_deletion)
    {
        stableStaffLineFinder slf1 (image);
        RGBImageData *data1 = new RGBImageData(image.size());
        RGBImageView *new1 = new RGBImageView(*data1);
        vector<vector <Point> > validStaves;
        OneBitImageView *firstPass = slf1.stableStaffDetection(validStaves);
        OneBitImageView *subtractedImage = slf1.subtractImage(image, *firstPass);
        validStaves.clear();
        stableStaffLineFinder slf2 (*subtractedImage);
        vector< vector <vector<Point> > > setsOfValidStaves;
        setsOfValidStaves = slf2.returnSetsOfStablePaths(validStaves, *subtractedImage);
        vector< vector <vector<Point> > > setsOfTrimmedPaths;
        cout <<"About to commence finalTrim" <<endl;
        setsOfTrimmedPaths = slf2.finalTrim(setsOfValidStaves, slf1.primaryImage);
        cout <<"Finished finalTrim" <<endl;
        int redCount, blueCount, greenCount, counter;
        redCount = blueCount = greenCount = counter = 0;
        
        for (int set = 0; set < setsOfTrimmedPaths.size(); set++)
        {
            if (counter == 0)
            {
                redCount = 255;
                greenCount = 0;
            }
            else if (counter == 1)
            {
                greenCount = 150;
                blueCount = 0;
                redCount = 0;
            }
            else if (counter == 2)
            {
                blueCount = 175;
                redCount = 0;
                counter = 0;
            }
            
            if (with_staff_fixing)
            {
                slf2.fixStaffSystem(setsOfTrimmedPaths[set]);
            }
            
            for (int staff = 0; staff < setsOfTrimmedPaths[set].size(); staff++)
            {
                slf2.smoothStaffLine(setsOfTrimmedPaths[set][staff], SMOOTH_STAFF_LINE_WINDOW * slf2.staffSpaceDistance);
                for (int line = 0; line < setsOfTrimmedPaths[set][staff].size(); line++)
                {
    //                slf2.fixLine(setsOfTrimmedPaths[set][staff]);
                    new1->set(setsOfTrimmedPaths[set][staff][line], RGBPixel(redCount, greenCount, blueCount));
                }
            }
            
            counter++;
        }
        
        return new1;
    }
    else
    {
        stableStaffLineFinder slf1 (image);
        RGBImageData *data1 = new RGBImageData(image.size());
        RGBImageView *new1 = new RGBImageView(*data1);
        vector<vector <Point> > validStaves;
        vector< vector <vector<Point> > > setsOfValidStaves;
        setsOfValidStaves = slf1.returnSetsOfStablePaths(validStaves, *slf1.primaryImage);
        vector< vector <vector<Point> > > setsOfTrimmedPaths;
        cout <<"About to commence finalTrim" <<endl;
        setsOfTrimmedPaths = slf1.finalTrim(setsOfValidStaves, slf1.primaryImage);
        cout <<"Finished finalTrim" <<endl;
        int redCount, blueCount, greenCount, counter;
        redCount = blueCount = greenCount = counter = 0;
        
        for (int set = 0; set < setsOfTrimmedPaths.size(); set++)
        {
            if (counter == 0)
            {
                redCount = 255;
                greenCount = 0;
            }
            else if (counter == 1)
            {
                greenCount = 150;
                blueCount = 0;
                redCount = 0;
            }
            else if (counter == 2)
            {
                blueCount = 175;
                redCount = 0;
                counter = 0;
            }
            
            if (with_staff_fixing)
            {
                slf1.fixStaffSystem(setsOfTrimmedPaths[set]);
            }
            
            for (int staff = 0; staff < setsOfTrimmedPaths[set].size(); staff++)
            {
                slf1.smoothStaffLine(setsOfTrimmedPaths[set][staff], SMOOTH_STAFF_LINE_WINDOW * slf1.staffSpaceDistance);
                for (int line = 0; line < setsOfTrimmedPaths[set][staff].size(); line++)
                {
                    //                slf1.findSlopes(setsOfTrimmedPaths[set][staff]);
                    new1->set(setsOfTrimmedPaths[set][staff][line], RGBPixel(redCount, greenCount, blueCount));
                }
            }
            
            counter++;
        }
        
        return new1;
    }
}

template<class T>
PyObject* setOfStablePathPoints(T &image)
{
    stableStaffLineFinder slf1 (image);
    vector<vector <Point> > validStaves;
    vector< vector <vector<Point> > > setsOfValidStaves;
    setsOfValidStaves = slf1.returnSetsOfStablePaths(validStaves, *slf1.primaryImage);
    
    PyObject *return_list = PyList_New(0);
    return return_list;
}

void copyONEBITToRGB(OneBitImageView primaryImage, RGBImageView dest)
{
    int width = primaryImage.width();
    int height = primaryImage.height();
    
    for (int col = 0; col < width; col++)
    {
        for (int row = 0; row < height; row++)
        {
            int pixVal = primaryImage.get(Point(col, row));
            if (pixVal)
            {
                dest.set(Point(col, row), RGBPixel(0, 0, 0));
            }
        }
    }
}

void overlayRGBPixels(RGBImageView primaryImage, RGBImageView dest)
{
    int width = primaryImage.width();
    int height = primaryImage.height();
    
    for (int col = 0; col < width; col++)
    {
        for (int row = 0; row < height; row++)
        {
//            int pixVal = primaryImage->get(Point(col, row));
            if (primaryImage.get(Point(col, row)) != RGBPixel(255, 255, 255))
            {
                dest.set(Point(col, row), RGBPixel(0, 0, 0));
            }
        }
    }
}

template<class T, class U>
RGBImageView *overlayStaves(T &staffImage, U &primaryImage)
{
    int width = staffImage.width();
    int height = staffImage.height();
    
    if ((width != primaryImage.width()) || (height != primaryImage.height()))
    {
        cout <<"Sorry, those images are not the same size. Please pick two images of equal size.";
        exit(1);
    }
    
    RGBImageData *data = new RGBImageData(staffImage.size());
    RGBImageView *result = new RGBImageView(*data);
    
    for (int col = 0; col < width; col++)
    {
        for (int row = 0; row < height; row++)
        {
            int pixVal = primaryImage.get(Point(col, row));
            if (pixVal)
            {
                result->set(Point(col, row), RGBPixel(0, 0, 0));
            }
        }
    }
    
    for (int col = 0; col < width; col++)
    {
        for (int row = 0; row < height; row++)
        {
            //            int pixVal = primaryImage->get(Point(col, row));
            if (staffImage.get(Point(col, row)) != RGBPixel(255, 255, 255))
            {
                result->set(Point(col, row), staffImage.get(Point(col, row)));
            }
        }
    }
//    copyONEBITToRGB(*primaryImage, *result);
//    overlayRGBPixels(*staffImage, *result);
    
    return result;

}

#endif