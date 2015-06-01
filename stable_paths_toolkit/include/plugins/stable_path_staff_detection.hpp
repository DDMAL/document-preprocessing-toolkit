
  //=================== STABLE PATH FUNCTIONS ===================
  //=============================================================

  /*
    Preprocessing:
        *1. Compute staffspaceheight and stafflineheight (Need to relax values so that similar staff spaces/line heights are taken together. Also need to make it so only most common
                value from each column is considered)
        *2. Compute weights of the graph

    Main Cycle:
        *1. Compute stable paths
        2. Validate paths with blackness and shape
        3. Erase valid paths from image
        4. Add valid paths to list of stafflines
        5. End of cycle if no valid path was found

    Postprocessing
        1. Uncross stafflines
        2. Organize stafflines in staves
        3. Smooth and trim stafflines

        Notes:
            - Big difference is that in original code the values are in grayscale
            - Currently being implemented only for one bit images
    */

#include <vector>
#include <algorithm>
#include "clear.hpp"
#include "gamera.hpp"


using namespace std;
using namespace Gamera;

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

    string img_path;
    int staffLineHeight;
    int staffSpaceDistance;
    time_t globalStart;

    typedef ImageData<OneBitPixel> OneBitImageData;
    typedef ImageView<OneBitImageData> OneBitImageView;

    //=========================================================================================
    //                          Image Cloners/Eroders
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
        int numPaths = validStaves.size();
        int numPix = validStaves[0].size();
        printf("numPaths: %d, numPix: %d\n", numPaths, numPix);
        for (int i = 0; i < numPaths; i++)
        {
            for (int x = 0; x < numPix; x++)
            {
                image->set(validStaves[i][x], 0);
            }
        }
    }

    void drawPaths(vector <vector<Point> > &validStaves, OneBitImageView *image)
    {
        int numPaths = validStaves.size();
        int numPix = validStaves[0].size();
        printf("numPaths: %d, numPix: %d\n", numPaths, numPix);
        for (int i = 0; i < numPaths; i++)
        {
            for (int x = 0; x < numPix; x++)
            {
                image->set(validStaves[i][x], 1);
            }
        }
    }

    //=========================================================================================
    //                          Functions
    //=========================================================================================

    template<class T>
    void constructGraphWeights(T &image) 
    {
        unsigned char WHITE = 0;
        int cols = image.ncols();
        int rows = image.nrows();

        //Find vertical run values
        // ***USE VECTOR ITERATORS WITH ROW ON THE OUTSIDE TO INCREASE PERFORMANCE IF THERE'S TIME***
        for (int c = 0; c < cols; c++) 
        {
            int run = 0;
            unsigned char val = WHITE;
            for (int r = 0; r < rows; r++) 
            {
                unsigned char pel = image.get(Point(c,r));
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

        //Find Vertical Distance
        for (int c = 0; c < cols; c++) 
        {
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

        //Find Graph Weights
        for (int r = 0; r < rows; r++) 
        {
            for (int c = 0; c < cols - 1; c++) 
            {
                graphWeight[(r * cols) + c].weight_hor = weightFunction(image, Point(c, r), Point(c + 1, r), NEIGHBOUR4);
                if (r > 0)
                    graphWeight[(r * cols) + c].weight_up = weightFunction(image, Point(c, r), Point(c + 1, r - 1), NEIGHBOUR8);
                else
                    graphWeight[(r * cols) + c].weight_up = TOP_VALUE;
                if (r < rows - 1)
                    graphWeight[(r * cols) + c].weight_down = weightFunction(image, Point(c, r), Point(c + 1, r + 1), NEIGHBOUR8);
                else
                    graphWeight[(r * cols) + c].weight_down = TOP_VALUE;
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

        int dist1 = verDistance[(pixelVal1.y() * image.nrows()) + pixelVal1.x()]; //Vertical Distance taken from array of values created in constructGraphWeights
        int dist2 = verDistance[(pixelVal2.y() * image.nrows()) + pixelVal2.x()];
        int vRun1 = verRun[(pixelVal1.y() * image.nrows()) + pixelVal1.x()]; //Vertical Runs taken from array of values created in constructGraphWeights
        int vRun2 = verRun[(pixelVal2.y() * image.nrows()) + pixelVal2.x()]; 

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
        if ( (pel) && (min(vRun1, vRun2) <= staffLineHeight))
        {
            --y;
        }
        if (max(dist1, dist2) > (2 * staffLineHeight) + staffSpaceDistance)
        {
            y++;
        }
        return y;
    }

    template<class T>
    stableStaffLineFinder(T &image)
    {
        graphPath = new NODE[image.nrows()*image.ncols()];
        graphWeight = new NODEGRAPH[image.nrows()*image.ncols()];
        verRun = new int[image.nrows()*image.ncols()];
        verDistance = new int[image.nrows()*image.ncols()];
        memset (verDistance, 0, sizeof(int)*image.nrows()*image.ncols());
    }

    double staffDissimilarity(vector<Point>& staff1, vector<Point>& staff2)
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
            double curr_dif = abs(staff1[i].y() - staff2[i].y() - currAvg1+currAvg2);
            avgDiff += (curr_dif*curr_dif);
        }
        avgDiff /= staff1.size();
        avgDiff = sqrt(avgDiff);
        return avgDiff;
    }

    //template<class T>
    int sumOfValuesInVector (vector<Point>& vec, OneBitImageView *image)
    {
        //size_t len = vec.size();
        int sumOfValues = 0;
        int startCol = 0;
        int endCol = image->ncols()-1;
        for (int i = startCol; i <= endCol; i++)
        {
            int col = vec[i].x();
            int row = vec[i].y();
            unsigned char pel = image->get(getPointView((row*image->ncols() + col), image->ncols(), image->nrows()));
            sumOfValues += pel;
        }
        return sumOfValues;
    }

    //template<class T>
    bool tooMuchWhite (vector<Point> &vec, OneBitImageView *image, double minBlackPerc)
    {
        int sumOfValues = sumOfValuesInVector(vec, image);
        //size_t len = vec.size();
        int startCol = 0;
        int endCol = image->ncols()-1;
        int usedSize = endCol-startCol+1;
        if (sumOfValues < 1*(1-minBlackPerc)*(usedSize))
        {
            return true;
        }
        return false;
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
                    weight10 = graphWeight[row*width + width-1-col].weight_up;
                    value1 = weight10 + graphPath[(row-1)*width + (col-1)].weight;
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
                    p = graphPath[p.y()*width + p.x()].previous;
                    contour[pos] = p;
                    pos--;
                }
                stablePaths.push_back(contour);
            }
        }
        return 0;
    }

    //TODO: increase speed with vector iterators
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
                        break;
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
                        break;
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

    //=============================================================================
    template <class T> //HELPER FUNCTION
    T findMostRepresentedValueOnSortedVector(vector<T>& vec)
    {
        T run = vec[0];
        int freq = 0;
        int maxFreq = 0;
        T maxRun = run;

        for (unsigned i = 0; i< vec.size(); i++)
        {
            if (vec[i] == run)
                freq ++;
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
    //=============================================================================

    struct STAT {
        int pixVal; //value of pixel (1 or 0)
        int runVal; //runValue
        int numOfOccurences; //Number of times the pixVal and runVal are identical in a graph
    }; 

    template <class T>
    void findStaffHeightandDistanceNoVectors(T &image) //Works only for staffLineHeight
    {

        STAT *graphStats = new STAT[image.nrows()*image.ncols()/2];
        int counter = 0;
        int found;
        for (int x = 0; x < image.ncols()*image.nrows(); x++)
        {
            found = 0;
            for (int y = 0; y < counter; y++)
            {
                if (verRun[x] == graphStats[y].runVal && image.get(getPoint(x, image)) == graphStats[y].pixVal)
                {
                    graphStats[y].numOfOccurences++;
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                graphStats[counter].runVal = verRun[x];
                graphStats[counter].pixVal = image.get(getPoint(x, image));
                graphStats[counter].numOfOccurences = 1;
                counter++;
            }
        }
        sort(graphStats, graphStats+counter, structCompare);
        staffLineHeight = 0;
        staffSpaceDistance = 0;
        for (int i = 0; i < counter; i++)
        {
            if (!staffLineHeight) //Has no assigned value yet
            {
                if (graphStats[i].pixVal) //pixel is black
                    staffLineHeight = graphStats[i].runVal; //Assign value to StaffLineHeight
            }
            if (!staffSpaceDistance) //Has no assigned value yet
            {
                if (!graphStats[i].pixVal) //pixel is white
                    staffSpaceDistance = graphStats[i].runVal; //Assign value to StaffSpaceDistance
            }
        }
    }

    // template <class T>
    // void findStaffHeightandDistanceNoVectors2(T &image)
    // {
    //  STAT *graphStats = new STAT[image.ncols()][image.nrows()/2];
    //  int counter = 0;
    //  int found;

    //  for (int c = 0; x < image.ncols(); c++)
    //  {

    //  }
    // }

    //Used in sort() to sort items from greatest number of occurences to lowest number of occurences
    static bool structCompare(STAT a, STAT b)
    {
        return a.numOfOccurences > b.numOfOccurences;
    }

    //Used for testing
    int fillValues() 
    {
        int x;
        for (x = 0; x < staffLineHeight; x++) 
        {
            verRun[x] = 1;
        }
        return x;
    }

    template<class T>
    void stableStaffDetection(vector <vector <Point> > &validStaves, T &image)
    {
        constructGraphWeights(image);
        OneBitImageView *imageCopy = myCloneImage(image);
        OneBitImageView *imgErode = myCloneImage(image);
        myVerticalErodeImage(imgErode, image.ncols(), image.nrows());
        // myIplImage* imgErode  = myCloneImage(img);
        // myVerticalErodeImage (imgErode);
        // myIplImage* imgErodedCopy  = myCloneImage (imgErode);

        vector<int> npaths;

        bool first_time = 1;
        double blackPerc;
        vector<Point> bestStaff;

        while(1)
        {
            vector <vector<Point> > stablePaths;
            int curr_n_paths = 0;
            printf("About to findAllStablePaths\n");
            findAllStablePaths(image, 0, image.ncols()-1, stablePaths);
            printf("Finished findAllStablePaths. Size = %lu\n", stablePaths.size());
            if (first_time && stablePaths.size() > 0)
            {
                first_time = 0;
                bestStaff.clear();
                size_t bestSumOfValues = INT_MAX;
                size_t bestStaffIdx = 0;
                vector<size_t> allSumOfValues;

                for (size_t c = 0; c < stablePaths.size(); c++)
                {
                    size_t sumOfValues = sumOfValuesInVector(stablePaths[c], imageCopy);
                    if (sumOfValues/(1.0*(stablePaths[c].size())) < MIN_BLACK_PER) //Checks to make sure percentage of black values are larger than the minimum black percentage allowed
                        allSumOfValues.push_back(sumOfValues);
                    if (sumOfValues < bestSumOfValues)
                    {
                        bestSumOfValues = sumOfValues;
                        bestStaffIdx = c;
                    }
                }

                vector<size_t> copy_allSumOfValues = allSumOfValues;
                sort(allSumOfValues.begin(), allSumOfValues.end());
                size_t medianSumOfValues = allSumOfValues[allSumOfValues.size()/2];
                int i;
                for (i = 0; i < copy_allSumOfValues.size(); i++)
                    if (copy_allSumOfValues[i] == medianSumOfValues)
                        break;
                bestStaff = stablePaths[i];

                double bestBlackPerc = medianSumOfValues/(1.0*bestStaff.size());
                blackPerc = max(MIN_BLACK_PER, bestBlackPerc*0.8);
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
                if (dissimilarity > 4*staffSpaceDistance)
                {
                    printf ("\tToo Dissimilar\n");
                    continue;
                }

                validStaves.push_back(staff);
                curr_n_paths++;

                int path_half_width2 = max(staffLineHeight, staffSpaceDistance/2);

                //Erasing paths from our image, must create a copy of our image
                // for (size_t i = 0; i<staff.size(); i++)
                // {
                //  int col = staff[i].x;
                //  int row = staff[i].y;

                //  // ERASE PATHS ALREADY SELECTED!
                //  for (int j = -path_half_width2-1; j <= path_half_width2+1; j++)
                //  {   
                //      if ( ((row+j)>img->height-1) || ((row+j)<0) )
                //          continue;
                //      img->imageData[(row+j)*img->widthStep + col] = 255;
                //      imgErode->imageData[(row+j)*imgErode->widthStep + col] = 255;
                //      if ( ((row+j)>img->height-1) || ((row+j)<0) )
                //          continue;
                //      if (col == img->width-1)
                //          continue;
                //      if (row+j > 0)
                //          graphWeight[(row+j)*img->width + col].weight_up = 12;
                //      else
                //          graphWeight[(row+j)*img->width + col].weight_up = TOP_VALUE;
                //      graphWeight[(row+j)*img->width + col].weight_hor = 8;
                //      if (row+j < img->height-1)
                //          graphWeight[(row+j)*img->width + col].weight_down = 12;
                //      else
                //          graphWeight[(row+j)*img->width + col].weight_down = TOP_VALUE;
                //  } //
                // }

            }
            npaths.push_back(curr_n_paths);
            if (curr_n_paths == 0)
                break;
        }

        //postProcessing(validStaves, staffSpaceDistance, imageErodedCopy, image)
    }
};

template<class T>
float returnGraphWeights(T &image) 
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    slf1.constructGraphWeights(image);
    slf1.findStaffHeightandDistanceNoVectors(image);
    return slf1.staffLineHeight;
}

template<class T>
OneBitImageView* copyImage(T &image)
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    OneBitImageView *new1 = slf1.myCloneImage(image);
    printf("Rows: %lu, Columns: %lu \n", image.nrows(), image.ncols());
    //slf1.myVerticalErodeImage(new1, image.ncols(), image.nrows());
    slf1.constructGraphWeights(image);
    printf("findAllStablePaths: %d\n", slf1.findAllStablePaths(image, 0, image.ncols()-1, validStaves));
    slf1.deletePaths(validStaves, new1);

    //slf1.stableStaffDetection(validStaves, image);
    return new1;
}

template<class T>
OneBitImageView* findStablePaths(T &image) //Returns blank image with stable paths drawn
{
    vector <vector<Point> > validStaves;
    stableStaffLineFinder slf1 (image);
    OneBitImageView *blank = slf1.clear(image);
    printf("Rows: %lu, Columns: %lu \n", image.nrows(), image.ncols());
    slf1.constructGraphWeights(image);
    printf("findAllStablePaths: %d\n", slf1.findAllStablePaths(image, 0, image.ncols()-1, validStaves));
    slf1.drawPaths(validStaves, blank);
    return blank;
}