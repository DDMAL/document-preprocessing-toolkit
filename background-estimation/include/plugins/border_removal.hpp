
//#ifndef jab18112005_binarization
//#define jab18112005_binarization

#include "gamera.hpp"
#include "plugins/image_utilities.hpp"
#include "plugins/morphology.hpp"
#include "plugins/binarization.hpp"
#include "plugins/image_conversion.hpp"
#include "plugins/edgedetect.hpp"
#include "plugins/segmentation.hpp"
#include "plugins/draw.hpp"
#include "plugins/thinning.hpp"
#include "connected_components.hpp"

#include "math.h"
#include <vector>
#include <queue>
#include <numeric>
#include <algorithm>

#include <iostream>



// ====== flood fill ======
#define MINUS 1
#define PLUS -1
// ====== paper estimation ======
#define SMOOTH 1
#define DETAIL 0
// ====== border removal =====
#define AREA_STANDARD 114000


using namespace Gamera;
using namespace std;


// ========== Median Filter ===============

/* this function returns the intermediate pixel value within the region
 */
template<class T>
FloatPixel image_med(const T &src)
{
    int med;
    vector<FloatPixel> patch;
    patch.resize(src.ncols()*src.nrows());
    copy(src.vec_begin(),
              src.vec_end(),
              patch.begin());
    med=ceil(src.ncols()*src.nrows()/2)-1;
    nth_element(patch.begin(),patch.begin()+med,patch.end());
    return *(patch.begin()+med);
}


// main function of median filter
/* The implementation of region size is not entirely correct because of
   integer rounding but matches the implementation of the thresholding
   algorithms.
 */
template<class T>
FloatImageView* med_filter(const T &src, size_t region_size)
{
    if ((region_size < 1) || (region_size > std::min(src.nrows(), src.ncols())))
        throw std::out_of_range("median_filter: region_size out of range");

    size_t half_region_size = region_size / 2;

    typename ImageFactory<T>::view_type* copy = ImageFactory<T>::new_view(src);
    FloatImageData* data = new FloatImageData(src.size(), src.origin());
    FloatImageView* view = new FloatImageView(*data);

    for (coord_t y = 0; y < src.nrows(); ++y) {
        for (coord_t x = 0; x < src.ncols(); ++x) {
            // Define the region.
            Point ul((coord_t)std::max(0, (int)x - (int)half_region_size),
                     (coord_t)std::max(0, (int)y - (int)half_region_size));
            Point lr((coord_t)std::min(x + half_region_size, src.ncols() - 1),
                     (coord_t)std::min(y + half_region_size, src.nrows() - 1));
            copy->rect_set(ul, lr);
            view->set(Point(x, y), image_med(*copy));
        }
    }

    delete copy;
    return view;
}



// =============== Flood Fill ================

/* this function only implements 4-connectivity. 8-connectivity can be added by assigning new labels.
 * the neighbourhood labels: (x indicates the current pixel)
                 -1
              -2  x  2
                  1
 */
Point neighbour(Point current, int pos)
{
    switch (pos) {
        case -1:
          return Point(current.x(), current.y()-1);
        case -2:
          return Point(current.x()-1, current.y());
        case 1:
          return Point(current.x(), current.y()+1);
        case 2:
          return Point(current.x()+1, current.y());
        default:
          return current;
    }
}


/* this function returns the nth neighbour position of current pixel along raster scanning or anti-raster scanning
 * "sequence" indicates the kind of scannig, PLUS as raster and MINUS as anti-raster
 * when "nth" is invalid, the current pixel is returned
 *
 * the nth-neighbour labels: (x indicates the current pixel)
   raster:                 anti-raster:
                 1                        x  2
              2  x                        1
 *
 * this function only implements 4-connectivity. 8-connectivity can be added by assigning new labels.
 */
template<class T>
Point neighbour_nth(const T &src, Point current, int nth, int sequence)
{
    int nth_max=2;      // the maximum number of neighbours in scanning
    int shift=0;        // label shifting when the current pixel is on the image border

    // raster scanning
    if (sequence==PLUS) {
        if (current.y()==0) { // first row
            nth_max--;
            shift++;
        }
        if (current.x()==0)   // first column
            nth_max--;
    }
    // anti-raster scanning
    else {
        if (current.y()==src.nrows()-1) {   // last row
            nth_max--;
            shift++;
            }
        if (current.x()==src.ncols()-1) {   // last column
            nth_max--;
        }
    }
    if (nth>nth_max||nth==0)   // when "nth" is invalid, return the current position
        return current;
    else {
        return neighbour(current, sequence*(nth+shift));
        }
}


/* this function creates mask image for filling holes approach, and it only works for greyscale image
 */
template<class T>
typename ImageFactory<T>::view_type* flood_mask_grey(const T &src)
{
        typename ImageFactory<T>::view_type* mask_view = pad_image(src, 1, 1, 1, 1, 0);
        invert(*mask_view);
        return mask_view;
}


/* this function creates marker image for filling holes approach, and it only works for greyscale image
 */
template<class T>
typename ImageFactory<T>::view_type* flood_marker_grey(const T &src)
{
    typename ImageFactory<T>::data_type* data = new typename ImageFactory<T>::data_type(src.size(), src.origin());
    typename ImageFactory<T>::view_type* view = new typename ImageFactory<T>::view_type(*data);
        invert(*view);
        typename ImageFactory<T>::view_type* marker_view = pad_image(*view, 1, 1, 1, 1, 255);
        delete data;
        delete view;
        return marker_view;
}


/* this function creates mask image for flood fill approach, and it only works for binary image
 */
template<class T>
typename ImageFactory<T>::view_type* flood_mask_bw(const T &src)
{
    typename ImageFactory<T>::view_type* mask_view = pad_image(src, 1, 1, 1, 1, 0);
    invert(*mask_view);
    return mask_view;
}


/* this function creates marker image for flood fill approach, and it only works for binary image
 */
template<class T>
typename ImageFactory<T>::view_type* flood_marker_bw(const T &src)
{
    typename ImageFactory<T>::data_type* data = new typename ImageFactory<T>::data_type(src.size(), src.origin());
    typename ImageFactory<T>::view_type* view = new typename ImageFactory<T>::view_type(*data);
    typename ImageFactory<T>::view_type* marker_view = pad_image(*view, 1, 1, 1, 1, 1);
    return marker_view;
}


// greyscale reconstruction, the core function of both flood fill and filling holes.
/*  Algorithm reference: Luc Vincent, "Morphological Grayscale Reconstruction
    In Image Analysis: Applications and Efficient Algorithms", IEEE Transactions
    on Image Processing, vol.2, no.2, April 1993, pp. 176-201
 * this function works on both 4-connectivity and 8-connectivity, but "neighbour_nth" and "neighbour" functions should be revised in case of 8-connectivity.
 */
template<class T>
void flood_fill_core(const T &mask, T &marker, int conn=4)
{
    Point p, q, p2;
    typename T::value_type max_pixel, value;
    queue<Point> queue_fifo;

    // raster scanning
    for (p=Point(0,0); p.y() < marker.nrows(); p.move(0,1)) {
        for (p.x(0); p.x() < marker.ncols(); p.move(1,0)) {
            max_pixel=marker.get(p);
            for (int walker = 1; walker <= (conn / 2); walker++) {
                q=neighbour_nth(marker, p, walker, PLUS);
                if (marker.get(q) > max_pixel)
                    max_pixel=marker.get(q);
            }
            value=(max_pixel < (mask.get(p))) ? max_pixel : mask.get(p);
            marker.set(p,value);
        }
    }

    // anti-raster scanning - AH refactor
    p = Point(marker.ncols() - 1, marker.nrows() - 1);
    while ( p.y() )
    {
        while ( p.x() )
        {
            max_pixel = marker.get(p);
            for ( int walker = 1; walker <= (conn / 2); walker++)
            {
                q = neighbour_nth(marker, p, walker, MINUS);
                if ( marker.get(q) > max_pixel )
                {
                    max_pixel = marker.get(q);
                }
            }
            value = (max_pixel < (mask.get(p))) ? max_pixel : mask.get(p);
            marker.set(p, value);
            for ( int walker = 1; walker <= (conn / 2); walker++ )
            {
                q = neighbour_nth(marker, p, walker, MINUS);
                if (q == p)
                {
                    continue;
                }
                if ( (marker.get(q) < marker.get(p)) && (marker.get(q) < mask.get(q)) ) {
                    queue_fifo.push(p);
                    break;
                }
            }
            p.move(-1, 0);
        }
        p.move(0, -1);
    }

    // for (p = Point(marker.ncols() - 1, marker.nrows() - 1); p.y() >= 0; p.move(0, -1)) {
    //     for (p.x(marker.ncols() - 1); p.x() >= 0; p.move(-1, 0)) {
    //         max_pixel = marker.get(p);
    //         for (int walker=1; walker <= conn / 2; walker++) {
    //             q=neighbour_nth(marker, p, walker, MINUS);
    //             if (marker.get(q) > max_pixel)
    //                 max_pixel = marker.get(q);
    //         }
    //         value=(max_pixel<(mask.get(p))) ? max_pixel : mask.get(p);
    //         marker.set(p,value);
    //         // push to queue
    //         for (int walker = 1; walker <= conn / 2; walker++) {
    //             q=neighbour_nth(marker, p, walker, MINUS);
    //             if (q == p)
    //                 continue;
    //             if ((marker.get(q) < marker.get(p)) && (marker.get(q) < mask.get(q))) {
    //                 queue_fifo.push(p);
    //                 break;
    //             }
    //         }
    //     if (p.x()==0)
    //         break;
    //     }
    // if (p.y()==0)
    //     break;
    // }

    // propagation
    while ( !queue_fifo.empty() ) {
        p = queue_fifo.front();
        queue_fifo.pop();
        // traverse all neighbours by raster and anti-raster scanning
        for ( int walker=1; walker<=conn/2; walker++ ) {
            q = neighbour_nth(marker, p, walker, PLUS);
            if (q == p)
                continue;
            if ( (marker.get(q)<marker.get(p))&&(marker.get(q)!=mask.get(q)) ) {
                value = (marker.get(p)<mask.get(q)) ? marker.get(p) : mask.get(q);
                marker.set(q,value);
                queue_fifo.push(q);
            }
        }
        for ( int walker = 1; walker <= conn / 2; walker++ ) {
            q = neighbour_nth(marker, p, walker, MINUS);
            if (q == p)
                continue;
            if ( (marker.get(q)<marker.get(p))&&(marker.get(q)!=mask.get(q)) ) {
                value = (marker.get(p) < mask.get(q)) ? marker.get(p) : mask.get(q);
                marker.set(q, value);
                queue_fifo.push(q);
            }
        }
    }
}


// main function for filling holes
/* it only works on greyscale images.
 * this function works on both 4-connectivity and 8-connectivity, but "neighbour_nth" and "neighbour" functions should be revised in case of 8-connectivity.
 */
template<class T>
typename ImageFactory<T>::view_type* flood_fill_holes_grey(const T &src, int conn=4)
{
    // create mask and marker images
    typename ImageFactory<T>::view_type* mask = flood_mask_grey(src);
    typename ImageFactory<T>::view_type* marker = flood_marker_grey(src);
    // flood fill
    flood_fill_core(*mask, *marker, conn);
    invert(*marker);
    // extract the central region because of the padding process during mask and marker process
    marker->rect_set(Point(1, 1), Size((marker->size()).width()-2, (marker->size()).height()-2));
    typename ImageFactory<T>::view_type* result=simple_image_copy(*marker);
    result->move(-1, -1);
    (result->data())->page_offset_x(0);
    (result->data())->page_offset_y(0);

    delete mask->data();
    delete mask;
    delete marker->data();
    delete marker;
    return result;
}


// main function for flood fill
/* it only works on binary images.
 * this function works on both 4-connectivity and 8-connectivity, but "neighbour_nth" and "neighbour" functions should be revised in case of 8-connectivity.
 */
template<class T>
typename ImageFactory<T>::view_type* flood_fill_bw(const T &src, int conn=4)
{
    // create mask and marker images
    typename ImageFactory<T>::view_type* mask = flood_mask_bw(src);
    typename ImageFactory<T>::view_type* marker = flood_marker_bw(src);
    // flood fill
    flood_fill_core(*mask, *marker, conn);
    invert(*mask);
    // extract the central region because of the padding process during mask and marker process
    typename ImageFactory<T>::view_type* marker2=or_image(*mask, *marker, false);
    marker2->rect_set(Point(1, 1), Size((marker2->size()).width()-2, (marker2->size()).height()-2));
    typename ImageFactory<T>::view_type* result=simple_image_copy(*marker2);
    result->move(-1, -1);
    (result->data())->page_offset_x(0);
    (result->data())->page_offset_y(0);

    delete mask->data();
    delete mask;
    delete marker->data();
    delete marker;
    delete marker2->data();
    delete marker2;
    return result;
}



// =============== Paper Estimation ===================

/* this function returns the estimation of background paper by removing foreground pen strokes.
    *sign*
        An extra smoothing process(dilation+erosion) is applied at the begining, when sign=1.
        An extra edge-preserving process(filling holes) is applied at the end, when sign=0.

    *dil_win*
        region size for dilation+erosion.

    *avg_win*
        region size for mean filter.

    *med_win*
        region size for median filter.
 */
template<class T>
GreyScaleImageView* paper_estimation(const T &src, int sign, int win_dil, int win_avg, int win_med)
{
    GreyScaleImageData* copy_data = new GreyScaleImageData(src.size(), src.origin());
    GreyScaleImageView* copy_view = new GreyScaleImageView(*copy_data);
    copy(src.vec_begin(),
              src.vec_end(),
              copy_view->vec_begin());

    // dilation & erosion
    GreyScaleImageView* image_morph1;
    GreyScaleImageView* image_morph2;
    if (sign==SMOOTH) {
        image_morph1=erode_dilate(*copy_view, win_dil, 1, 0);
        delete copy_data;
        delete copy_view;
        image_morph2=erode_dilate(*image_morph1, win_dil, 0, 0);
        delete image_morph1->data();
        delete image_morph1;
        }
    else
        image_morph2=copy_view;

    // blurring
        // filling holes
    GreyScaleImageView* image_fill=flood_fill_holes_grey(*image_morph2);
    delete image_morph2->data();
    delete image_morph2;
        // mean (average) filter
    FloatImageView* image_avg=mean_filter(*image_fill, win_avg);
    delete image_fill->data();
    delete image_fill;
        // median filter
    FloatImageView* image_med=med_filter(*image_avg, win_med);
    delete image_avg->data();
    delete image_avg;

    if (sign!=DETAIL) {
        GreyScaleImageView* paper=to_greyscale(*image_med);
        delete image_med->data();
        delete image_med;
        return paper;
    }
    // final flood fill
    else {
        GreyScaleImageView* paper0=to_greyscale(*image_med);
        GreyScaleImageView* paper=flood_fill_holes_grey(*paper0);
        delete image_med->data();
        delete image_med;
        delete paper0->data();
        delete paper0;
        return paper;
    }

}



// ================= grey2logical =======================

/* this function converts greyscale image into binary image.
 * pixels with value equal or smaller than 1 are considered as black pixel. (As "canny_edge_image" function labels edges with value 1)
 */
template<class T>
struct to_logical_core : public std::unary_function<GreyScalePixel, T>
{
    OneBitPixel operator()(T x) { return (x<=1) ? 1 : 0; }
};


template<class T>
OneBitImageView* to_logical(const T &src)
{
    OneBitImageData* data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* view = new OneBitImageView(*data);
    transform(src.vec_begin(),
              src.vec_end(),
              view->vec_begin(),
              to_logical_core<typename T::value_type>());
    return view;
}



// ================= Edge Detection ======================

/* This function searchs the 2nd edge map for long edges and combines them into the 1st edge map.
 * the threshold for the minimum edge length in 2nd edge map is scale_length*maximum side of image.
 */
template<class T>
OneBitImageView* edge_combine(const T &src, const T &src2, double scale_length)
{
    typedef ConnectedComponent<OneBitImageData> CC;
    // define the threshold of minimum edge length
    double max_length=scale_length*((src.ncols()>src.nrows()) ? src.ncols() : src.nrows());

    OneBitImageView* edge_view = to_logical(src);
    typename ImageFactory<OneBitImageView>::view_type* edge_view_cc = ImageFactory<OneBitImageView>::new_view(*edge_view);
    OneBitImageView* edge2_view = to_logical(src2);
    typename ImageFactory<OneBitImageView>::view_type* edge2_view_cc = ImageFactory<OneBitImageView>::new_view(*edge2_view);

    ImageList* ccs2_list=cc_analysis(*edge2_view);
    ImageList::iterator i;
    unsigned int ccs_length;

    for (i = ccs2_list->begin(); i != ccs2_list->end(); i++) {
        ConnectedComponent<OneBitImageData>* cc_cur=static_cast<ConnectedComponent<OneBitImageData>* >(*i);
        // compute the maximum length of bounding box of current edge
        ccs_length=(cc_cur->ncols()>cc_cur->nrows()) ? cc_cur->ncols() : cc_cur->nrows();
        // transfer the edge to final edge map
        if (ccs_length > max_length) {
            edge_view_cc->rect_set(cc_cur->origin(), cc_cur->size());
            for (unsigned int m=0; m<cc_cur->nrows(); m++) {
                for (unsigned int n=0; n<cc_cur->ncols(); n++) {
                    if (cc_cur->get(Point(n,m))!=0) {
                        edge_view_cc->set(Point(n,m), 1);
                    }
                }
            }
        }
    }

    delete edge_view_cc;
    delete edge2_view->data();
    delete edge2_view;
    delete edge2_view_cc;
    return edge_view;
}


// the main function for edge detection
/* this function etects and combines edges from two images in different levels of smoothness.

    *src1*
        the image in higher level of smoothness.
        (the result of "paper_estimation" with sign=1)

    *src2*
        the image of same subject as src1, but in lower level of smoothness.
        (the result of "paper_estimation" with sign=0)

    *threshold1_scale*
        scale for canny edge detector on current image. See "canny_edge_image" function for details.

    *threshold1_gradient*
        gradient for canny edge detector on current image. See "canny_edge_image" function for details.

    *threshold2_scale*
        scale for canny edge detector on image2. See "canny_edge_image" function for details.

    *threshold2_gradient*
        gradient for canny edge detector on image2. See "canny_edge_image" function for details.

    *scale_length*
        edge tranfer parameter. See "edge_combine" for details
 */
template<class T>
OneBitImageView* edge_detection(const T &src1, const T &src2,
                                double threshold1_scale, double threshold1_gradient,
                                double threshold2_scale, double threshold2_gradient,
                                double scale_length)
{
     // canny edge detection
     GreyScaleImageView* edge1=canny_edge_image(src1, threshold1_scale, threshold1_gradient);
     GreyScaleImageView* edge2=canny_edge_image(src2, threshold2_scale, threshold2_gradient);

     // edge combination
     OneBitImageView* edge_final=edge_combine(*edge1, *edge2, scale_length);
     delete edge1->data();
     delete edge1;
     delete edge2->data();
     delete edge2;

     return edge_final;
}



// ====================== Boundary Reconstruction ================

/* this function sets the pixel on image border to 0
 */
template<class T>
void border_clear(T &src)
{
    for (unsigned int y=0; y<src.nrows(); y++) {
        src.set(Point(0, y), 0);
        src.set(Point(src.ncols()-1, y), 0);
    }
    for (unsigned int x=0; x<src.ncols(); x++) {
        src.set(Point(x, 0), 1);
        src.set(Point(0, src.nrows()-1), 0);
    }
}


/* this function fills the image with 0
 */
template<class T>
void region_clear(T &src)
{
    for (unsigned int y=0; y<src.nrows(); y++) {
        for (unsigned int x=0; x<src.ncols(); x++) {
            src.set(Point(x,y), 0);
        }
    }
}


/* this function checks if all the pixels are 0
 */
template<class T>
bool region_isclear(const T &src)
{
    for (unsigned int y=0; y<src.nrows(); y++) {
        for (unsigned int x=0; x<src.ncols(); x++) {
            if (src.get(Point(x,y))!=0)
                return false;
        }
    }
    return true;
}


/* this function computes the lyric_extraction of an image. However, it outputs the result after certain number of iterations, and the result may not be the ultimate lyric_extraction.
 * the code is adapted from Gamera c++ function "thin_hs" by Ichiro Fujinaga, Michael Droettboom and Karl MacMillan
 */
template<class T>
typename ImageFactory<T>::view_type* lyric_extraction(const T& in, int number_iteration) {
    int count=0;
    typedef typename ImageFactory<T>::data_type data_type;
    typedef typename ImageFactory<T>::view_type view_type;
    Dim new_size(in.ncols() + 2, in.nrows() + 2);
    bool upper_left_origin = (in.ul_x() == 0) || (in.ul_y() == 0);
    Point new_origin;
    if (upper_left_origin)
      new_origin = Point(0, 0);
    else
      new_origin = Point(in.ul_x() - 1, in.ul_y() - 1);
    data_type* thin_data = new data_type(new_size, new_origin);
    view_type* thin_view = new view_type(*thin_data);
    try {
      for (size_t y = 0; y != in.nrows(); ++y)
    for (size_t x = 0; x != in.ncols(); ++x)
      thin_view->set(Point(x + 1, y + 1), in.get(Point(x, y)));
      if (in.nrows() == 1 || in.ncols() == 1)
    goto end;
      data_type* H_M_data = new data_type(new_size, new_origin);
      view_type* H_M_view = new view_type(*H_M_data);
      try {
    bool not_finished = true;
    while (not_finished) {
      not_finished = thin_hs_one_pass(*thin_view, *H_M_view);
      count++;
      if (count==number_iteration)
          break; }
      } catch (std::exception e) {
    delete H_M_view;
    delete H_M_data;
    throw;
      }
      delete H_M_view;
      delete H_M_data;
    } catch (std::exception e) {
      delete thin_view;
      delete thin_data;
      throw;
    }
  end:
    if (upper_left_origin) {
      data_type* new_data = new data_type(in.size(), in.origin());
      view_type* new_view = new view_type(*new_data);
      for (size_t y = 0; y != in.nrows(); ++y)
    for (size_t x = 0; x != in.ncols(); ++x)
      new_view->set(Point(x, y), thin_view->get(Point(x + 1, y + 1)));
      delete thin_view;
      delete thin_data;
      return new_view;
    } else {
      delete thin_view;
      thin_view = new view_type(*thin_data, in);
      return thin_view;
    }
}


/* This function returns the foreground region of image. The edges keep
   growing and meet each other till a closed boundary of the forground is
   found.
 * NULL is returned when the appoach failed to reconstruct the boundary.
 * To check if there's a closed bounday, flood fill the edge map by function
   "flood_fill_bw". If there is one, the checking region should remain unflooeded,
   that is, zero.
 * It assumes that the foreground region covers the central part of the
   image, and hence two small region in the center of the image are used as
   samples to check if a closed
 */
template<class T>
OneBitImageView* edge_reconnect(const T &src, int terminate_time)
{
    int count=0;  // count the number of iterations
    bool sign;   // if there's an closed boundary, sign=1; otherwise, sign=0.
    OneBitImageData* data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* new_boundary = new OneBitImageView(*data);
    copy(src.vec_begin(),
              src.vec_end(),
              new_boundary->vec_begin());
    OneBitImageView* boundary_temp;

    // define check-box
    unsigned int block_width=src.ncols()/9;
    unsigned int block_height=src.nrows()/9;
    Point ul1(block_width*3, block_height*3);
    Point ul2(block_width*6, block_height*6);

    // first check
    OneBitImageView* check_box=new OneBitImageView(*data);
    check_box->rect_set(ul1, Size(block_width, block_height));
    region_clear(*check_box);
    check_box->rect_set(ul2, Size(block_width, block_height));
    region_clear(*check_box);
    OneBitImageView* flood_test=flood_fill_bw(*new_boundary);
    flood_test->rect_set(ul1, Size(block_width, block_height));
    sign=(region_isclear(*flood_test));
    flood_test->rect_set(ul2, Size(block_width, block_height));
    sign=sign&&(region_isclear(*flood_test));
    delete flood_test->data();
    delete flood_test;

    while (!sign) {
        // dilate edges
        boundary_temp=erode_dilate(*new_boundary, 1, 0, 0);
        delete new_boundary->data();
        delete new_boundary;
        delete check_box;
        if (count>terminate_time) {
            delete boundary_temp->data();
            delete boundary_temp;
            cout<<"unsuccess"<<'\n';
            return NULL;
        }
        new_boundary=simple_image_copy(*boundary_temp);
        delete boundary_temp->data();
        delete boundary_temp;
        // check by flood fill
        check_box=new OneBitImageView(*(new_boundary->data()));
        check_box->rect_set(ul1, Size(block_width, block_height));
        region_clear(*check_box);
        check_box->rect_set(ul2, Size(block_width, block_height));
        region_clear(*check_box);
        border_clear(*new_boundary);
        flood_test=flood_fill_bw(*new_boundary);
        flood_test->rect_set(ul1, Size(block_width, block_height));
        sign=(region_isclear(*flood_test));
        flood_test->rect_set(ul2, Size(block_width, block_height));
        sign=sign&&(region_isclear(*flood_test));
        delete flood_test->data();
        delete flood_test;
        count++;
    }
    boundary_temp=erode_dilate(*new_boundary, 1, 0, 0);
    OneBitImageView* boundary_temp2=erode_dilate(*boundary_temp, 1, 1, 0);
    delete new_boundary->data();
    delete new_boundary;
    delete check_box;
    new_boundary=simple_image_copy(*boundary_temp2);
    delete boundary_temp->data();
    delete boundary_temp;

    // lyric_extractionization
    OneBitImageView* skel_org=lyric_extraction(*boundary_temp2, count);
    delete boundary_temp2->data();
    delete boundary_temp2;
    invert(*skel_org);
    OneBitImageView* skel=erode_dilate(*skel_org, 1, 1, 0);
    delete skel_org->data();
    delete skel_org;

    // extract region of interest
    ImageList* ccs2_list;
    ccs2_list=cc_analysis(*skel);
    data = new OneBitImageData(src.size(), src.origin());
    new_boundary = new OneBitImageView(*data);
    unsigned int label1=skel->get(ul1);
    unsigned int label2=skel->get(ul2);
    for (unsigned int m=0; m<skel->nrows(); m++) {
        for (unsigned int n=0; n<skel->ncols(); n++) {
            if ((skel->get(Point(n, m))==label1)||(skel->get(Point(n, m))==label2)) {
                new_boundary->set(Point(n, m), 1);
            }
        }
    }
    delete skel->data();
    delete skel;
    boundary_temp=erode_dilate(*new_boundary, 3, 0, 0);
    boundary_temp2=erode_dilate(*boundary_temp, 3, 1, 0);
    delete new_boundary->data();
    delete new_boundary;
    new_boundary=simple_image_copy(*boundary_temp2);
    delete boundary_temp->data();
    delete boundary_temp;
    delete boundary_temp2->data();
    delete boundary_temp2;

    GreyScaleImageView* boundary_grey=to_greyscale(*new_boundary);
    invert(*boundary_grey);
    GreyScaleImageView* mask_grey=flood_fill_holes_grey(*boundary_grey);
    OneBitImageView* mask=to_logical(*mask_grey);
    invert(*mask);
    delete new_boundary->data();
    delete new_boundary;
    delete boundary_grey->data();
    delete boundary_grey;
    delete mask_grey->data();
    delete mask_grey;
    return mask;

}


/* This function adds edges to current edge map. Four new edges are placed along
   the four sides of the edge map, with some interval to the vertices of the map.
 * "interval" defines the distance between the vertex of new edge and the
   vertex of the edge map.
 * The length of each new edge is width(height) of the map - 2*interval
 */
template<class T>
OneBitImageView* add_edge(const T &src, unsigned int interval)
{
    OneBitImageData* data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* view = new OneBitImageView(*data);
    copy(src.vec_begin(),
              src.vec_end(),
              view->vec_begin());

    for (unsigned int y=interval; y<view->nrows()-interval; y++) {
        view->set(Point(0, y), 1);
        view->set(Point(view->ncols()-1, y), 1);
    }
    for (unsigned int x=interval; x<view->ncols()-interval; x++) {
        view->set(Point(x, 0), 1);
        view->set(Point(0, view->nrows()-1), 1);
    }
    return view;
}


// main function for boundary reconstruct
/* this function returns the foreground region of image.
    *terminate_time1*
        maximum numbers of iterations in 1st round.

    *terminate_time2*
        maximum numbers of iterations in 2nd round.

    *terminate_time3*
        maximum numbers of iterations in 3rd round.

    *interval2*
        interval for edge adding in 2nd round.

    *interval3*
        interval for edge adding in 3rd round.
 */
template<class T>
OneBitImageView* boundary_reconstruct(const T &src,
                                int terminate_time1, int terminate_time2, int terminate_time3,
                                unsigned int interval2, unsigned int interval3)
{
    // 1st round
    OneBitImageView* mask=edge_reconnect(src, terminate_time1);

    // 2nd round
    if (mask==NULL) {
        OneBitImageView* boundary=add_edge(src, interval2);
        mask=edge_reconnect(*boundary, terminate_time2);
        delete boundary->data();
        delete boundary;
    }

    // 3rd round
    if (mask==NULL) {
        OneBitImageView* boundary=add_edge(src, interval3);
        mask=edge_reconnect(*boundary, terminate_time3);
        delete boundary->data();
        delete boundary;
    }
    // when fail
    if (mask==NULL) {
        OneBitImageData* data = new OneBitImageData(src.size(), src.origin());
        mask = new OneBitImageView(*data);
    }

    return mask;
}


// ============================ Border Removal =============================
// main function for border removal
template<class T>
OneBitImageView* border_removal(const T &src,
                                int win_dil, int win_avg, int win_med,
                                double threshold1_scale, double threshold1_gradient,
                                double threshold2_scale, double threshold2_gradient,
                                double scale_length,
                                int terminate_time1, int terminate_time2, int terminate_time3,
                                unsigned int interval2, unsigned int interval3)
{
    // image resize
    double scalar=sqrt(double(AREA_STANDARD)/(src.nrows()*src.ncols()));
    GreyScaleImageView* src_scale=static_cast<GreyScaleImageView*>(scale(src, scalar, 1));

    // paper estimation
    GreyScaleImageView* blur1=paper_estimation(*src_scale, SMOOTH, win_dil, win_avg, win_med);
    GreyScaleImageView* blur2=paper_estimation(*src_scale, DETAIL, win_dil, win_avg, win_med);

    // edge detection
    OneBitImageView* boundary=edge_detection(*blur1, *blur2,
                                threshold1_scale, threshold1_gradient,
                                threshold2_scale, threshold2_gradient,
                                scale_length);

    // boundary reconstruct
    OneBitImageView* mask_scale=boundary_reconstruct(*boundary,
                                terminate_time1, terminate_time2, terminate_time3,
                                interval2, interval3);

    // image resize back
    OneBitImageView* mask_temp=static_cast<OneBitImageView*>(scale(*mask_scale, 1.0/scalar, 1));
    OneBitImageData* data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* mask = new OneBitImageView(*data);
    for (unsigned int m=0; m<mask->nrows(); m++) {
        for (unsigned int n=0; n<mask->ncols(); n++) {
            mask->set(Point(n, m), mask_temp->get(Point(n, m)));
        }
    }

    delete src_scale->data();
    delete src_scale;
    delete blur1->data();
    delete blur1;
    delete blur2->data();
    delete blur2;
    delete boundary->data();
    delete boundary;
    delete mask_scale->data();
    delete mask_scale;
    delete mask_temp->data();
    delete mask_temp;
    return mask;

}

//#endif

