
#ifndef ypo21082008_lyricline
#define ypo21082008_lyricline

#include "gamera.hpp"
#include "plugins/segmentation.hpp"
#include "plugins/image_utilities.hpp"
#include "connected_components.hpp"

#include "math.h"
#include <list>
#include <numeric>
#include <algorithm>

#include <iostream>

#define PI 3.1415926

using namespace Gamera;
using namespace std;

// ========== Baseline Detection ===============

// ---------- Local Minima -----------
/* this function computes the number of black pixels in an image
 * it only works on binary image;
 */
template<class T>
unsigned int cc_area(const T &src)
{
    unsigned int area=0;
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            if (src.get(Point(n, m))!=0) {
                area++;
            }
        }
    }
    return area;
}


/* this function computes the minimum vertex of an connected component within a strip of image
   *label*
        the label of current connected component
 */
template<class T>
Point local_min_strip(const T &src, unsigned int label)
{
    unsigned int col=floor(0.5*double(src.ncols()));
    unsigned int row=src.nrows()-1;
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            if (src.get(Point(n, m))==label) {
                row=m;
                break;
            }
        }
    }
    return Point(col, row);
}


// this is the main function for local minima marking
/* This function marks the local minima vertices of each connected component.
 * The local minimum vertex is defined as a vertex whose y-coordinate is the global
   minimum inside a strip of a connected component, and whose x-coordinate
   is the center of the strip

    *ccs_list*
        connected component list of src.

    *strip_width*
        long connected components are broked into strips with certain width "strip_width".

    *thershold noise*
        minimum area of connected component not considered as noise.
 */
template<class T>
OneBitImageView* local_min(const T &src, ImageList &ccs_list, int strip_width, unsigned int threshold_noise)
{
    OneBitImageData* data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* local_minima = new OneBitImageView(*data);
    typename ImageFactory<OneBitImageView>::view_type* cc_strip = ImageFactory<OneBitImageView>::new_view(src);
    typename ImageFactory<OneBitImageView>::view_type* local_minima_strip = ImageFactory<OneBitImageView>::new_view(*local_minima);
    ImageList::iterator i;

    for (i = ccs_list.begin(); i != ccs_list.end(); i++) {
        ConnectedComponent<OneBitImageData>* cc_cur=static_cast<ConnectedComponent<OneBitImageData>* >(*i);
        // filter out small component (noise)
        unsigned int area = cc_area(*cc_cur);
        if (area>threshold_noise) {
            // compute local minimum within current component
                // divide the component into strips
            int number_strip=ceil((double(cc_cur->ncols()))/double(strip_width));
            Point cc_origin=cc_cur->origin();
            Point strip_origin;
            for (int n=1; n<number_strip; n++) {
                strip_origin=Point((cc_origin.x()+(n-1)*strip_width), cc_origin.y());
                /* this is to prevent the bug when cc_strip->rect_set(strip_origin,
                 * strip_size) is made too large */
                Size strip_size=Size(strip_width, 
                    /* if the lower-right coordinate would overshoot the
                     * height*/
                    ((cc_cur->nrows() + strip_origin.y()) > src.size().height())
                    /* make the strip height just the difference between the origin
                     * and source image height */
                    ? (src.size().height() - strip_origin.y())
                    /* otherwise just use the size of the connected component */
                    : (cc_cur->nrows()));
                    /* The reason why this is hacky is because really we should
                     * be figuring out why the cc_analysis returned a component
                     * that can't fit on the original image in the first place
                     * */
              /* The error occurs here.  cc_cur->nrows() might be bigger than
               * the edge of the image. My hacky fix is to just make it
               * cc_cur->nrows() if that's not too big, or the largest possible
               * size without going over the edge otherwise. */
                cc_strip->rect_set(strip_origin, strip_size);
                local_minima_strip->rect_set(strip_origin, strip_size);
                Point pos_strip=local_min_strip(*cc_strip, cc_cur->label());
                local_minima_strip->set(pos_strip, cc_cur->label());
            }
            strip_origin=Point((cc_origin.x()+(number_strip-1)*strip_width), cc_origin.y());
            cc_strip->rect_set(strip_origin, Point(cc_origin.x()+cc_cur->ncols()-1, cc_origin.y()+cc_cur->nrows()-1));
            local_minima_strip->rect_set(strip_origin, Point(cc_origin.x()+cc_cur->ncols()-1, cc_origin.y()+cc_cur->nrows()-1));
            Point pos_strip=local_min_strip(*cc_strip, cc_cur->label());
            local_minima_strip->set(pos_strip, cc_cur->label());
        }
    }
    delete cc_strip;
    delete local_minima_strip;
    return local_minima;
}



// ---------- Potential Baseline -----------
/* This function check if the weighted distance between two candidate
   vertices is within the threshold
 * The threshold function is the left part of a polynomial of degree 2. When
   x=0, y=angle; when y=0, x=dist
 * "true" is returned when the vertices pass the thresholding function, otherwise "false" is returned.
 */
bool weighted_dist_threshold(Point point1, Point point2, double angle, double dist)
{
    // euclidean distance between the two vertices
    double x=sqrt(pow(double(point2.x())-double(point1.x()), 2)+pow(double(point2.y())-double(point1.y()), 2));
    // angle between the two vertices
    double y=atan(abs(double(point2.y())-double(point1.y()))/(double(point2.x())-double(point1.x())));

    if (y<angle/pow(dist, 2)*pow(x-dist, 2))
        return true;
    else
        return false;
}


// this is the main function for potential baseline detection
/* Vertices are labeled with same label if their weighted distance is not over a threshold (connect virturally).

    *src*
        local minimum map

    *angle*
        tolerance on angle between two adjacent local minimum vertices of the same line.

    *dist*
        tolerance on distance (in x-axis) between two adjacent local minimum vertices of the same line.

    *min_group*
        minimun number of local minimum vertices in a potential baseline segment.
 */
template<class T>
OneBitImageView* potential_basline_seg(const T &src, double angle, double dist, int min_group)
{
    unsigned int number_seg=2; // label for each potential baseline segment
    Size box=Size(floor(dist), floor(tan(angle)*dist));  // bounding box of candidate points to connected with current point
    Point box_origin=Point(floor(dist), floor(tan(angle)*dist));
    // pad image by the size of bounding box
    OneBitImageView* src_pad=pad_image(src, box.height(), box.width(), box.height(), box.width(), 0);
    OneBitImageData* data = new OneBitImageData(src_pad->size(), src_pad->origin());
    OneBitImageView* baseline_seg_pad = new OneBitImageView(*data);
    list<Point> vertex_list;  // store the first "min_group" vertices within a segment

    for (unsigned int m=0; m<src_pad->nrows(); m++) {
        for (unsigned int n=0; n<src_pad->ncols(); n++) {
            // go through each unconnected vertex
            if (src_pad->get(Point(n, m))!=0) {
                vertex_list.push_back(Point(n, m));     // store the current vertex
                bool sign=true;     // sign=true, if a new vertex is added into the segment; otherwise, sign=false
                int count=1;    // number of vertices within a segment
                unsigned int m1=m;
                unsigned int n1=n;
                unsigned int m2;
                unsigned int n2;
                while (sign) {
                    bool connect=false;
                    // search the candidate region for the left-most vertex that pass the weighted_dist_threshold function
                    for (n2=n1+1; n2<n1+box.width(); n2++) {
                        for (m2=m1-box.height(); m2<m1+box.height(); m2++) {
                            if (src_pad->get(Point(n2, m2))!=0) {
                                connect=weighted_dist_threshold(Point(n1, m1), Point(n2, m2), angle, dist);
                                if (connect)
                                    break;
                            }
                        }
                        if (connect)
                            break;
                    }
                    // when there's no vertex to add, end the baseline segment
                    if (!connect)
                        sign=false;
                    else {
                        // store the first "min_group" vertices within a segment
                        if (count<min_group+1) {
                            vertex_list.push_back(Point(n2, m2));
                            count++;
                        }
                        // if the baseline is longer than "min_group", directly add new vertex into the segment,
                        // and remove the vertex from the local minimum map
                        else {
                            baseline_seg_pad->set(Point(n2, m2), number_seg);
                            src_pad->set(Point(n2, m2), 0);
                        }
                        m1=m2;
                        n1=n2;
                    }
                }
                // remove the first "min_group" vertices from the local minimum map
                if (count==min_group+1) {
                    while (!vertex_list.empty()) {
                        baseline_seg_pad->set(vertex_list.front(), number_seg);
                        src_pad->set(vertex_list.front(), 0);
                        vertex_list.pop_front();
                    }
                    number_seg++;
                }
                // empty the vertex list
                else {
                    while (!vertex_list.empty()) {
                        vertex_list.pop_front();
                    }
                }
            }
        }
    }

    // extract the center region
    baseline_seg_pad->rect_set(box_origin, src.size());
    OneBitImageView* baseline_seg=simple_image_copy(*baseline_seg_pad);
    baseline_seg->move(-box_origin.x(), -box_origin.y());
    (baseline_seg->data())->page_offset_x(0);
    (baseline_seg->data())->page_offset_y(0);

    delete src_pad->data();
    delete src_pad;
    delete baseline_seg_pad->data();
    delete baseline_seg_pad;

    return baseline_seg;
}


// ----------------------- Baseline Merge -------------------------
/* this function extract the connected component with label as "label_k"
 */
template<class T>
OneBitImageView* cc_kth(const T &src, unsigned int label_k)
{
    OneBitImageData* data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* cc_k = new OneBitImageView(*data);
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            unsigned int value1=floor(double(src.get(Point(n, m)))/double(label_k))-1;
            unsigned int value2=(src.get(Point(n, m)))%label_k;
            if ((value1==0)&&(value2==0))
                cc_k->set(Point(n, m), 1);
        }
    }
    return cc_k;
}


/* this function computes the right end of a group of vertices
 * the right end is defined as a vertex whose x-coordinate is same as the right-most vertex,
   and the y-coordinate is the average of all vertices.
 */
template<class T>
Point cc_right_end(const T &src)
{
    // the right-most x-coordinate
    unsigned int col=0;
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            if ((src.get(Point(n, m))!=0)&&(n>col)) {
                col=n;
            }
        }
    }
    // average y-coordinate
    unsigned int row=0;
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            if (src.get(Point(n, m))!=0) {
                row=row+(m+1);
            }
        }
    }
    row=row/cc_area(src)-1;

    return Point(col, row);
}


/* this function copies the vertices in "other" to "src", and label them as "label_other"
 */
template<class T, class U>
void cc_combine(T &src, const U &other, unsigned int label_other)
{
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            if (other.get(Point(n, m))!=0) {
                src.set(Point(n, m), label_other);
            }
        }
    }
}


/* this function delete the vertices labeled as "label"
 */
template<class T>
void cc_delete_kth(T &src, unsigned int label)
{
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            if (src.get(Point(n, m))==label) {
                src.set(Point(n, m), 0);
            }
        }
    }
}


// the main function for baseline merge
/* This function merges adjacent baseline segments if they are close enough.
 * The weighted distance is defined by both distance and direction
 */
template<class T>
OneBitImageView* baseline_merge(const T &src, double angle, double dist)
{
    unsigned int number_seg=2;      // label for each baseline segment
    Size box=Size(floor(dist), floor(tan(angle)*dist));     // bounding box of points in candidate segments to connected with current segment
    OneBitImageData* data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* baseline_potential = new OneBitImageView(*data);
    OneBitImageData* src_copy_data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* src_copy = new OneBitImageView(*src_copy_data);
    copy(src.vec_begin(),
                src.vec_end(),
                src_copy->vec_begin());

    for (unsigned int m=0; m<src_copy->nrows(); m++) {
        for (unsigned int n=0; n<src_copy->ncols(); n++) {
            if (src_copy->get(Point(n, m))!=0) {
                // extract current potential baseline segment
                unsigned int label_k=src.get(Point(n, m));
                OneBitImageView* cc_k=cc_kth(*src_copy, label_k);
                OneBitImageData* cc_k1_data = new OneBitImageData(cc_k->size(), cc_k->origin());
                OneBitImageView* cc_k1 = new OneBitImageView(*cc_k1_data);
                copy(cc_k->vec_begin(),
                         cc_k->vec_end(),
                         cc_k1->vec_begin());
                // compute the right end
                Point right_end_avg=cc_right_end(*cc_k);
                unsigned int m1=right_end_avg.y();
                unsigned int n1=right_end_avg.x();
                bool sign=true;     // sign=true, if a new segment is added into the baseline; otherwise, sign=false
                while (sign) {
                    sign=false;
                    // search within the candidate region for segments
                    for (unsigned int n2=n1+1; n2<min(n1+box.width(), src.ncols()-1); n2++) {
                        for (unsigned int m2=max<unsigned int>((unsigned int)0, m1-box.height()); m2<min(m1+box.height(), src.nrows()-1); m2++) {
                            if (src_copy->get(Point(n2, m2))!=0) {
                                // extract segment
                                unsigned int label_k2=src.get(Point(n2, m2));
                                OneBitImageView* cc_k2=cc_kth(*src_copy, label_k2);
                                // combine with current segment and remove the newly-found one from the old segment map
                                cc_combine(*cc_k1, *cc_k2, 1);
                                cc_delete_kth(*src_copy, label_k2);
                                delete cc_k2->data();
                                delete cc_k2;
                                // update the right end
                                right_end_avg=cc_right_end(*cc_k1);
                                m1=right_end_avg.y();
                                n1=right_end_avg.x();
                                sign=true;
                            }
                        }
                    }
                }
                // label the new baseline
                cc_combine(*baseline_potential, *cc_k1, number_seg);
                // remove the starting segment of new baseline from the old segment map
                cc_delete_kth(*src_copy, label_k);
                number_seg++;
                delete cc_k1->data();
                delete cc_k1;
                delete cc_k->data();
                delete cc_k;
            }
        }
    }

    delete src_copy->data();
    delete src_copy;
    return baseline_potential;
}


// -------------------- Baseline Validation -----------------------
/* this function fits data points to a straight line.
   it minimizes the chi-square error.
 * the code is adapted from Numerical Recipes: The Art of Scientific Computing, Ch.15 Modeling of Data, pp.665-666. Cambridge University Press, 2007
 */
void linear_least_square(list<Point> &point_list, double &para_a, double &para_b)
{
    double sx=0;
    double sy=0;
    double ss=0;
    double sxoss, t;
    double st2=0.0;
    para_a=0.0;
    list<Point>::iterator i;
    for (i = point_list.begin(); i != point_list.end(); i++) {
        sx += (*i).x();
        sy += (*i).y();
        ss++;
    }
    sxoss=sx/ss;
    for (i = point_list.begin(); i != point_list.end(); i++) {
        t=(*i).x()-sxoss;
        st2 += t*t;
        para_a += t*(*i).y();
    }
    para_a /= st2;
    para_b=(sy-sx*para_a)/ss;
}


/* this function detects black pixels in an image and fits them to a straight line
 */
template<class T>
void cc_line_fit(const T &src, double &para_a, double &para_b)
{
    // extract points
    list<Point> point_list;
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            if (src.get(Point(n, m))!=0) {
                point_list.push_back(Point(n, m));
            }
        }
    }
    // linear fitting
    linear_least_square(point_list, para_a, para_b);
}


// this is the main function for baseline validation
/* this function function validate the baseline segments by checking if there are enough local minimum vertices within a baseline.
 * The baseline is computed by linear least square fitting.

    *angle*
        tolerance on angle between baseline and horizon.

    *height*
        tolerance on distance (in y-axis) between local minimum vertices and the estimated baseline they belong to.

    *min_group*
        minimun number of local minimum vertices in a baseline other than current segment.
    *min_single*
        minimun number of local minimum vertices in a segment to pass the validation without linear fitting test.
 */
template<class T>
OneBitImageView* baseline_validation(const T &src, double angle, double height, int min_group, int min_single)
{
    unsigned int number_line=2; // label for each valid baseline segment
    OneBitImageData* data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* baseline = new OneBitImageView(*data);
    unsigned int number_seg=*(max_element(src.vec_begin(), src.vec_end()));
    unsigned int height_var;

    for (unsigned int k=2; k<=number_seg; k++) {
        // extract current baseline segment
        OneBitImageView* cc_k=cc_kth(src, k);
        double line_para_a, line_para_b;
        // linear fitting
        cc_line_fit(*cc_k, line_para_a, line_para_b);
        int number_k=cc_area(*cc_k);    // number of vertices within current segment

        // compute the number of vertices along current estimated baseline
        int count=0;
        if (abs(line_para_a)<tan(angle)) {
            for (unsigned int m=0; m<src.nrows(); m++) {
                for (unsigned int n=0; n<src.ncols(); n++) {
                    if ((src.get(Point(n, m))!=0)&&(src.get(Point(n, m))!=k)) {
                        height_var=abs(m-(n*line_para_a+line_para_b));
                        if (height_var<height) {
                            cc_k->set(Point(n, m), 1);
                            count++;
                        }
                    }
                }
            }
        }
        // label valid segment
        if ((count>min_group)||(number_k>min_single)) {
            cc_combine(*baseline, *cc_k, number_line);
            number_line++;
        }
    }
    return baseline;
}


// ------------------------- Baseline Detection Main --------------------
/*  this function detects the baseline of lyrics, and returns the local minimum vertex map of lyric baseline.

    *staffspace*
        staffspace height.

    *thershold noise*
        minimum area of connected component not considered as noise.

    *scalar_cc_strip*
        long connected components are broked into strips with certain width, scalar_cc_strip*staffspace.

    *seg_angle_degree*
        tolerance on angle between two adjacent local minimum vertices of the same line (in degree).

    *scalar_seg_dist*
        scalar_seg_dist*staffspace: tolerance on distance (in x-axis) between two adjacent local minimum vertices of the same line.

    *min_group*
        minimun number of local minimum vertices in a potential baseline segment.

    *merge_angle_degree*
        tolerance on angle between two adjacent potential baseline segments to merge into one line (in degree).

    *scalar_merge_dist*
        scalar_merge_dist*staffspace: tolerance on distance (in x-axis) between two adjacent potential baseline segments to merge into one line.

    *valid_angle_degree*
        tolerance on angle between baseline and horizon(in degree).

    *scalar_valid_height*
        scala_valid_height*staffspace: tolerance on distance (in y-axis) between local minimum vertices and the estimated baseline they belong to.

    *valid_min_group*
        minimun number of local minimum vertices in a baseline.
*/
template<class T>
OneBitImageView* baseline_detection(const T &src, double staffspace,
                                    unsigned int threshold_noise, double scalar_cc_strip,
                                    double seg_angle_degree, double scalar_seg_dist, int min_group,
                                    double merge_angle_degree, double scalar_merge_dist,
                                    double valid_angle_degree, double scalar_valid_height, int valid_min_group)
{
    OneBitImageData* data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* src_copy = new OneBitImageView(*data);
    copy(src.vec_begin(),
            src.vec_end(),
            src_copy->vec_begin());

    // label connected component
    ImageList* ccs_list;
    ccs_list=cc_analysis(*src_copy);

    // local minima detection
    int strip_width=ceil(staffspace*scalar_cc_strip);
    OneBitImageView* local_minima=local_min(*src_copy, *ccs_list, strip_width, threshold_noise);

    // potential baseline segment detection
    double seg_angle=seg_angle_degree/180.0*PI;
    int seg_dist=staffspace*scalar_seg_dist;
    OneBitImageView* baseline_seg=potential_basline_seg(*local_minima, seg_angle, seg_dist, min_group);

    // baseline segment merge
    double merge_angle=merge_angle_degree/180.0*PI;
    int merge_dist=staffspace*scalar_merge_dist;
    OneBitImageView* baseline_potential=baseline_merge(*baseline_seg, merge_angle, merge_dist);

    // baseline validation
    double valid_height=scalar_valid_height*staffspace;
    double valid_angle=valid_angle_degree/180.0*PI;
    int valid_min_single=valid_min_group+min_group;
    OneBitImageView* baseline=baseline_validation(*baseline_potential, valid_angle, valid_height, valid_min_group, valid_min_single);

    // re-label the valid baseline vertices with the corresponding connected component labels
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            if (baseline->get(Point(n,m))!=0)
                baseline->set(Point(n,m), local_minima->get(Point(n, m)));
        }
    }

    delete src_copy->data();
    delete src_copy;
    delete local_minima->data();
    delete local_minima;
    delete baseline_seg->data();
    delete baseline_seg;
    delete baseline_potential->data();
    delete baseline_potential;
    delete ccs_list;

    return baseline;
}



// ========================== Lyric Height Estimation ========================
/* this function computes the height of connected component with label as "label"
 */
template<class T>
unsigned int cc_height(const T &src, unsigned int label)
{
    bool top=true;
    unsigned int high=0;
    unsigned int low=0;
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            if (src.get(Point(n, m))==label) {
                if (top) {
                    high=m;
                    top=false;
                }
                else
                    low=m;
            }
        }
    }
    return (low-high+1);
}


// this is the main function for lyric height estimation
/* This function estimates the lyric height by averaging the height of lyric
   fragments. Those fragments are reconstructed from the baseline fragments.
   The reconstruction is the inverse processing of marking the local minima.
   See LocalMinimum part for details of the forward processing.

    *baseline*
        local minimum vertex map of lyric baseline.

    *staffspace*
        staffspace height.

    *scalar_cc_strip*
        it should be the same as the parameter used in baseline_detection.

    *scalar_height*
        scala_valid_height*staffspace: maximum height of potential lyric.
 */
template<class T, class U>
double lyric_height_estimation(const T &src, const U &baseline, double staffspace,
                               double scalar_cc_strip, double scalar_height)
{
    int strip_width=ceil(staffspace*scalar_cc_strip);

    OneBitImageData* data = new OneBitImageData(src.size(), src.origin());
    OneBitImageView* src_copy = new OneBitImageView(*data);
    copy(src.vec_begin(),
            src.vec_end(),
            src_copy->vec_begin());

    ImageList* ccs_list=cc_analysis(*src_copy);
    OneBitImageView* cc_strip = new OneBitImageView(*data);

    unsigned int count=0;
    unsigned int height=ceil(staffspace*scalar_height);
    double lyric_height=0;

    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            // reconstruct local strip and extract lyric fragment
            if (baseline.get(Point(n, m))!=0) {
                unsigned int label_k=baseline.get(Point(n, m));
                ConnectedComponent<OneBitImageData>* cc_cur;
                ImageList::iterator i;
                for (i = ccs_list->begin(); i != ccs_list->end(); i++) {
                    cc_cur=static_cast<ConnectedComponent<OneBitImageData>* >(*i);
                    if (cc_cur->label()==label_k) {
                        Point cc_origin=cc_cur->origin();
                        int strip_nth=floor(double(n-cc_origin.x())/double(strip_width))+1;
                        Point strip_origin=Point((cc_origin.x()+(strip_nth-1)*strip_width), cc_origin.y());
                        Size strip_size=Size(min(strip_width, int(cc_cur->ncols()+cc_origin.x()-1-strip_origin.x())),
                          /* if the lower-right coordinate would overshoot the
                           * height*/
                          ((cc_cur->nrows() + strip_origin.y()) > src.size().height())
                          /* make the strip height just the difference between the origin
                           * and source image height */
                          ? (src.size().height() - strip_origin.y())
                          /* otherwise just use the size of the connected component */
                          : (cc_cur->nrows()));
                          /* The reason why this is hacky is because really we should
                           * be figuring out why the cc_analysis returned a component
                           * that can't fit on the original image in the first place
                           * */
                          /* The error occurs here.  cc_cur->nrows() might be bigger than
                           * the edge of the image. My hacky fix is to just make it
                           * cc_cur->nrows() if that's not too big, or the largest possible
                           * size without going over the edge otherwise. */
                        cc_strip->rect_set(strip_origin, strip_size);
                        // compute height of fragment
                        unsigned int height_strip=cc_height(*cc_strip, label_k);
                        if (height_strip<height) {
                            lyric_height=lyric_height+height_strip;
                            count++;
                        }
                    break;
                    }
                }
            }
        }
    }

    lyric_height=lyric_height/double(count);
    delete ccs_list;
    delete cc_strip;
    delete src_copy->data();
    delete src_copy;
    cout<<"lyric height:"<<lyric_height<<'\n';
    return lyric_height;
}


// =========================== Lyric Line Fit ===============================
/* This function reconstructs the lyric region by linear fitting the
   baseline and dilating the baseline with the lyric height.
 * Note: It does nothing about the overlap, ascent and descent stuff.

    *baseline*
        local minimum vertex map of lyric baseline.

    *lyric_height*
        estimation of average lyric height.

    *fit_angle_degree*
        tolerance on angle between lyric line and horizon(in degree).

    *scalar_search_height*
        scala_search_height*lyric_height: tolerance on distance (in y-axis) between local minimum vertices that belong to a single lyric line.

    *scalar_fit_up*
        scalar_fit_up*lyric_height: height of lyric portion above baseline.

    *scalar_fit_down*
        scalar_fit_down*lyric_height: height of lyric portion beneath baseline.
*/
template<class T, class U>
OneBitImageView* lyric_line_fit(const T &src, const U &baseline, double lyric_height,
                                double fit_angle_degree, double scalar_search_height,
                                double scalar_fit_up, double scalar_fit_down)
{
    double fit_angle=fit_angle_degree/180*PI;
    unsigned int search_height=floor(scalar_search_height*lyric_height);

    OneBitImageData* data = new OneBitImageData(baseline.size(), baseline.origin());
    OneBitImageView* baseline_copy = new OneBitImageView(*data);
    copy(baseline.vec_begin(),
            baseline.vec_end(),
            baseline_copy->vec_begin());

    OneBitImageData* mask_data = new OneBitImageData(baseline.size(), baseline.origin());
    OneBitImageView* mask = new OneBitImageView(*mask_data);

    OneBitImageView* baseline_patch = new OneBitImageView(*data);
    for (unsigned int m=0; m<src.nrows(); m++) {
        for (unsigned int n=0; n<src.ncols(); n++) {
            if (baseline_copy->get(Point(n, m))!=0) {
                // extract next baseline
                baseline_patch->rect_set(baseline_copy->origin(), Size(baseline.width(), min(m+1+search_height, (unsigned int)(
                          /* if the lower-right coordinate would overshoot the
                           * height*/
                          ((baseline_copy->nrows() + (baseline_copy->origin()).y()) > src.size().height())
                          /* make the strip height just the difference between the origin
                           * and source image height */
                          ? (src.size().height() - (baseline_copy->origin()).y())
                          /* otherwise just use the size of the connected component */
                          : (baseline_copy->nrows())))));
                          /* The reason why this is hacky is because really we should
                           * be figuring out why the cc_analysis returned a component
                           * that can't fit on the original image in the first place
                           * */

                if (cc_area(*baseline_patch)>1) {  // a baseline should contain at least 2 vertices
                    double line_para_a, line_para_b;
                    cc_line_fit(*baseline_patch, line_para_a, line_para_b);
                    // linear least square fitting
                    if (abs(line_para_a)>tan(fit_angle))
                        break;
                    // expand baseline with lyric height and marks it in mask image
                    for (unsigned int t=0; t<src.ncols(); t++) {
                        double s0=t*line_para_a+line_para_b;
                        unsigned int s1=max(0.0, int(baseline_patch->offset_y())+s0-int(lyric_height*scalar_fit_up));
                        unsigned int s2=min(src.nrows()-1, (Gamera::coord_t)(int(baseline_patch->offset_y())+s0+int(lyric_height*scalar_fit_down)));
                        for (unsigned int s=s1; s<=s2; s++) {
                            mask->set(Point(t, s), 1);
                        }
//                  if (s1==0)
//                  cout<<"s0:"<<s0<<", s2:"<<s2<<'\n';
                    }
                }
                fill(baseline_patch->vec_begin(),
                baseline_patch->vec_end(),
                0);
            }
        }
    }

    delete baseline_patch;
    delete baseline_copy->data();
    delete baseline_copy;

    return mask;
}


// ============================ Lyric Line Detection =========================
// integrated function for whole lyric line detection process
template<class T>
OneBitImageView* lyric_line_detection(const T &src, double staffspace,
                                    unsigned int threshold_noise, double scalar_cc_strip,
                                    double seg_angle_degree, double scalar_seg_dist, int min_group,
                                    double merge_angle_degree, double scalar_merge_dist,
                                    double valid_angle_degree, double scalar_valid_height, int valid_min_group,
                                    double scalar_height,
                                    double fit_angle_degree, double scalar_search_height,
                                    double scalar_fit_up, double scalar_fit_down)
{
    OneBitImageView* baseline=baseline_detection(src, staffspace,
                                    threshold_noise, scalar_cc_strip,
                                    seg_angle_degree, scalar_seg_dist, min_group,
                                    merge_angle_degree, scalar_merge_dist,
                                    valid_angle_degree, scalar_valid_height, valid_min_group);

    double lyric_height=lyric_height_estimation(src, *baseline, staffspace,
                               scalar_cc_strip, scalar_height);

    OneBitImageView* mask=lyric_line_fit(src, *baseline, lyric_height,
                                fit_angle_degree, scalar_search_height,
                                scalar_fit_up, scalar_fit_down);

    delete baseline->data();
    delete baseline;
    return mask;
}


#endif

