
#ifndef ypo21082008_staff_removal
#define ypo21082008_staff_removal

#include "gamera.hpp"
#include "plugins/image_utilities.hpp"
#include "plugins/projections.hpp"

#include "math.h"
#include <vector>
#include <numeric>
#include <algorithm>

#include <iostream>

using namespace Gamera;
using namespace std;

// staffspace estimation
#define WHITE 0
#define BLACK 1


// ================ Directional Median Filter =======================
/* this function returns the intermediate pixel value within the region
 * it only works on binary image
 */
template<class T>
OneBitPixel image_med_bw(const T &src)
{
    OneBitPixel med=0;
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            med += src.get(Point(n, m));
        }
    }

    return (unsigned int)((unsigned int)(2*med)>(src.nrows()*src.ncols())) ? 1 : 0;
}


// main function of directional median filter
/* it only works on binary image
 * The implementation of region size is not entirely correct because of
   integer rounding but matches the implementation of the thresholding
   algorithms.
 */
template<class T>
typename ImageFactory<T>::view_type* directional_med_filter_bw(const T &src, size_t region_width, size_t region_height)
{
    if ((min(region_width, region_height) < 1) || (max(region_width, region_height) > std::min(src.nrows(), src.ncols())))
        throw std::out_of_range("median_filter: region_size out of range");

    size_t half_region_width = region_width / 2;
    size_t half_region_height = region_height / 2;

    typename ImageFactory<T>::view_type* copy = ImageFactory<T>::new_view(src);
    typename ImageFactory<T>::data_type* data = new typename ImageFactory<T>::data_type(src.size(), src.origin());
    typename ImageFactory<T>::view_type* view = new typename ImageFactory<T>::view_type(*data);

    for (coord_t y = 0; y < src.nrows(); ++y) {
        for (coord_t x = 0; x < src.ncols(); ++x) {
            // Define the region.
            Point ul((coord_t)std::max(0, (int)x - (int)half_region_width),
                     (coord_t)std::max(0, (int)y - (int)half_region_height));
            Point lr((coord_t)std::min(x + half_region_width, src.ncols() - 1),
                     (coord_t)std::min(y + half_region_height, src.nrows() - 1));
            copy->rect_set(ul, lr);
            view->set(Point(x, y), image_med_bw(*copy));
        }
    }

    delete copy;
    return view;
}



// ========================== Staff Removal ====================
/* this function cheks if there's black pixel within the region
 */
template<class T>
bool is_close(const T &src)
{
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            if (src.get(Point(n, m))!=0)
                return true;
        }
    }
    return false;
}


/* this function estimates the staff image based on original image and a coarse staff removal result

    *neighbour_width*, *neighbour_height*
        region size defined as neighbourhood of a pixel.
 */
template<class T>
typename ImageFactory<T>::view_type* staff_recover(T &src, const T &removal_coarse, size_t region_width, size_t region_height)
{
    if ((min(region_width, region_height) < 1) || (max(region_width, region_height) > std::min(src.nrows(), src.ncols())))
        throw std::out_of_range("median_filter: region_size out of range");
    // extract coarse staff estimation
    typename ImageFactory<T>::view_type* staff=xor_image(src, removal_coarse, false);

    typename ImageFactory<T>::view_type* local_region=ImageFactory<T>::new_view(removal_coarse);
    size_t half_region_width = region_width / 2;
    size_t half_region_height = region_height / 2;
    // check each staff pixel if it is close enough to non-staff pixel. If so, label it as non-staff pixel
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            if (staff->get(Point(n, m))!=0) {
                Point ul((coord_t)std::max(0, (int)n - (int)half_region_width),
                        (coord_t)std::max(0, (int)m - (int)half_region_height));
                Point lr((coord_t)std::min(n + half_region_width, src.ncols() - 1),
                        (coord_t)std::min(m + half_region_height, src.nrows() - 1));
                local_region->rect_set(ul, lr);
                if (is_close(*local_region))
                    staff->set(Point(n, m), 0);
            }
        }
    }

    delete local_region;

    return staff;
}


// ============================= Staffspace/height Estimation =======================
/* this function estimates staffspace and staffheight based on vertical run-length coding with local projection
 * "win" defines the width of each vertical strip. Local projection is done within each strip.
   "staffspace_threshold1" defines the minimum height of staffline height. If the estimation is underneath this threshold,
    chose the next peak whose staffspace is over "staffspace_threshold2"
 */
template<class T>
void staffspaceheight_estimation(const T &src, unsigned int win,
                            unsigned int staffspace_threshold1, unsigned int staffspace_threshold2,
                            unsigned int &staffspace, unsigned int &staffheight)
{
    vector<unsigned int> hist_black (src.nrows());  // histogram for black run-length, staffline height
    vector<unsigned int> hist_white (src.nrows());  // histogram for white run-length, staffspace height
    typename ImageFactory<T>::view_type* patch=ImageFactory<T>::new_view(src);
    int sign;
    unsigned int pos, height;

    for (unsigned int k=0; k<src.nrows(); k++) {
        hist_white[k]=0;
        hist_black[k]=0;
    }

    for (unsigned int n=0; n<src.ncols()-win; n++) {
        // local projection
        patch->rect_set(Point(n, 0), Point(n+win-1, src.nrows()-1));
        IntVector* patch_proj=projection_rows(*patch);
        for (unsigned int k=0; k<src.nrows(); k++) {
            if (2*(*patch_proj)[k]>=int(win))
                (*patch_proj)[k]=1;
            else
                (*patch_proj)[k]=0;
        }
        // check whether the first run-length is white or black
        if ((*patch_proj)[0]==WHITE)
            sign=WHITE;
        else
            sign=BLACK;

        pos=0;
        for (unsigned int m=0; m<src.nrows(); m++) {
            // from a white segment to black segment
            if ((sign==WHITE)&&((*patch_proj)[m]==BLACK)) {
                height=m-pos;
                if (pos!=0) {
                    hist_white[height]++; }
                sign=BLACK;
                pos=m;
            }
            // from a black segment to white segment
            else if ((sign==BLACK)&&((*patch_proj)[m]==WHITE)) {
                height=m-pos;
                if (pos!=0)
                    hist_black[height]++;
                sign=WHITE;
                pos=m;
            }
        }
    }

    // find the peak of white run-length to estimate the staffspace height
    unsigned int pos_hist=0;
    unsigned int value=hist_white[0];
    for (unsigned int k=0; k<src.nrows(); k++) {
        if (hist_white[k]>value) {
            pos_hist=k;
            value=hist_white[k];
        }
    }
    // when the estimation is underneath staffspace_threshold1, chose the next peak whose staffspace is over staffspace_threshold2
    if (pos_hist<staffspace_threshold1) {
        hist_white[pos_hist]=0;
        for (unsigned int it=0; it<src.nrows(); it++) {
            value=hist_white[0];
            for (unsigned int l=0; l<src.nrows(); l++) {
                if (hist_white[l]>value) {
                    pos_hist=l;
                    value=hist_white[l];
                }
            }
            if (pos_hist<staffspace_threshold2)
                hist_white[pos_hist]=0;
        }
    }
    staffspace=pos_hist;

    // find the peak of black run-length to estimate the staffline height
    pos_hist=0;
    value=hist_black[0];
    for (unsigned int k=0; k<src.nrows(); k++) {
        if (hist_black[k]>value) {
            pos_hist=k;
            value=hist_black[k];
        }
    }
    staffheight=pos_hist;
    cout << "staffspace:" << staffspace << ", staffheight: " << staffheight << endl;
}


// main function of staff removal
/* this function removes staffline from music score.
 * it handles the case of curved staffline.
 *  Note: it is specially for lyric line detection and cannot be used directly for standard staff removal.

    *staffspace*
        staffspace height. If negative, estimated automatically.

    *staffheight*
        staff height. If negative, estimated automatically.

    *scalar_med_width_staffspace*, *scalar_med_height_staffspace*
        (scalar_med_width_staffspace*staffspace, scalar_med_height_staffspace*staffspace):
        region size for median filter estimated from staffspace.

    *scalar_med_width_staffheight*, *scalar_med_height_staffheight*
        (scalar_med_width_staffheight*staffheight, scalar_med_height_staffheight*staffheight):
        region size for median filter estimated from staffheight.

    *neighbour_width*, *neighbour_height*
        region size defined as neighbourhood of a pixel.
*/
template<class T>
OneBitImageView* staff_removal(const T &src, int staffspace0, int staffheight0,
                                                   double scalar_med_width_staffspace, double scalar_med_height_staffspace,
                                                   double scalar_med_width_staffheight, double scalar_med_height_staffheight,
                                                   size_t neighbour_width, size_t neighbour_height,
                                                   unsigned int staff_win, unsigned int staffspace_threshold1, unsigned int staffspace_threshold2)
{
    unsigned int staffspace, staffheight;
    // estimate staffspace and staffheight
    if ( (staffspace0 <= 0 ) || (staffheight0 <= 0) ) {
        cout << "staff estimation" << endl;
        staffspaceheight_estimation(src, staff_win,
                            staffspace_threshold1, staffspace_threshold2,
                            staffspace, staffheight);
    }
    else {
        staffspace = staffspace0;
        staffheight = staffheight0;
    }

    OneBitImageData* src_copy_data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* src_copy = new OneBitImageView(*src_copy_data);
    copy(src.vec_begin(),
                src.vec_end(),
                src_copy->vec_begin());

    size_t med_region_width, med_region_height;

    // create kernel of median filter
    if (staffheight > 1) {
        med_region_width=ceil(scalar_med_width_staffheight*staffheight);
        med_region_height=ceil(scalar_med_height_staffheight*staffheight);
    }
    else {
        med_region_width=ceil(scalar_med_width_staffspace*staffheight);
        med_region_height=ceil(scalar_med_height_staffspace*staffheight);
    }
    // coarse staff removal
    OneBitImageView* removal_coarse=directional_med_filter_bw(*src_copy, med_region_width, med_region_height);

    // staff recover
    OneBitImageView* staff=staff_recover(*src_copy, *removal_coarse, neighbour_width, neighbour_height);

    // determine non-staff part
    OneBitImageView* nostaff=xor_image(*src_copy, *staff, false);

    delete removal_coarse->data();
    delete removal_coarse;
    delete staff->data();
    delete staff;
    delete src_copy_data;
    delete src_copy;

    return nostaff;
}



// this function does the same work as staffspaceheight_estimation, but is purely for Python, as staffspaceheight_estimation cannot get complied when wrapped into Python
template<class T>
unsigned int staffspace_estimation(const T &src, unsigned int win,
                            unsigned int staffspace_threshold1, unsigned int staffspace_threshold2)
{
    vector<unsigned int> hist_black (src.nrows());  // histogram for black run-length, staffline height
    vector<unsigned int> hist_white (src.nrows());  // histogram for white run-length, staffspace height
    typename ImageFactory<T>::view_type* patch=ImageFactory<T>::new_view(src);
    int sign;
    unsigned int pos, height;

    unsigned int staffspace, staffheight;

    for (unsigned int k=0; k<src.nrows(); k++) {
        hist_white[k]=0;
        hist_black[k]=0;
    }

    for (unsigned int n=0; n<src.ncols()-win; n++) {
        // local projection
        patch->rect_set(Point(n, 0), Point(n+win-1, src.nrows()-1));
        IntVector* patch_proj=projection_rows(*patch);
        for (unsigned int k=0; k<src.nrows(); k++) {
            if (2*(*patch_proj)[k]>=int(win))
                (*patch_proj)[k]=1;
            else
                (*patch_proj)[k]=0;
        }
        // check whether the first run-length is white or black
        if ((*patch_proj)[0]==WHITE)
            sign=WHITE;
        else
            sign=BLACK;

        pos=0;
        for (unsigned int m=0; m<src.nrows(); m++) {
            // from a white segment to black segment
            if ((sign==WHITE)&&((*patch_proj)[m]==BLACK)) {
                height=m-pos;
                if (pos!=0) {
                    hist_white[height]++; }
                sign=BLACK;
                pos=m;
            }
            // from a black segment to white segment
            else if ((sign==BLACK)&&((*patch_proj)[m]==WHITE)) {
                height=m-pos;
                if (pos!=0)
                    hist_black[height]++;
                sign=WHITE;
                pos=m;
            }
        }
    }

    // find the peak of white run-length to estimate the staffspace height
    unsigned int pos_hist=0;
    unsigned int value=hist_white[0];
    for (unsigned int k=0; k<src.nrows(); k++) {
        if (hist_white[k]>value) {
            pos_hist=k;
            value=hist_white[k];
        }
    }
    // when the estimation is underneath staffspace_threshold1, chose the next peak whose staffspace is over staffspace_threshold2
    if (pos_hist<staffspace_threshold1) {
        hist_white[pos_hist]=0;
        for (unsigned int it=0; it<src.nrows(); it++) {
            value=hist_white[0];
            for (unsigned int l=0; l<src.nrows(); l++) {
                if (hist_white[l]>value) {
                    pos_hist=l;
                    value=hist_white[l];
                }
            }
            if (pos_hist<staffspace_threshold2)
                hist_white[pos_hist]=0;
        }
    }
    staffspace=pos_hist;

    // find the peak of black run-length to estimate the staffline height
    pos_hist=0;
    value=hist_black[0];
    for (unsigned int k=0; k<src.nrows(); k++) {
        if (hist_black[k]>value) {
            pos_hist=k;
            value=hist_black[k];
        }
    }
    staffheight=pos_hist;
    cout<<"staffspace:"<<staffspace<<", staffheight:"<<staffheight<<'\n';

    return staffspace;
}


// this function does the same work as staffspaceheight_estimation, but is purely for Python, as staffspaceheight_estimation cannot get complied when wrapped into Python
template<class T>
unsigned int staffheight_estimation(const T &src, unsigned int win,
                            unsigned int staffspace_threshold1, unsigned int staffspace_threshold2)
{
    vector<unsigned int> hist_black (src.nrows());  // histogram for black run-length, staffline height
    vector<unsigned int> hist_white (src.nrows());  // histogram for white run-length, staffspace height
    typename ImageFactory<T>::view_type* patch=ImageFactory<T>::new_view(src);
    int sign;
    unsigned int pos, height;

    unsigned int staffspace, staffheight;

    for (unsigned int k=0; k<src.nrows(); k++) {
        hist_white[k]=0;
        hist_black[k]=0;
    }

    for (unsigned int n=0; n<src.ncols()-win; n++) {
        // local projection
        patch->rect_set(Point(n, 0), Point(n+win-1, src.nrows()-1));
        IntVector* patch_proj=projection_rows(*patch);
        for (unsigned int k=0; k<src.nrows(); k++) {
            if (2*(*patch_proj)[k]>=int(win))
                (*patch_proj)[k]=1;
            else
                (*patch_proj)[k]=0;
        }
        // check whether the first run-length is white or black
        if ((*patch_proj)[0]==WHITE)
            sign=WHITE;
        else
            sign=BLACK;

        pos=0;
        for (unsigned int m=0; m<src.nrows(); m++) {
            // from a white segment to black segment
            if ((sign==WHITE)&&((*patch_proj)[m]==BLACK)) {
                height=m-pos;
                if (pos!=0) {
                    hist_white[height]++; }
                sign=BLACK;
                pos=m;
            }
            // from a black segment to white segment
            else if ((sign==BLACK)&&((*patch_proj)[m]==WHITE)) {
                height=m-pos;
                if (pos!=0)
                    hist_black[height]++;
                sign=WHITE;
                pos=m;
            }
        }
    }

    // find the peak of white run-length to estimate the staffspace height
    unsigned int pos_hist=0;
    unsigned int value=hist_white[0];
    for (unsigned int k=0; k<src.nrows(); k++) {
        if (hist_white[k]>value) {
            pos_hist=k;
            value=hist_white[k];
        }
    }
    // when the estimation is underneath staffspace_threshold1, chose the next peak whose staffspace is over staffspace_threshold2
    if (pos_hist<staffspace_threshold1) {
        hist_white[pos_hist]=0;
        for (unsigned int it=0; it<src.nrows(); it++) {
            value=hist_white[0];
            for (unsigned int l=0; l<src.nrows(); l++) {
                if (hist_white[l]>value) {
                    pos_hist=l;
                    value=hist_white[l];
                }
            }
            if (pos_hist<staffspace_threshold2)
                hist_white[pos_hist]=0;
        }
    }
    staffspace=pos_hist;

    // find the peak of black run-length to estimate the staffline height
    pos_hist=0;
    value=hist_black[0];
    for (unsigned int k=0; k<src.nrows(); k++) {
        if (hist_black[k]>value) {
            pos_hist=k;
            value=hist_black[k];
        }
    }
    staffheight=pos_hist;
    cout<<"staffspace:"<<staffspace<<", staffheight:"<<staffheight<<'\n';

    return staffheight;
}

#endif

