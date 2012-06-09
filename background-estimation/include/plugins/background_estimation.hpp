
#ifndef ypo21082008_background_estimation
#define ypo21082008_background_estimation

#include "gamera.hpp"
#include "plugins/image_utilities.hpp"
#include "plugins/binarization.hpp"
#include "border_removal.hpp"

#include "math.h"
#include <vector>
#include <numeric>
#include <algorithm>

#include <iostream>

using namespace Gamera;
using namespace std;


// ===================== Wiener Filter =======================
/* this function implement the 2D adaptive noise-removal filter (directional).
 * the way to deal with the case when the local variance is smaller than the noise variance might be wrong. Further checking is needed.
 * The implementation of region size is not entirely correct because of
   integer rounding but matches the implementation of the thresholding
   algorithms.

    *region_width*, *region_height*
      The size of the region within which to calculate the intermediate pixel value.

  *noise_variancee*
    noise variance. If negative, estimated automatically.
 */
template<class T>
typename ImageFactory<T>::view_type* wiener2_filter(const T &src, size_t region_width, size_t region_height, double noise_variance0)
{
    if ((min(region_width, region_height) < 1) || (max(region_width, region_height) > std::min(src.nrows(), src.ncols())))
        throw std::out_of_range("wiener_filter: region_size out of range");

    size_t half_region_width = region_width / 2;
    size_t half_region_height = region_height / 2;

    typename ImageFactory<T>::view_type* patch = ImageFactory<T>::new_view(src);
    typename ImageFactory<T>::data_type* data = new typename ImageFactory<T>::data_type(src.size(), src.origin());
    typename ImageFactory<T>::view_type* view = new typename ImageFactory<T>::view_type(*data);

  vector<double> mean(src.nrows()*src.ncols()); // local mean vector
  vector<double> variance(src.nrows()*src.ncols()); // local variance vector
  double noise_variance=0;
  coord_t x,y;

    for (y = 0; y < src.nrows(); ++y) {
        for (x = 0; x < src.ncols(); ++x) {
            // Define the region.
            Point ul((coord_t)std::max(0, (int)x - (int)half_region_width),
                     (coord_t)std::max(0, (int)y - (int)half_region_height));
            Point lr((coord_t)std::min(x + half_region_width, src.ncols() - 1),
                     (coord_t)std::min(y + half_region_height, src.nrows() - 1));
            patch->rect_set(ul, lr);

      double mean_patch=0;
      double variance_patch=0;
      unsigned int m, n;
      // compute local mean
      for (m=0; m<patch->nrows(); m++) {
        for (n=0; n<patch->ncols(); n++) {
          mean_patch += patch->get(Point(n, m));
        }
      }
      mean_patch=mean_patch/(patch->nrows()*patch->ncols());
      // compute local variance
      for (m=0; m<patch->nrows(); m++) {
        for (n=0; n<patch->ncols(); n++) {
          variance_patch += (patch->get(Point(n, m))-mean_patch)*(patch->get(Point(n, m))-mean_patch);
        }
      }
      variance_patch=variance_patch/(patch->nrows()*patch->ncols()-1);

      mean[y*src.ncols()+x]=mean_patch;
      variance[y*src.ncols()+x]=variance_patch;
      noise_variance += variance_patch;
        }
    }
  if (noise_variance0>0)
    noise_variance=noise_variance0;
  // compute noise variance
  else
    noise_variance=noise_variance/(src.nrows()*src.ncols());

  // filtering
    for (y = 0; y < src.nrows(); ++y) {
        for (x = 0; x < src.ncols(); ++x) {
      double value=mean[y*src.ncols()+x]+max(0.0, variance[y*src.ncols()+x]-noise_variance)/variance[y*src.ncols()+x]*(src.get(Point(x, y))-mean[y*src.ncols()+x]);
      if (value>255)
        value=255;
      else if (value<0)
        value=0;
      view->set(Point(x, y), value);
    }
  }

    delete patch;
    return view;
}


// ==================== Background Estimation =======================
/* this function estimates background of image by median filter and flood fill.

  *med_size*
    the kernel for median filter.
 */
template<class T>
GreyScaleImageView* background_estimation(const T &src, size_t med_size)
{
  // median filter
  FloatImageView* image_med_float=med_filter(src, med_size);
  GreyScaleImageView* image_med=to_greyscale(*image_med_float);

  // filling holes
  GreyScaleImageView* image_med_pad=pad_image(*image_med, 1, 1, 1, 1, 0);
  GreyScaleImageView* image_fill_pad=flood_fill_holes_grey(*image_med_pad);
  image_fill_pad->rect_set(Point(1, 1), src.size());
  GreyScaleImageView* result=simple_image_copy(*image_fill_pad);
  result->move(-1, -1);
  (result->data())->page_offset_x(0);
  (result->data())->page_offset_y(0);

  delete image_med_float->data();
  delete image_med_float;
  delete image_med->data();
  delete image_med;
  delete image_med_pad->data();
  delete image_med_pad;
  delete image_fill_pad->data();
  delete image_fill_pad;

  return result;
}


// ======================= Histogram Equalization ====================
// --------------- Histogram with Mask -------------
/* this function colculates histogram of an image within the region indicated by mask image.
 */
template<class T, class U>
FloatVector* histogram_mask(const T& image, const U& mask) {
// The histogram is the size of all of the possible values of
// the pixel type.
    size_t l = std::numeric_limits<typename T::value_type>::max() + 1;
    FloatVector* values = new FloatVector(l);
  double size = 0;

    try {
      // set the list to 0
      std::fill(values->begin(), values->end(), 0);

      typename T::const_row_iterator row = image.row_begin();
      typename T::const_col_iterator col;
      ImageAccessor<typename T::value_type> acc;

    coord_t m=0;
    coord_t n=0;

      // create the histogram
      for (; row != image.row_end(); ++row) {
    n=0;
      for (col = row.begin(); col != row.end(); ++col) {
      if (mask.get(Point(n,m))==1) {
        size=size+1.0;
        (*values)[acc.get(col)]++;
      }
      n++;
    }
    m++;
  }

      // convert from absolute values to percentages
      for (size_t i = 0; i < l; i++) {
  (*values)[i] = (*values)[i] / size;
      }
    } catch (std::exception e) {
      delete values;
      throw;
    }

    return values;
}


// --------- Image equalise_histogram with Mask----------------

/* Equalise an image histogram to match a reference histogram.
 */
template<class T, class U>
class find_transform
     : public std::binary_function<T, T, U>
   {
     const std::vector<T> output_cumulative_histogram;
     const T pixel_count;
     const double input_length;

   public:

     find_transform(const std::vector<T> &output_cumulative_histogram,
                    unsigned int pixel_count,
                    unsigned int input_length)
       : output_cumulative_histogram(output_cumulative_histogram),
         pixel_count(pixel_count),
         input_length(input_length) {}
     U operator()(T in, T tol)
       {
       FloatVector differences(output_cumulative_histogram.size());
       std::transform(output_cumulative_histogram.begin(),
                      output_cumulative_histogram.end(),
                      differences.begin(),
                      std::bind1st(std::plus<T>(), (T)(tol - in)));
       std::replace_if(differences.begin(), differences.end(),
                       std::bind2nd(std::less<T>(), (T)0), // cf. MATLAB
                       pixel_count);
       return (U)((double)(std::min_element(differences.begin(),
                                            differences.end())
                           - differences.begin())
                  * (input_length /
                     (double)output_cumulative_histogram.size()));
       }
   };


template<class T, class U>
Image* equalise_histogram_mask(const T& image, const U& mask,
              const FloatVector *hist)
{
     typedef typename ImageFactory<T>::data_type data_type;
     typedef typename ImageFactory<T>::view_type view_type;
     typedef typename FloatVector::value_type Float;

     // Prepare the output image.

     data_type* output_data = new data_type(image);
     view_type* output_image = new view_type(*output_data);

     // Define histograms for input and (ideal) output.

     FloatVector *input_histogram = histogram_mask(image, mask);
     FloatVector output_histogram(*hist);

     // Normalise histograms to sum to total number of pixels in image.

     unsigned int pixel_count = image.nrows() * image.ncols();
     std::transform(input_histogram->begin(), input_histogram->end(),
                    input_histogram->begin(),
                    std::bind1st(std::multiplies<Float>(),
                                 ((Float)pixel_count/std::accumulate(input_histogram->begin(),
                input_histogram->end(),
                (Float)0))));
     std::transform(output_histogram.begin(), output_histogram.end(),
                    output_histogram.begin(),
                    std::bind1st(std::multiplies<Float>(),
                                 ((Float)pixel_count/std::accumulate(output_histogram.begin(),
                output_histogram.end(),
                (Float)0))));

     // Compute cumulative histograms.

     FloatVector input_cumulative_histogram(input_histogram->size());
     std::partial_sum(input_histogram->begin(), input_histogram->end(),
                      input_cumulative_histogram.begin());
     FloatVector output_cumulative_histogram(output_histogram.size());
     std::partial_sum(output_histogram.begin(), output_histogram.end(),
                      output_cumulative_histogram.begin());

     // Compute tolerances (half of histogram value or zero at
     // beginning and end).

     FloatVector tolerance(*input_histogram);
     tolerance.front() = 0;
     tolerance.back() = 0;
     std::transform(tolerance.begin(), tolerance.end(),
                    tolerance.begin(),
                    std::bind1st(std::multiplies<Float>(), (Float)0.5));

     // Create a transform vector.

     std::vector<typename T::value_type>
       value_transform(input_histogram->size());
     find_transform<Float, typename T::value_type>
       transform_function(output_cumulative_histogram,
                          pixel_count,
                          input_histogram->size());
     std::transform(input_cumulative_histogram.begin(),
                    input_cumulative_histogram.end(),
                    tolerance.begin(),
                    value_transform.begin(),
                    transform_function);

     // Use row and column iterators and replace pixels with
     // transform[pix_val].

     typename T::const_row_iterator input_row = image.row_begin();
     typename T::row_iterator output_row = output_image->row_begin();
     for ( ; input_row != image.row_end(); input_row++, output_row++) {
       typename T::const_row_iterator::iterator input_column = input_row.begin();
       typename T::row_iterator::iterator output_column = output_row.begin();
       for ( ; input_column != input_row.end();
             input_column++, output_column++)
         *output_column = value_transform[*input_column];
     }

     // Clean up and return.
     delete input_histogram;
     return output_image;
}


// ======================= Binarization ======================
// ------------ Fill Region within Mask ---------------
/* this function fills the masked region with specified color
 */
template<class T, class U>
typename ImageFactory<T>::view_type* mask_fill(const T &src, const U &mask, int color)
{
    typename ImageFactory<T>::data_type* data = new typename ImageFactory<T>::data_type(src.size(), src.origin());
    typename ImageFactory<T>::view_type* view = new typename ImageFactory<T>::view_type(*data);
  for (coord_t m=0; m<src.nrows(); m++) {
    for (coord_t n=0; n<src.ncols(); n++) {
      if (mask.get(Point(n, m))==0)
        view->set(Point(n, m), color);
      else
        view->set(Point(n, m), src.get(Point(n, m)));
    }
  }
  return view;
}


// ------------- Gatos with Mask ---------------------
/* only unmasked region is used to compute parameters for Gatos thresholding

 *background*
 Estimated background of the image.

 *binarization*
 A preliminary binarization of the image.

 *mask*
 Mask image that defines the process region
 */
template<class T, class U, class S>
OneBitImageView* gatos_threshold_mask(const T &src,
                                 const T &background,
                                 const U &binarization,
                                 const S &mask,
                                 double q,
                                 double p1,
                                 double p2)
{
    typename ImageFactory<U>::view_type* binarization1=mask_fill(binarization, mask, 0);
    typename ImageFactory<U>::view_type* binarization2=mask_fill(binarization, mask, 1);

    if (src.size() != background.size())
        throw std::invalid_argument("gatos_threshold: sizes must match");
    if (background.size() != binarization.size())
        throw std::invalid_argument("gatos_threshold: sizes must match");

    typedef std::pair<unsigned int, FloatPixel> gatos_pair;
    typedef typename T::value_type base_value_type;
    typedef typename U::value_type binarization_value_type;

    double delta_numerator
        = std::inner_product(src.vec_begin(),
                             src.vec_end(),
                             background.vec_begin(),
                             (double)0,
                             double_plus<base_value_type>(),
                             std::minus<base_value_type>());
    unsigned int delta_denominator
        = std::count_if(binarization1->vec_begin(),
                        binarization1->vec_end(),
                        is_black<binarization_value_type>);
    double delta = delta_numerator / (double)delta_denominator;

    gatos_pair b_sums
        = std::inner_product(binarization2->vec_begin(),
                             binarization2->vec_end(),
                             background.vec_begin(),
                             gatos_pair(0, 0.0),
                             pair_plus<gatos_pair>(),
                             gatos_accumulate
                             <
                             gatos_pair,
                             binarization_value_type,
                             base_value_type
                             >());
    double b = (double)b_sums.second / (double)b_sums.first;

    typedef ImageFactory<OneBitImageView>::data_type data_type;
    typedef ImageFactory<OneBitImageView>::view_type view_type;
    data_type* data = new data_type(src.size(), src.origin());
    view_type* view = new view_type(*data);

    std::transform(src.vec_begin(),
                   src.vec_end(),
                   background.vec_begin(),
                   view->vec_begin(),
                   gatos_thresholder
                   <
                   typename T::value_type,
                   typename U::value_type
                   >(q, delta, b, p1, p2));

  delete binarization1->data();
  delete binarization1;
  delete binarization2->data();
  delete binarization2;
    return view;
}


// --------------------- Binarization ----------------------
/* this is the main function for binarization

 *mask*
 Mask image that defines the process region

 *do_wiener*
 1 if adding wiener filtering before binarization, otherwise 0

 *region_width*, *region_height*, *noise_variance*
 parameters for wiener filter

 *med_size*
 The kernel for median filter.

 *region size*, *sensitivity*, *dynamic range*, *lower bound*, *upper bound*
 parameters for sauvola binarization

 *q*, *p1*, *p2*
 parameters for gatos thresholding
 */
template<class T, class U>
OneBitImageView* binarization(const T &src, const U &mask, const FloatVector *hist,
                              int sign_wiener, size_t wiener_width, size_t wiener_height, double noise_variance,
                size_t med_size,
                size_t region_size, double sensitivity, int dynamic_range, int lower_bound, int upper_bound,
                double q, double p1, double p2)
{
  GreyScaleImageView* src_grey=to_greyscale(src);
  if (sign_wiener==1) {        // wiener filter
    GreyScaleImageView* src_wiener=wiener2_filter(*src_grey, wiener_width, wiener_height, noise_variance);
    delete src_grey->data();
    delete src_grey;
    src_grey=src_wiener;
  }
    // apply mask
  GreyScaleImageView* src_mask_black=mask_fill(*src_grey, mask, 0);
    // histogram equalization
  Image* adjust0=equalise_histogram_mask(*src_mask_black, mask, hist);
  GreyScaleImageView* adjust=static_cast<GreyScaleImageView*> (adjust0);
  GreyScaleImageView* adjust_mask_black=mask_fill(*adjust, mask, 0);
    // background estimation
  GreyScaleImageView* background=background_estimation(*adjust, med_size);
  GreyScaleImageView* background_mask_black=mask_fill(*background, mask, 0);
    // sauvola binarization
  OneBitImageView* binarization_coarse=sauvola_threshold(*adjust, region_size, sensitivity, dynamic_range, lower_bound, upper_bound);
    // gatos thresholding
  OneBitImageView* binarization=gatos_threshold_mask(*adjust_mask_black, *background_mask_black, *binarization_coarse, mask, q, p1, p2);

  delete src_grey->data();
  delete src_grey;
  delete src_mask_black->data();
  delete src_mask_black;
  delete adjust->data();
  delete adjust;
  delete adjust_mask_black->data();
  delete adjust_mask_black;
  delete background->data();
  delete background;
  delete background_mask_black->data();
  delete background_mask_black;
  delete binarization_coarse->data();
  delete binarization_coarse;
  return binarization;
}

#endif

