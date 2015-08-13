#ifndef STABLE_PATH_STAFF_DETECTION_GREYSCALE
#define STABLE_PATH_STAFF_DETECTION_GREYSCALE

#include <vector>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <math.h>
#include "gameramodule.hpp"
#include "gamera.hpp"
#include "png.h"
#include "knnmodule.hpp"
#include "plugins/arithmetic.hpp"
#include "stable_path_staff_detection.hpp"
#include <time.h>

using namespace std;
using namespace Gamera;

#define MIN_BLACK_PER 0.25
#define CUSTOM_STAFF_LINE_HEIGHT 0
#define CUSTOM_STAFF_SPACE_HEIGHT 0
#define ALLOWED_DISSIMILARITY 3
#define ALLOWED_THICKNESS_OF_STAFFLINE_DELETION 2
#define ALLOWED_DISSIMILARITY_STAFF_LINE_HEIGHT_IN_WEIGHT_CONSTRUCTION 1
#define ALLOWED_VERTICAL_BLACK_PERCENTAGE .99
#define ALLOWED_VERTICAL_HIT_PERCENTAGE .50
#define ALLOWED_OFFSET_NEARHIT 1
#define SMOOTH_STAFF_LINE_WINDOW 2
#define SLOPE_WINDOW 2
#define SLOPE_TOLERANCE 1.3
#define VERBOSE_MODE 0
#define SLOPE_TOLERANCE_OFFSET .05
#define ALLOWED_MIN_BLACK_PERC .5
#define SSP_TOLERANCE 1

class stableStaffLineFinderGrey {
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
    //    const double MIN_BLACK_PER = 0.25;
    static const weight_t TOP_VALUE = (INT_MAX/2);
    
    int* verRun; //length of vertical run of same color pixels.
    int* verDistance; //Minimum distance of a pixel of the same color NOT in the same run of pixels of the same color
    NODE* graphPath;
    NODEGRAPH* graphWeight;
    bool* strongStaffPixels; //Array indicating which points are strong staff-pixels
    
    int staffLineHeight;
    int staffSpaceDistance;
    time_t globalStart;
    
    typedef ImageData<OneBitPixel> OneBitImageData;
    typedef ImageView<OneBitImageData> OneBitImageView;
    
    typedef ImageData<GreyScalePixel> GreyScaleImageData;
    typedef ImageView<GreyScaleImageData> GreyScaleImageView;
    
    typedef ImageData<RGBPixel> RGBImageData;
    typedef ImageView<RGBImageData> RGBImageView;
    
    GreyScaleImageData *primaryImageData; //Will be the data for the image being input from the python side
    GreyScaleImageView *primaryImage; //Will be the view for the image being input from the python side
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
    GreyScaleImageView* clear(T& image)
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
    GreyScaleImageView* myCloneImage(T &image)
    {
        GreyScaleImageData* dest_data = new GreyScaleImageData(image.size());
        GreyScaleImageView* dest_view = new GreyScaleImageView(*dest_data);
        
        for (size_t r = 0; r < image.nrows(); r++)
        {
            for (size_t c = 0; c < image.ncols(); c++)
            {
                dest_view->set(Point(c, r), image.get(Point(c, r)));
            }
        }
        
        return dest_view;
    }
    
    void printPoint(Point p)
    {
        printf("(%lu, %lu)\n", p.x(), p.y());
    }
    
    //=========================================================================================
    //=============================Primary Functions===========================================
    //=========================================================================================
    
    template<class T>
    stableStaffLineFinderGrey(T &image) //Initializes the stableStaffLineFinder class and its values
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
        strongStaffPixels = new bool[imageWidth * imageHeight];
        
        constructGraphWeights();
    }
    
    stableStaffLineFinderGrey()
    {
        //Allows you to use the functions without having to compute anything
        graphPath = new NODE[0];
        graphWeight = new NODEGRAPH[0];
        verRun = new int[0];
        verDistance = new int[0];
    }
    
    ~stableStaffLineFinderGrey ()
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
        unsigned char WHITE = 255;
        int cols = primaryImage->ncols();
        int rows = primaryImage->nrows();
        
        //Find vertical run values
        // ***USE VECTOR ITERATORS WITH ROW ON THE OUTSIDE TO INCREASE PERFORMANCE IF THERE'S TIME***
        if (VERBOSE_MODE)
        {
            cout <<"About to find vertical runs" <<endl;
        }
        
        for (int c = 0; c < cols; c++)
        {
            int run = 0;
            unsigned char val = WHITE;
            
            for (int r = 0; r < rows; r++)
            {
                unsigned int pel = primaryImage->get(Point(c, r));
                
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
                    val = pel; //Changes value from 0 to 1 or from 1 to
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
        
        if (VERBOSE_MODE)
        {
            cout <<"Done finding vertical runs" <<endl;
        }
        
        if ((!staffLineHeight) && (!staffSpaceDistance)) //No values yet assigned to staffLineHeight or staffSpaceDistance
        {
            findStaffLineHeightAndDistance();
        }
        
//        determineStrongStaffPixels();
        
        //Find Graph Weights
//        for (int r = 0; r < rows; r++)
//        {
//            for (int c = 0; c < cols - 1; c++)
//            {
//                graphWeight[(r * cols) + c].weight_hor = weightFunction(primaryImage, Point(c, r), Point(c + 1, r), NEIGHBOUR4);
//                
//                if (r > 0)
//                {
//                    graphWeight[(r * cols) + c].weight_up = weightFunction(primaryImage, Point(c, r), Point(c + 1, r - 1), NEIGHBOUR8);
//                }
//                else
//                {
//                    graphWeight[(r * cols) + c].weight_up = TOP_VALUE;
//                }
//                
//                if (r < rows - 1)
//                {
//                    graphWeight[(r * cols) + c].weight_down = weightFunction(primaryImage, Point(c, r), Point(c + 1, r + 1), NEIGHBOUR8);
//                }
//                else
//                {
//                    graphWeight[(r * cols) + c].weight_down = TOP_VALUE;
//                }
//            }
//        }
        
//        for (int x = 0; x < rows * cols; x++)
//        {
//            printf("Value: %d -> verDistance: %d -> verRun: %d\n", x, verDistance[x], verRun[x]);
//        }
    }
    
//    weight_t weightFunction(GreyScaleImageView *image, Point pixelVal1, Point pixelVal2, NEIGHBOUR neigh)
//    {
//        unsigned int pel1 = primaryImage->get(pixelVal1); //Gets the pixel value of Point 1
//        unsigned int pel2 = primaryImage->get(pixelVal2); //Gets pixel value of Point 2
//        
//        int vRun1 = verRun[(pixelVal1.y() * imageWidth) + pixelVal1.x()];
//        int vRun2 = verRun[(pixelVal2.y() * imageWidth) + pixelVal2.x()];
//        
//        int pel = min(pel1, pel2);
//        
//        //Weights for a 4 Neighbourhood
//        int y0 = 4; //Black
//        int y255 = 8; //White
//        
//        if (neigh == NEIGHBOUR8)
//        {
//            y0 = 6; //Black
//            y255 = 12; //White
//        }
//        
//        //Weight function
//        //w(p) = b/(1 + exp(-(p - q)/b)) + a
//        
//        int y =
//    }
    
    void determineStrongStaffPixels()
    {
        
    }
    
    void findStaffLineHeightAndDistance()
    {
        int minValue = 255;
        int maxValue = 0;
        vector <int> values;
    
        for (int c = 0; c < imageWidth; c++)
        {
            for(int r = 0; r < imageHeight; r++)
            {
                unsigned char pel = primaryImage->get(Point(c, r));
                minValue = min<int>(minValue, pel);
                maxValue = max<int>(maxValue, pel);
                values.push_back(static_cast<int>(pel));
            }
        }
    
        sort(values.begin(), values.end());
        int medValue = values[values.size()/2];
        int firstValue = values[0.01*values.size()];
        printf ("minValue %d; maxValue %d; firstValue %d; median Value %d\n", minValue, maxValue, firstValue, medValue);
        unsigned char WHITE = 255;
        vector<int> runs[2];
        vector<int> sum2runs;
        runs[0].resize(imageHeight + 1, 0);
        runs[1].resize(imageHeight + 1, 0);
        sum2runs.resize(imageHeight + 1, 0);
        int *hist2d = new int [(imageHeight + 1) * (imageHeight + 1)];
        memset(hist2d, 0, (imageHeight + 1) * (imageHeight + 1) * sizeof(int));
        
        for (int value = minValue + 1; value <= medValue; value++)
        {
            if (!(value % 10)) printf("value %d \n", value);
                for (int c = 0; c < imageWidth; c++)
                {
                    int run = 0;
                    int last_run = 0;
                    unsigned char val = WHITE;
                    for(int r = 0; r < imageHeight; r++)
                    {
                        unsigned char pel = primaryImage->get(Point(c, r));
                        //beginOf implicit horizontal dilate + erode to remove black noise
                        unsigned char pel_left2 = WHITE;
                        unsigned char pel_left1 = WHITE;
                        unsigned char pel_right1 = WHITE;
                        unsigned char pel_right2 = WHITE;
                        if (c > 0) pel_left1 = primaryImage->get(Point(c - 1, r));
                        if (c > 1) pel_left2 = primaryImage->get(Point(c - 2, r));
                        if (c < (imageWidth - 1)) pel_right1 = primaryImage->get(Point(c + 1, r));
                        if (c < (imageWidth - 2)) pel_right2 = primaryImage->get(Point(c + 2, r));
                        
                        pel = min(max(pel_left2, pel_left1), max (pel_left1, pel));
                        //endOf implicit horizontal dilate + erode to remove black noise
                        if (pel>=value) pel=WHITE;
                        else pel = 0;
                        
                        if (pel == val)
                        {
                            run++;
                        }
                        else
                        {
                            ++runs[val/WHITE][run];
                            ++sum2runs[run+last_run];
                            
                            if (val==0)
                            {
                                ++hist2d[(run * (imageHeight + 1)) + last_run];
                            }
                            else
                            {
                                ++hist2d[(last_run * (imageHeight + 1)) + run];
                            }
                            
                            val = WHITE - val;
                            last_run = run;
                            run = 1;
                        }
                    }
                    
                    ++runs[val / WHITE][run];
                    ++sum2runs[run + last_run];
                    if (!val)
                    {
                        ++hist2d[(run * (imageHeight + 1)) + last_run];
                    }
                    else
                    {
                        ++hist2d[(last_run * (imageHeight + 1)) + run];
                    }
                }
            //		break; //REMOVE FOR GRAY-LEVEL IMGS
        }
        
        // find most repeated
        {
            int maxcounter = 0;
            for (int i = 0; i < runs[0].size(); i++)
            {
                //fprintf(fp, "%d;%d;%d\n", 0, i, runs[0][i]);
                if (runs[0][i]> maxcounter)
                {
                    maxcounter=runs[0][i]; staffLineHeight = i;
                }
            }
        }
    
        {
            int maxcounter = 0;
            
            for (int i = 0; i < runs[1].size(); i++)
            {
                if (runs[1][i]> maxcounter)
                {
                    maxcounter=runs[1][i];
                    staffSpaceDistance = i;
                }
            }
        }
    
        int staffHeightDistance = 0;
    
        {	
            int maxsum = 0;
            
            for (int i = 0; i < sum2runs.size(); i++)
            {
                if (sum2runs[i]> maxsum)
                {
                    maxsum = sum2runs[i];
                    staffHeightDistance = i;
                }
            }
        }
    
        int b_run, w_run;
    
        {	
            int maxvalue = 0;
            for(int i = 0; i <= staffHeightDistance; i++)
            {
                int j = staffHeightDistance - i;
                if (hist2d[(i * (imageHeight + 1)) + j] > maxvalue)
                {
                    maxvalue = hist2d[(i * (imageHeight + 1)) + j];
                    b_run = i;
                    w_run = j;
                }
            }
        }
    
        delete hist2d;
        
        cout << "staffHeight " << staffLineHeight <<" staffDistance " << staffSpaceDistance << endl;
        cout << "staffHeight " << b_run <<" staffDistance " << w_run << " staffHeightDistance " << staffHeightDistance<< endl; 
        staffLineHeight = b_run;
        staffSpaceDistance = w_run;
        return;
    }

};

#endif