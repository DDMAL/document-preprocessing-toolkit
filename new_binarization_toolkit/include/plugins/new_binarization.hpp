#ifndef new_binarization_toolkit
#define new_binarization_toolkit

#include "gameramodule.hpp"
#include "gamera.hpp"
#include "vigra/edgedetection.hxx"
#include "plugins/edgedetect.hpp"

using namespace Gamera;
using namespace std;
using namespace vigra;

typedef ImageData<OneBitPixel> OneBitImageData;
typedef ImageView<OneBitImageData> OneBitImageView;

typedef ImageData<GreyScalePixel> GreyScaleImageData;
typedef ImageView<GreyScaleImageData> GreyScaleImageView;

typedef ImageData<RGBPixel> RGBImageData;
typedef ImageView<RGBImageData> RGBImageView;

template<class T>
void test(T& image)
{
	std::fill(image.vec_begin(), image.vec_end(), white(image));
}

template <class T>
GreyScaleImageView* binarize(T& image)
{
    GreyScaleImageData *img_data = new GreyScaleImageData(image.size());
    GreyScaleImageView *img_view = new GreyScaleImageView(*img_data);
    
//    BImage src(image.width(), image.height()), edges(image.width(), image.height());
//    
//    edges = 0;
//    cannyEdgeImage(srcImageRange(image), destImage(edges), 0.8, 4.0, 1);
    
    img_view = canny_edge_image(image, 0.8, 4);
    return img_view;
}

#endif
