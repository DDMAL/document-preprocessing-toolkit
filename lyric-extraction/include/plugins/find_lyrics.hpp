#ifndef FIND_LYRICS_HPP
#define FIND_LYRICS_HPP 

#include <stdio.h> 
#include "gamera.hpp"
#include <cstdio> 
#include <vector> 
#include <algorithm> 
#include <iostream>
#include <sstream> 
#include <string> 
#include <exception> 

using namespace Gamera;

/* 
 * Given a line equation of the form y = m * x + b, draw this line across the
 * image and count the number of black pixels beneath it.
 */
template<class T>
int
count_black_under_line(const T &img, double m, double b)
{
  unsigned int count = 0;
  size_t i;
  for (i = img.ul_x(); i < img.lr_x(); ++i) {
    if ( is_black( img.get( Point(i, m * ((double)i) + b) ) ) )
      ++count;
  }
  return count;
}

/*
 * Given two different points, find the equation of the line that crosses
 * through both of them and then count the number of times that black is seen
 * beneath them.
 */
template<class T>
int
count_black_under_line_points(const T &img, double x0, double y0, double x1,
                              double y1)
{
  if ((x0 == x1) && (y0 == y1)) {
    std::ostringstream errmess;
    errmess << "The points must be different, instead got:";
    errmess << " x0=";
    errmess << x0;
    errmess << " y0=";
    errmess << y0;
    errmess << " x1=";
    errmess << x1;
    errmess << " y1=";
    errmess << y1;
    errmess << std::endl;
    throw std::runtime_error(errmess.str());
  }
  return count_black_under_line(img, (y1 - y0) / (x1 - x0), y0 - x0 * (y1 - y0)
                                      / (x1 - x0));
}

#endif /* FIND_LYRICS_HPP */
