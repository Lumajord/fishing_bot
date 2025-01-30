#ifndef SCREENSHOT_H
#define SCREENSHOT_H


#include <X11/Xutil.h>
#include <vector>


void print_image(XImage *img);
void print_image2(XImage *img, std::vector<unsigned char> bg_red, std::vector<unsigned char> bg_green, std::vector<unsigned char> bg_blue);


#endif // SCREENSHOT_H
