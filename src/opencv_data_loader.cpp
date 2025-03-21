#include "opencv_data_loader.h"

#include <X11/Xlib.h>

#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

OpenCV_data_loader::OpenCV_data_loader(Display* display_, Window& root_, XWindowAttributes& attributes)
{

	width = attributes.width;
	height = attributes.height;

	screen = attributes.screen;
	display = display_;
	root = root_;

	y_min = (int)((double)height*0.4);
	y_max = (int)((double)height*0.8);

	x_min = (int)((double)width*0.25);
	x_max = (int)((double)width*0.75);

	ximg = XShmCreateImage(display, DefaultVisualOfScreen(screen), DefaultDepthOfScreen(screen), ZPixmap, NULL, &shminfo, width, height);
	shminfo.shmid = shmget(IPC_PRIVATE, ximg->bytes_per_line * ximg->height, IPC_CREAT|0777);
	shminfo.shmaddr = ximg->data = (char*)shmat(shminfo.shmid, 0, 0);
	shminfo.readOnly = False;
	if(shminfo.shmid < 0)
		puts("Fatal shminfo error!");;
	Status s1 = XShmAttach(display, &shminfo);
	printf("XShmAttach() %s\n", s1 ? "success!" : "failure!");
}

void OpenCV_data_loader::get_image_data(cv::Mat &img, cv::Mat &img_enemy)
{
	XShmGetImage(display, root, ximg, 0, 0, 0x00ffffff);
	img = cv::Mat(height, width, CV_8UC4, ximg->data);
	cv::Rect myROI(x_min, y_min, x_max-x_min, y_max-y_min);
	//cv::Rect enemy_ROI(x_max/6, y_max/30, x_max/7, y_max/10);
	cv::Rect enemy_ROI(1350, 1100, 320, 100);
	//img_enemy = img(enemy_ROI);
	img(enemy_ROI).copyTo(img_enemy);

	cv::MatIterator_<cv::Vec4b> it; // = src_it.begin<cv::Vec3b>();
	for (it = img_enemy.begin<cv::Vec4b>(); it != img_enemy.end<cv::Vec4b>(); ++it)
	{
		(*it)[2] = 0;
	}
	/*
	 cv::Mat BGRChannels[3];
	split(img_enemy,BGRChannels); // split the BGR channesl
	BGRChannels[2]=cv::Mat::zeros(img_enemy.rows,img_enemy.cols,CV_8UC1);// removing Red channel
	merge(BGRChannels,3,img_enemy); // pack the image
	*/
	cv::cvtColor(img_enemy, img_enemy, cv::COLOR_BGR2HSV);
	img = img(myROI);
	cv::cvtColor(img, img, cv::COLOR_BGRA2BGR);
	x_scale = (double)(x_max-x_min)/(400.0);
	y_scale = (double)(y_max-y_min)/(300.0);
	cv::resize(img, img, cv::Size(400, 300), 0, 0, cv::INTER_AREA);
}



void OpenCV_data_loader::save_to_disk(cv::Mat &img, std::string img_name, const int num_failure)
{
	// Enable for debugging
	if(num_failure > 7)
	//if(true)
	{
		imwrite(img_name, img);
		printf("printing %s\n", img_name.c_str());
	}
	(void)img;
	(void)img_name;
}


void OpenCV_data_loader::transform_coords(double &x, double &y)
{
	x = x*x_scale + x_min;
	y = y*y_scale + y_min;
}
