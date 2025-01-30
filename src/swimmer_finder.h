#ifndef SWIMMER_FINDER_H
#define SWIMMER_FINDER_H

#include "opencv_data_loader.h"
#include <chrono>
#include <cmath>
#include <random>

#include <X11/extensions/XTest.h>
#include <unistd.h>
#include "mouse_mover.h"

#include "gjrand.h"

class swimmer_finder
{
	gjrand64 gen;	//Standard mersenne_twister_engine seeded with rd()

	std::normal_distribution<double> human_reaction_dist;
	std::normal_distribution<double> wait_dist;
	std::normal_distribution<double> clam_timer_dist;

	std::chrono::system_clock::time_point lastFrame;
	std::chrono::system_clock::time_point time_last_break;
	std::chrono::system_clock::time_point time_last_blobber;
	std::chrono::system_clock::time_point  time_last_4;


	int wait_key4;
	int open_clam;
	int open_clam_timer;
	int num_failure;
    int num_enemy;

	int width;
	int height;

	int stride;

	int cm_x;
	int cm_y;
	int max_y;

	cv::Rect window1;
	cv::Rect window2;

	Display* display;
	Window root;
	XKeyboardState xkey;

	mouse_mover *mouse;

	std::vector<unsigned char> bg_red;
	std::vector<unsigned char> bg_green;
	std::vector<unsigned char> bg_blue;


	double resolution_factor;

	int counter;
	cv::Mat m_img_enemy;
	cv::Mat m_img;
	cv::Mat m_img_reference;
	cv::Mat m_img_work;
	cv::Mat m_img_work_grey;
	cv::Mat m_img_work_tmp;
	OpenCV_data_loader *opencv_data_loader;
	OpenCV_data_loader *opencv_bg_loader;
	cv::Ptr<cv::BackgroundSubtractor> pBackSub;





	void click_stealth();
	void click_mouse();
	void camera_zoom();
	void camera_reset();
	void exit_wow();


	void click_jump();
	void click_enter();
	void click_key();
	void click_key2();
	void click_key3();
	void click_key4();
	void mouseClick(int button);
	void InitImageFromDisplay();
	void BgFromDisplay();
	bool FindSwimmer(int &cm_x, int &cm_y, int& x_t, int& y_t, int& cm_x2, int& cm_y2, int& x_t2, int& y_t2);

	void thread4();
	bool checkEnemy();
	bool CheckSwimmer(const cv::Rect window1);

	void getImage();

	int write_jpeg2(XImage *img, const char* filename);
	int write_jpeg(XImage *img, const char* filename);



public:
	void main_loop();

	swimmer_finder();
	~swimmer_finder();
};

#endif // SWIMMER_FINDER_H
