#ifndef OPENCV_DATA_LOADER_H
#define OPENCV_DATA_LOADER_H

#include <opencv2/opencv.hpp>  // This includes most headers!

#include <X11/Xutil.h>

#include <X11/extensions/XShm.h>

class OpenCV_data_loader
{
private:
	int width;
	int height;
	Display* display;
	Window root;
	Screen* screen;

	int y_min;
	int y_max;


	int x_min;
	int x_max;

	double x_scale;
	double y_scale;


	XImage* ximg;

	XShmSegmentInfo shminfo;

public:
	OpenCV_data_loader(Display* display, Window &root, XWindowAttributes &attributes);
	void get_image_data(cv::Mat& img, cv::Mat& img_enemy);
	void save_to_disk(cv::Mat& img,  std::string img_name, const int num_failure);

	void transform_coords(double &x, double &y);
};

#endif // OPENCV_DATA_LOADER_H
