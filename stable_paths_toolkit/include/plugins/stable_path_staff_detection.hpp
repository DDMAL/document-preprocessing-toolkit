
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

using namespace std;
using namespace Gamera;

#define CUSTOM_STAFF_LINE_HEIGHT 99999999
#define CUSTOM_STAFF_SPACE_HEIGHT 99999999
#define ALLOWED_DISSIMILARITY 2
#define ALLOWED_THICKNESS_OF_STAFFLINE_DELETION 3

//Copied from stableStaffLineFinder.h
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

    //Values taken from stableStaffLineFinder.cpp lines 106-107
    const double MIN_BLACK_PER = 0.25;
    static const weight_t TOP_VALUE = (INT_MAX/2);

    int* verRun; //length of vertical run of same color pixels. 
    int* verDistance; //Minimum distance of a pixel of the same color NOT in the same run of pixels of the same color
    NODE* graphPath;
    NODEGRAPH* graphWeight;

    int staffLineHeight = 0;
    int staffSpaceDistance = 0;
    time_t globalStart;

    typedef ImageData<OneBitPixel> OneBitImageData;
    typedef ImageView<OneBitImageData> OneBitImageView;

    typedef ImageData<GreyScalePixel> GreyScaleImageData;
    typedef ImageView<GreyScaleImageData> GreyScaleImageView;
    
    typedef ImageData<RGBPixel> RGBImageData;
    typedef ImageView<RGBImageData> RGBImageView;
    
    //OneBitImageView *primaryImage;

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
    
    template<class T>
    OneBitImageView* myCloneImagePNG(T &image)
    {
        int width = image.png_get_image_width();
        int height = image.png_get_image_height();
        
        OneBitImageData* dest_data = new OneBitImageData(Dim(width, height));
        OneBitImageView* dest_view = new OneBitImageView(*dest_data);
        
        for (size_t r = 0; r < height; r++)
        {
            for (size_t c = 0; c < width; c++)
            {
                //dest_view->set(Point(c, r), image.get(Point(c, r)));
            }
        }
        
        return dest_view;
    }

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

    void deletePaths(vector <vector<Point> > &validStaves, OneBitImageView *image)
    {
        unsigned long numPaths = validStaves.size();
        unsigned long numPix = validStaves[0].size();
        printf("numPaths: %lu, numPix: %lu\n", numPaths, numPix);
        
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
        unsigned long size = path.size();
        
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

    //=========================================================================================
    //                          Functions w/ Templates
    //=========================================================================================
    
    template<class T>
    stableStaffLineFinder(T &image)
    {
        graphPath = new NODE[image.nrows() * image.ncols()];
        graphWeight = new NODEGRAPH[image.nrows() * image.ncols()];
        verRun = new int[image.nrows() * image.ncols()];
        verDistance = new int[image.nrows() * image.ncols()];
        memset (verDistance, 0, sizeof(int) * image.nrows() * image.ncols());
//        OneBitImageView *copy = myCloneImage(image);
//        findStaffLineHeightandDistanceFinal(copy);
//        delete copy;
//        staffLineHeight = CUSTOM_STAFF_LINE_HEIGHT;
//        staffSpaceDistance = CUSTOM_STAFF_SPACE_HEIGHT;
        //primaryImage = myCloneImage(image);
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

    template<class T>
    void constructGraphWeights(T &image) 
    {
        unsigned char WHITE = 0;
        int cols = image.ncols();
        int rows = image.nrows();

        //Find vertical run values
        // ***USE VECTOR ITERATORS WITH ROW ON THE OUTSIDE TO INCREASE PERFORMANCE IF THERE'S TIME***

        cout <<"About to calculate vertical runs\n";

        for (int c = 0; c < cols; c++) 
        {
            int run = 0;
            unsigned char val = WHITE;
            
            for (int r = 0; r < rows; r++) 
            {
                unsigned char pel = image.get(Point(c, r));
                
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

        cout <<"Finished calculating vertical runs\n";

        //Find Vertical Distance
        for (int c = 0; c < cols; c++) 
        {
            //printf("Vertical Distance column %d\n", c);
            for (int r = 0; r < rows; r++) 
            {
                unsigned char pel = image.get(Point(c, r));
                int row = r;
                unsigned char curr_pel = pel;
                
                while ((row > 0) && (curr_pel == pel))
                {
                    row--;
                    curr_pel = image.get(Point(c, row));
                }

                int run1 = 1;
                
                while (row > 0 && curr_pel != pel) 
                {
                    row--;
                    curr_pel = image.get(Point(c, row));
                    run1++;
                }

                row = r;
                curr_pel = pel;
                
                while ((row < rows - 1) && (curr_pel == pel))
                {
                    row++;
                    curr_pel = image.get(Point(c, row));
                }

                int run2 = 1;
                
                while ((row < rows - 1) && (curr_pel != pel))
                {
                    row++;
                    curr_pel = image.get(Point(c, row));
                    run2++;
                }

                verDistance[(r * cols) + c] = min(run1, run2);
            }
        }

        cout <<"Finished calculating vertical distance\n";
        
        if ((!staffLineHeight) && (!staffSpaceDistance)) //No non-zero values yet assigned to staffLineHeight or staffSpaceDistance
        {
            findStaffLineHeightandDistanceFinalTemplate(image);
        }
        else
        {
            cout <<"staffLineHeight already set to: " <<staffLineHeight <<" staffSpaceDistance already set to: " <<staffSpaceDistance <<endl;
        }

        //Find Graph Weights
        for (int r = 0; r < rows; r++) 
        {
            for (int c = 0; c < cols - 1; c++) 
            {
                //printf("About to find weight for Point(%d, %d)\n", c, r);
                graphWeight[(r * cols) + c].weight_hor = weightFunction(image, Point(c, r), Point(c + 1, r), NEIGHBOUR4);
                
                if (r > 0)
                {
                    graphWeight[(r * cols) + c].weight_up = weightFunction(image, Point(c, r), Point(c + 1, r - 1), NEIGHBOUR8);
                }
                else
                {
                    graphWeight[(r * cols) + c].weight_up = TOP_VALUE;
                }
                
                if (r < rows - 1)
                {
                    graphWeight[(r * cols) + c].weight_down = weightFunction(image, Point(c, r), Point(c + 1, r + 1), NEIGHBOUR8);
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

    template<class T>
    weight_t weightFunction(T &image, Point pixelVal1, Point pixelVal2, stableStaffLineFinder::NEIGHBOUR neigh) 
    {
        unsigned int pel1 = image.get(pixelVal1); //Gets the pixel value of Point 1
        unsigned int pel2 = image.get(pixelVal2); //Gets pixel value of Point 2
        //int height = image.nrows();
        int width = image.ncols();

        int dist1 = verDistance[(pixelVal1.y() * width) + pixelVal1.x()]; //Vertical Distance taken from array of values created in constructGraphWeights
        int dist2 = verDistance[(pixelVal2.y() * width) + pixelVal2.x()];
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
        
        if ( (pel) && ( (min(vRun1, vRun2) <= staffLineHeight)) )
        {
            --y;
        }
        
        if (max(dist1, dist2) > ((2 * staffLineHeight) + staffSpaceDistance))
        {
            y++;
        }
        
        return y;
    }
    
    template<class T>
    int findAllStablePaths(T &image, int startCol, int endCol, vector <vector<Point> > &stablePaths)
    {
        stablePaths.clear();
        int width = image.ncols();
        int height = image.nrows();
        vector<int> startRow_i;
        
        int endCol_i = width - 1 - startCol;
        int startCol_i = width - 1 - endCol;
        
        for (int row = 0; row < height; row++)
        {
            graphPath[row*width + startCol_i].weight = static_cast<weight_t>(0);
            graphPath[row*width + startCol_i].start = Point(startCol_i, row);
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
                
                weight20 = graphWeight[row*width + width-1-col].weight_hor;
                value2 = weight20 + graphPath[(row)*width + (col-1)].weight;
                
                if (row < height-1)
                {
                    weight30 = graphWeight[row*width + width-1-col].weight_down;
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
            startRow_i.push_back(graphPath[i*width + endCol_i].start.y());
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
            int startR = graphPath[(i * width) + endCol].start.y();
            
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
        //findStaffHeightandDistance(image, stablePaths); 
        return 0;
    }
    
    template<class T>
    OneBitImageView* stableStaffDetection(vector <vector <Point> > &validStaves, T &image)
    {
        constructGraphWeights(image);
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
            findAllStablePathsView(imageCopy, 0, ncols - 1, stablePaths);
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
                for (size_t i = 0; i<staff.size(); i++)
                {
                    //printf("Staff Size: %lu\n", staff.size());
                    int col = staff[i].x();
                    int row = staff[i].y();
                    
                    //ERASE PATHS ALREADY SELECTED!
                    for (int j =-path_half_width2; j <= path_half_width2; j++)
                    {
                        // printf("path_half_width2 = %d, j = %d\n", path_half_width2, j);
                        // printf("Currently erasing lines\n");
                        if ( ((row + j) > nrows - 1) || ((row + j) < 0) )
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
        postProcessing(validStaves, staffSpaceDistance, imageErodedCopy);
        printf ("TOTAL = %lu TOTAL STAFF LINES\n", validStaves.size());
        OneBitImageView *blank = clear(image);
        //drawPaths(validStaves, blank);
        //return blank;
        
        return imageCopy;
    }

    //TODO: increase speed with vector iterators
    //Currently not being used
    template<class T>
    void findStaffHeightandDistance(T &image, vector<vector<Point> >&stablePaths)
    {
        unsigned char WHITE = 0;
        vector<int> runs[2];

        for (int i=0; i < stablePaths.size(); i++)
        {
            vector<Point> &staff = stablePaths[i];
            
            for (int j=0; j < staff.size(); j++)
            {
                Point curr = staff[j];
                int col = curr.x();
                int row = curr.y();
                unsigned char pel = image.get(getPoint(row*(image.ncols()) + col, image));
                int runBlack = -1;
                
                while (pel)
                {
                    --row;
                    ++runBlack;
                    
                    if (row < 0)
                    {
                        break;
                    }
                    
                    pel = image.get(getPoint(row*(image.ncols()) + col, image));
                }

                int runWhite = 0;
                
                while (row >= 0 && (pel = image.get(getPoint(row*(image.ncols()) + col, image))) == WHITE)
                {
                    ++runWhite;
                    --row;
                }
                
                runs[1].push_back(runWhite);
                row = curr.y();
                pel = image.get(getPoint(row*(image.ncols()) + col, image));
                
                while (pel)
                {
                    ++row;
                    ++runBlack;
                    
                    if (row > image.nrows()-1)
                    {
                        break;
                    }
                    
                    pel = image.get(getPoint(row*(image.ncols()) + col, image));
                }

                runWhite = 0;
                
                while (row < image.nrows() && (pel = image.get(getPoint(row*(image.ncols()) + col, image)) == WHITE))
                {
                    ++runWhite;
                    ++row;
                }

                runs[1].push_back(runWhite);
                runs[0].push_back(runBlack);
            }
        }

        //Now find the most repeated black runs and white runs
        sort(runs[0].begin(), runs[0].end());
        sort(runs[1].begin(), runs[1].end());

        vector<int> v = runs[0];
        staffSpaceDistance = -1;

        if (!v.size())
        {
            staffLineHeight = 0;
            staffSpaceDistance = 0;
        }
        else
        {
            staffLineHeight = findMostRepresentedValueOnSortedVector<int>(runs[0]);
            staffSpaceDistance = findMostRepresentedValueOnSortedVector<int>(runs[1]);
        }
    }
    
    template<class T>
    vector <vector <vector<Point> > > returnSetsOfStablePaths(vector <vector <Point> > &validStaves, T &image)
    {
        constructGraphWeights(image);
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
            findAllStablePathsView(imageCopy, 0, ncols - 1, stablePaths);
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
                for (size_t i = 0; i<staff.size(); i++)
                {
                    //printf("Staff Size: %lu\n", staff.size());
                    int col = staff[i].x();
                    int row = staff[i].y();
                    
                    //ERASE PATHS ALREADY SELECTED!
                    for (int j =-path_half_width2; j <= path_half_width2; j++)
                    {
                        // printf("path_half_width2 = %d, j = %d\n", path_half_width2, j);
                        // printf("Currently erasing lines\n");
                        if ( ((row + j) > nrows - 1) || ((row + j) < 0) )
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
        return postProcessing(validStaves, staffSpaceDistance, imageErodedCopy);
//        printf ("TOTAL = %lu TOTAL STAFF LINES\n", validStaves.size());
//        OneBitImageView *blank = clear(image);
        //drawPaths(validStaves, blank);
        //return blank;
        
        //return imageCopy;
    }

    //=============================================================================
    //                          HELPER FUNCTION
    //=============================================================================
    
    template <class T>
    void findStaffLineHeightandDistanceFinalTemplate(T &image)
    {
        int width = image.ncols();
        int height = image.nrows();
        vector<int> values;
        vector<int> runs[2];
        vector<int> sum2runs;
        runs[0].resize(height + 1, 0);
        runs[1].resize(height + 1, 0);
        
        for (int c = 0; c < width; c++)
        {
            for (int r = 0; r < height; r++)
            {
                unsigned int pel = image.get(Point(c, r));
                
                if (pel) //Pixel is black
                {
                    ++runs[1][verRun[(r * width) + c]];
                    ++runs[0][verDistance[(r * width) + c]];
                    r += verRun[(r * width) + c];
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
                if (runs[1][i] > maxcounter)
                {
                    maxcounter = runs[1][i];
                    staffLineHeight = i;
                }
            }
        }
        cout <<"Staff Height = " <<staffLineHeight <<" Staff Distance = " <<staffSpaceDistance <<endl;
    }
    
    template <class T>
    T findMostRepresentedValueOnSortedVector(vector<T>& vec)
    {
        T run = vec[0];
        int freq = 0;
        int maxFreq = 0;
        T maxRun = run;

        for (unsigned i = 0; i< vec.size(); i++)
        {
            if (vec[i] == run)
            {
                freq ++;
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
        double currAvg1 = 0;
        double currAvg2 = 0;
        
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
            //unsigned char pel = image->get(getPointView(((row * image->ncols()) + col), image->ncols(), image->nrows()));
            unsigned char pel = image->get(Point(col, row));
            sumOfValues += pel;
        }
        
        return sumOfValues;
    }
    
    bool tooMuchWhite (vector<Point> &vec, OneBitImageView *image, double minBlackPerc)
    {
        int sumOfValues = sumOfValuesInVector(vec, image);
        //size_t len = vec.size();
        int startCol = 0;
        int endCol = image->ncols() - 1;
        double usedSize = endCol - startCol + 1.0;
        
        if (sumOfValues < (1.0 - minBlackPerc) * (usedSize))
        {
            return true;
        }
        
        return false;
    }
    
    //===================================================================================================
    //================================== OneBitImageView Functions ======================================
    //===================================================================================================
    
    int findAllStablePathsView(OneBitImageView *image, int startCol, int endCol, vector <vector<Point> > &stablePaths)
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
                    weight10 = graphWeight[(row*width) + width-1-col].weight_up;
                    value1 = weight10 + graphPath[(row-1)*width + (col-1)].weight;
                }
                
                weight20 = graphWeight[row*width + width-1-col].weight_hor;
                value2 = weight20 + graphPath[(row)*width + (col-1)].weight;
                
                if (row < height - 1)
                {
                    weight30 = graphWeight[row*width + width-1-col].weight_down;
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
                    graphPath[(row)*width + (col)].previous = Point(col - 1, row);
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
            startRow_i.push_back(graphPath[i*width + endCol_i].start.y());
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
    
    void constructGraphWeightsView(OneBitImageView *image)
    {
        unsigned char WHITE = 0;
        int cols = image->ncols();
        int rows = image->nrows();
        
        //Find vertical run values
        // ***USE VECTOR ITERATORS WITH ROW ON THE OUTSIDE TO INCREASE PERFORMANCE IF THERE'S TIME***
        cout <<"About to find vertical runs" <<endl;
        
        for (int c = 0; c < cols; c++)
        {
            int run = 0;
            unsigned char val = WHITE;
            
            for (int r = 0; r < rows; r++)
            {
                unsigned char pel = image->get(Point(c, r));
                
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
        for (int c = 0; c < cols; c++)
        {
            for (int r = 0; r < rows; r++)
            {
                //cout <<"Finding vertical distance of Point(" <<c <<", " <<r <<")" <<endl;
                unsigned char pel = image->get(Point(c, r));
                int row = r;
                unsigned char curr_pel = pel;
                
                while ((row > 0) && (curr_pel == pel))
                {
                    row--;
                    curr_pel = image->get(Point(c, row));
                }
                
                int run1 = 1;
                
                while ((row > 0) && (curr_pel != pel))
                {
                    row--;
                    curr_pel = image->get(Point(c, row));
                    run1++;
                }
                
                row = r;
                curr_pel = pel;
                
                while ((row < rows - 1) && (curr_pel == pel))
                {
                    row++;
                    curr_pel = image->get(Point(c, row));
                }
                
                int run2 = 1;
                
                while ((row < rows - 1) && (curr_pel != pel))
                {
                    row++;
                    curr_pel = image->get(Point(c, row));
                    run2++;
                }
                
                verDistance[(r * cols) + c] = min(run1, run2);
            }
        }
        
        if ((!staffLineHeight) && (!staffSpaceDistance)) //No values yet assigned to staffLineHeight or staffSpaceDistance
        {
            findStaffLineHeightandDistanceFinal(image);
        }
        
        //Find Graph Weights
        for (int r = 0; r < rows; r++)
        {
            for (int c = 0; c < cols - 1; c++)
            {
                graphWeight[(r * cols) + c].weight_hor = weightFunctionView(image, Point(c, r), Point(c + 1, r), NEIGHBOUR4);
                
                if (r > 0)
                {
                    graphWeight[(r * cols) + c].weight_up = weightFunctionView(image, Point(c, r), Point(c + 1, r - 1), NEIGHBOUR8);
                }
                else
                {
                    graphWeight[(r * cols) + c].weight_up = TOP_VALUE;
                }
                
                if (r < rows - 1)
                {
                    graphWeight[(r * cols) + c].weight_down = weightFunctionView(image, Point(c, r), Point(c + 1, r + 1), NEIGHBOUR8);
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
    
    weight_t weightFunctionView(OneBitImageView *image, Point pixelVal1, Point pixelVal2, stableStaffLineFinder::NEIGHBOUR neigh)
    {
        unsigned int pel1 = image->get(pixelVal1); //Gets the pixel value of Point 1
        unsigned int pel2 = image->get(pixelVal2); //Gets pixel value of Point 2
        int width = image->ncols();
        //int height = image->nrows();
        
        int dist1 = verDistance[(pixelVal1.y() * width) + pixelVal1.x()]; //Vertical Distance taken from array of values created in constructGraphWeights
        int dist2 = verDistance[(pixelVal2.y() * width) + pixelVal2.x()];
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
        
        if ( (pel) && ( (min(vRun1, vRun2) <= staffLineHeight)) )
        {
            --y;
        }
        
        if (max(dist1, dist2) > ((2 * staffLineHeight) + staffSpaceDistance))
        {
            y++;
        }
        
        return y;
    }

    void findStaffLineHeightandDistanceFinal(OneBitImageView *image)
    {
        int width = image->ncols();
        int height = image->nrows();
        vector<int> values;
        vector<int> runs[2];
        vector<int> sum2runs;
        runs[0].resize(height + 1, 0);
        runs[1].resize(height + 1, 0);
        
        for (int c = 0; c < width; c++)
        {
            for (int r = 0; r < height; r++)
            {
                unsigned int pel = image->get(Point(c, r));
                
                if (pel) //Pixel is black
                {
                    ++runs[1][verRun[(r * width) + c]];
                    ++runs[0][verDistance[(r * width) + c]];
                    r += verRun[(r * width) + c];
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
                if (runs[1][i] > maxcounter)
                {
                    maxcounter = runs[1][i];
                    staffLineHeight = i;
                }
            }
        }
        cout <<"Staff Height = " <<staffLineHeight <<" Staff Distance = " <<staffSpaceDistance <<endl;
    }

    vector <vector <vector<Point> > > postProcessing(vector <vector<Point> > &validStaves, int staffDistance, OneBitImageView *imageErodedCopy)
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
        
        if (abs(staffDistance - staffLineHeight) < (0.5 * max(staffDistance, staffLineHeight)))
        {
            maxStaffDistance = max(3 * staffDistance, 3 * staffLineHeight);
        }
        else
        {
            maxStaffDistance = max(2 * staffDistance, 2 * staffLineHeight);
        }

        printf("maxStaffDistance = %d\n", maxStaffDistance);

        //Organize in sets
//        vector <vector <vector<Point> > > setsOfValidStaves;
        int start = 0;
        
        for (size_t nvalid = 0; nvalid < validStaves.size(); nvalid++)
        {
            if (nvalid == validStaves.size() - 1 || simplifiedAvgDistance(validStaves[nvalid], validStaves[nvalid + 1]) > maxStaffDistance)
            {
                vector <vector<Point> > singleSet;
                
                for (int i = start; i <= nvalid; i++)
                {
                    singleSet.push_back(validStaves[i]);
                }
                
                if (singleSet.size() >= 2) //Paper writers wanted more complex rules to validate sets
                {
                    setsOfValidStaves.push_back(singleSet);
                    printf("SET SIZE = %lu\n", singleSet.size());
                }
                
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

                    //setOfStaves[nvalid][c].y() = min(max(y, 0), nrowsEroded - 1);
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
            vector<unsigned char> medianStaff;
            
            for (int c = 0; c < ncolsEroded; c++)
            {
                vector <unsigned char> medianValue;
                
                for (int i = 0; i < setOfStaves.size(); i++)
                {
                    int x = setOfStaves[i][c].x();
                    int y = setOfStaves[i][c].y();
                    unsigned char pel = imageErodedCopy->get(getPointView((y * ncolsEroded) + x, ncolsEroded, nrowsEroded));
                    medianValue.push_back(pel);
                }
                
                sort(medianValue.begin(), medianValue.end());
                medianStaff.push_back(medianValue[medianValue.size() / 5]); //Assumes stafflines in groups of 5
            }
            //1 find start and end
            int startx = 0, endx = ncolsEroded - 1;

//    #ifdef TRIMDEST
            trimPath(medianStaff, (2 * staffDistance), startx, endx);
//    #endif
            if ( (endx - startx) < maxStaffDistance) //remove whole set
            {
                set_it = setsOfValidStaves.erase(set_it);
                continue;
            }
            
            //2 trim staffs from start to nvalid
            for (int i = 0; i < setOfStaves.size(); i++)
            {
                smoothStaffLine(setOfStaves[i], 2 * staffDistance);

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
            if (sum < (window * 1 * 0.9)) //Original code accounted for greyscale, may be problematic
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
            //staff[i/2].y = accumY/(i+1);
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
    
//    OneBitImageView* lineDraw(OneBitImageView *image)
//    {
//        int height = image->nrows();
//        int width = image->ncols();
//        
//        for (int y = height/5; y < height-1; y += (height / 2))
//        {
//            for (int x = 0; x < width - 1; x++)
//            {
//                image->set(Point(x, y), 1);
//            }
//        }
//        
//        return image;
//    }
};

//===================================================================================================
//================================ Plugins ==========================================================
//===================================================================================================

template<class T>
float returnGraphWeights(T &image) 
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    OneBitImageView *new1 = slf1.myCloneImage(image);
    slf1.constructGraphWeightsView(new1);
    //slf1.findStaffLineHeightandDistanceFinal(new1);
    return slf1.staffLineHeight;
}

template<class T>
OneBitImageView* deleteStablePaths(T &image)
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    OneBitImageView *new1 = slf1.myCloneImage(image);
    printf("Rows: %lu, Columns: %lu\n", image.nrows(), image.ncols());
    //slf1.myVerticalErodeImage(new1, image.ncols(), image.nrows());
    slf1.constructGraphWeights(image);
    printf("findAllStablePaths: %d\n", slf1.findAllStablePaths(image, 0, image.ncols()-1, validStaves));
    slf1.deletePaths(validStaves, new1);
    //slf1.stableStaffDetection(validStaves, image);
    //return slf1.stableStaffDetection(validStaves, image);
    //slf1.~stableStaffLineFinder(); //deletes arrays
    return new1;
}

template<class T>
OneBitImageView* stablePathDetection1(T &image, int staffline_height, int staffspace_height)
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    slf1.staffLineHeight = staffline_height;
    slf1.staffSpaceDistance = staffspace_height;
    //OneBitImageView *new1 = slf1.myCloneImage(image);
    printf("Rows: %lu, Columns: %lu\n", image.nrows(), image.ncols());
    //slf1.myVerticalErodeImage(new1, image.ncols(), image.nrows());
    //slf1.constructGraphWeights(image);
    //printf("findAllStablePaths: %d\n", slf1.findAllStablePaths(image, 0, image.ncols()-1, validStaves));
    //slf1.deletePaths(validStaves, new1);
    //slf1.stableStaffDetection(validStaves, image);
    //slf1.~stableStaffLineFinder(); //deletes arrays
    return slf1.stableStaffDetection(validStaves, image);
}

template<class T>
OneBitImageView* stablePathDetectionDraw(T &image)
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    //OneBitImageView *new1 = slf1.myCloneImage(image);
    printf("Rows: %lu, Columns: %lu\n", image.nrows(), image.ncols());
    //slf1.myVerticalErodeImage(new1, image.ncols(), image.nrows());
    //slf1.constructGraphWeights(image);
    //printf("findAllStablePaths: %d\n", slf1.findAllStablePaths(image, 0, image.ncols()-1, validStaves));
    // slf1.deletePaths(validStaves, new1);
    //slf1.stableStaffDetection(validStaves, image);
    slf1.stableStaffDetection(validStaves, image);
    //slf1.~stableStaffLineFinder(); //deletes arrays
    OneBitImageView *blank = slf1.clear(image);
    slf1.drawPaths(validStaves, blank);
    return blank;
}

template<class T>
GreyScaleImageView* displayWeights(T &image) //Currently doesn't work...
{
    stableStaffLineFinder slf1 (image);
    int ncols = image.ncols();
    int nrows = image.nrows();
    slf1.constructGraphWeights(image);
    GreyScaleImageView *new1 = slf1.clearGrey(image);
    
    for (int col = 0; col < ncols - 1; col++)
    {
        for (int row = 0; row < nrows - 1; row++)
        {
            new1->set(Point(col, row), (2 * slf1.graphWeight[row*ncols+col].weight_up) + (2 * slf1.graphWeight[row*ncols+col].weight_down) + (2 * slf1.graphWeight[row*ncols+col].weight_hor));
            //new1->set(Point(col, row), slf1.graphWeight[row*ncols+col].weight_up + slf1.graphWeight[row*ncols+col].weight_down);
        }
    }
    
    //slf1.~stableStaffLineFinder(); //deletes arrays
    return new1;
}


template<class T>
OneBitImageView* findStablePaths(T &image) //Returns blank image with stable paths drawn
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    OneBitImageView *blank = slf1.clear(image);
    printf("Rows: %lu, Columns: %lu\n", image.nrows(), image.ncols());
    slf1.constructGraphWeights(image);
    printf("findAllStablePaths: %d\n", slf1.findAllStablePaths(image, 0, image.ncols()-1, validStaves));
    slf1.drawPaths(validStaves, blank);
    //slf1.~stableStaffLineFinder(); //deletes arrays
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
    int redCount, blueCount, greenCount;
    
    for (int set = 0; set < setsOfValidStaves.size(); set++)
    {
        for (int staff = 0; staff < setsOfValidStaves[set].size(); staff++)
        {
            for (int line = 0; line < setsOfValidStaves[set][staff].size(); line++)
            {
                new1->set(setsOfValidStaves[set][staff][line], RGBPixel((set * 100), 255 - (set * 100), 255 - (set)));
            }
        }
    }
    return new1;
}

#endif