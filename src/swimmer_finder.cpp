#include "swimmer_finder.h"

#include <X11/Xutil.h>

#include <X11/extensions/XTest.h>
#include <unistd.h>
//#include <linux/reboot.h>
//#include <sys/reboot.h>
#include <cmath>
#include <thread>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "mouse_mover.h"
#include <thread>

#include "screenshot.h"

#include "randutils.h"

//#define LOOK_FOR_ENEMY
#define APPLY_BLOBBER
#define OPEN_CLAMS
/*
static int clamp(int x, const int low, const int high)
{
	return std::min(high, std::max(x, low));
}
*/
#define DEBUG


static bool condition_red(unsigned char  color, unsigned char saturation, unsigned char lightness)
{
	return ((color < 15) || (color > 175)) &&  (saturation > 40) && (lightness > 20) && (lightness < 200); // default
	//return ((color < 35) || (color > 155)) &&  (saturation > 40) && (lightness > 10) && (lightness < 200);
	//return ((color < 75) || (color > 125)) &&  (saturation > 40) && (lightness > 20) && (lightness < 200); // does not work in moonglade // does not work at night in SW
	// return true; // works in moonglade
}

static bool condition_blue(unsigned char  color, unsigned char saturation, unsigned char lightness)
{
	//return ((color > 60) && (color < 150)) &&  (saturation > 40) && (lightness > 20) && (lightness < 200);
	//return ((color > 40) && (color < 180)) &&  (saturation > 40) && (lightness > 20) && (lightness < 200); // does not work in moonglade
	// return ((color > 40) && (color < 160)) &&  (saturation > 60) && (lightness > 60) && (lightness < 200); // works in moonglade, also default
	return true;
}


swimmer_finder::swimmer_finder()
{

	open_clam = 0;
	num_failure = 0;

	uint32_t seeds[4];
	randutils::auto_seed_256 seeder;
	seeder.generate(seeds, seeds+4);
	uint64_t seed1;
	uint64_t seed2;
	seed1 = (uint64_t)seeds[0] << 32 | seeds[1];
	seed2 = (uint64_t)seeds[2] << 32 | seeds[3];

	gen = gjrand64(seed1, seed2);

	counter = 0;
	XInitThreads();
	display = XOpenDisplay(nullptr);

	if(display == NULL)
	{
		fprintf(stderr, "Cannot initialize the display\n");
		exit(EXIT_FAILURE);
	}

	root = DefaultRootWindow(display);

	XWindowAttributes attributes;
	XGetWindowAttributes(display, root, &attributes);


	width = attributes.width;
	height = attributes.height;

	m_img_enemy = cv::Mat(width, height, CV_32FC4, cv::Scalar::all(0));
	m_img = cv::Mat(width, height, CV_32FC4, cv::Scalar::all(0));
	m_img_reference = cv::Mat(width, height, CV_32FC4, cv::Scalar::all(0));
	m_img_work = cv::Mat(width, height, CV_32FC4, cv::Scalar::all(0));
	m_img_work_grey = cv::Mat(width, height, CV_32FC4, cv::Scalar::all(0));
	m_img_work_tmp = cv::Mat(width, height, CV_32FC4, cv::Scalar::all(0));

	mouse = new mouse_mover(display);

	cm_x = 0;
	cm_y = 0;


	wait_dist = std::normal_distribution<double> (1000, 100);
	human_reaction_dist = std::normal_distribution<double> (247.63, 61.84);
	clam_timer_dist = std::normal_distribution<double>(4.0, 2.0);



	opencv_data_loader = new OpenCV_data_loader(display, root, attributes);
	opencv_bg_loader = new OpenCV_data_loader(display, root, attributes);
	pBackSub = cv::createBackgroundSubtractorMOG2();

}

swimmer_finder::~swimmer_finder()
{
	XCloseDisplay(display);
	delete[] opencv_data_loader;
	delete[] opencv_bg_loader;
	delete[] mouse;
}

void swimmer_finder::click_key()
{
	unsigned int keycode;
	keycode = XKeysymToKeycode(display, XK_1);

	XTestFakeKeyEvent(display, keycode, true, 0);
	XFlush(display);

	int wait = (int)human_reaction_dist(gen)/4;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode, false, 0);
	XFlush(display);

}

void swimmer_finder::click_key2()
{
	unsigned int keycode;
	keycode = XKeysymToKeycode(display, XK_2);

	XTestFakeKeyEvent(display, keycode, true, 0);
	XFlush(display);

	int wait = (int)human_reaction_dist(gen)/4;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode, false, 0);
	XFlush(display);

}


void swimmer_finder::click_key3()
{
	unsigned int keycode;
	keycode = XKeysymToKeycode(display, XK_3);

	XTestFakeKeyEvent(display, keycode, true, 0);
	XFlush(display);

	int wait = (int)human_reaction_dist(gen)/4;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode, false, 0);
	XFlush(display);

}


void swimmer_finder::click_key4()
{

#ifdef LOOK_FOR_ENEMY
	auto time_now = std::chrono::high_resolution_clock::now();
	auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(time_now - time_last_4).count();

	if(delta > wait_key4)
	{
	wait_key4 = (int)((2.0*wait_dist(gen)) + 12.0*human_reaction_dist(gen));
	printf("clicked 4\n");
	unsigned int keycode;
	keycode = XKeysymToKeycode(display, XK_4);

	XTestFakeKeyEvent(display, keycode, true, 0);
	XFlush(display);

	int wait = (int)human_reaction_dist(gen)/4;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode, false, 0);
	XFlush(display);

	time_last_4 = std::chrono::high_resolution_clock::now();
	wait = (int)human_reaction_dist(gen)/6;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));
	}

#endif // LOOK_FOR_ENEMY

}


void swimmer_finder::click_jump()
{
	unsigned int keycode;
	keycode = XKeysymToKeycode(display, XK_space);
	XTestFakeKeyEvent(display, keycode, true, 0);
	XFlush(display);

	int wait = (int)human_reaction_dist(gen)/2;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));
	XTestFakeKeyEvent(display, keycode, false, 0);
	XFlush(display);

	wait = 700;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

}

void swimmer_finder::click_enter()
{
	unsigned int keycode;
	keycode = XKeysymToKeycode(display, XK_Return);
	XTestFakeKeyEvent(display, keycode, true, 0);
	XFlush(display);

	int wait = (int)human_reaction_dist(gen)/4;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));
	XTestFakeKeyEvent(display, keycode, false, 0);
	XFlush(display);

}


void swimmer_finder::click_mouse()
{

    int wait;
    /*
	unsigned int keycode2;
	keycode2 = XKeysymToKeycode(display, XK_Shift_L);
	XTestFakeKeyEvent(display, keycode2, true, 0);
	printf("Click shift\n");
	XFlush(display);

	wait = (int)human_reaction_dist(gen)/2;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));
    */

	printf("Click Mouse\n");
	XTestFakeButtonEvent(display, Button3, true, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen)/4;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeButtonEvent(display, Button3, false, 0);
	XFlush(display);
	printf("Released Mouse\n");

    /*
	wait = (int)human_reaction_dist(gen)/3;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode2, false, 0);
	XFlush(display);
	printf("Released Shift\n");
    */

    return;

}



void swimmer_finder::click_stealth()
{
	unsigned int keycode1;
	unsigned int keycode2;

	int wait;

	keycode1 = XKeysymToKeycode(display, XK_Control_L);
	XTestFakeKeyEvent(display, keycode1, true, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen);
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	keycode2 = XKeysymToKeycode(display, XK_F1);
	XTestFakeKeyEvent(display, keycode2, true, 0);
	XFlush(display);


	wait = (int)human_reaction_dist(gen)/3;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode1, false, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen)/3;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode2, false, 0);
	XFlush(display);
}



void swimmer_finder::exit_wow()
{
	unsigned int keycode1;
	unsigned int keycode2;
	unsigned int keycode3;

	int wait;

	keycode1 = XKeysymToKeycode(display, XK_Super_L);
	XTestFakeKeyEvent(display, keycode1, true, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen);
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	keycode2 = XKeysymToKeycode(display, XK_Shift_L);
	XTestFakeKeyEvent(display, keycode2, true, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen);
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	keycode3 = XKeysymToKeycode(display, XK_Q);
	XTestFakeKeyEvent(display, keycode3, true, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen)/2;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode3, false, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen)/4;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode1, false, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen)/3;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode2, false, 0);
	XFlush(display);
}


void swimmer_finder::camera_reset()
{
	unsigned int keycode;


	int wait;


	keycode = XKeysymToKeycode(display, XK_End);
	XTestFakeKeyEvent(display, keycode, true, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen)/2;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode, false, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen);
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	keycode = XKeysymToKeycode(display, XK_End);
	XTestFakeKeyEvent(display, keycode, true, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen)/2;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode, false, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen);
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));


	for(int i = 0; i < 3; ++i)
	{
		keycode = XKeysymToKeycode(display, XK_Home);
		XTestFakeKeyEvent(display, keycode, true, 0);
		XFlush(display);

		wait = (int)human_reaction_dist(gen)/2;
		std::this_thread::sleep_for(std::chrono::milliseconds(wait));

		XTestFakeKeyEvent(display, keycode, false, 0);
		XFlush(display);

		wait = (int)human_reaction_dist(gen);
		std::this_thread::sleep_for(std::chrono::milliseconds(wait));
	}

	wait = (int)wait_dist(gen);
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));
}

void swimmer_finder::camera_zoom()
{
	unsigned int keycode, keycode1, keycode2;


	keycode1 = XKeysymToKeycode(display, XK_Super_L);
	keycode2 = XKeysymToKeycode(display, XK_f);

	XTestFakeKeyEvent(display, keycode1, true, 0);
	XFlush(display);
	int wait = (int)human_reaction_dist(gen);
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode2, true, 0);
	XFlush(display);
	wait = (int)human_reaction_dist(gen)/2;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));


	XTestFakeKeyEvent(display, keycode2, false, 0);
	XFlush(display);
	wait = (int)human_reaction_dist(gen);
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));


	XTestFakeKeyEvent(display, keycode1, false, 0);
	XFlush(display);
	wait = (int)human_reaction_dist(gen)*3;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));



	keycode = XKeysymToKeycode(display, XK_End);
	XTestFakeKeyEvent(display, keycode, true, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen)/2;
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	XTestFakeKeyEvent(display, keycode, false, 0);
	XFlush(display);

	wait = (int)human_reaction_dist(gen);
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));


	for(int i = 0; i < 5; ++i)
	{
		keycode = XKeysymToKeycode(display, XK_Home);
		XTestFakeKeyEvent(display, keycode, true, 0);
		XFlush(display);

		wait = (int)human_reaction_dist(gen)/2;
		std::this_thread::sleep_for(std::chrono::milliseconds(wait));

		XTestFakeKeyEvent(display, keycode, false, 0);
		XFlush(display);

		wait = (int)human_reaction_dist(gen);
		std::this_thread::sleep_for(std::chrono::milliseconds(wait));
	}
}


bool swimmer_finder:: checkEnemy()
{


#ifdef LOOK_FOR_ENEMY
	double score = cv::mean(m_img_enemy)[2];
	int anti_afk = 0;
	if(score > 30.0)
	{
		click_stealth();
		printf("Found enemy with score %.5e\n", score);
        char buff[256];
        sprintf(buff, "found_enemy%d.png", num_enemy++);
		imwrite(buff, m_img_enemy);

		std::this_thread::sleep_for(std::chrono::milliseconds(7000));

		while(score > 30)
		{
			printf("Enemy still here\n");
			int wait = (int)(5.0*wait_dist(gen));
			std::this_thread::sleep_for(std::chrono::milliseconds(wait));
			click_key4();
			anti_afk++;

			opencv_bg_loader->get_image_data(m_img_reference, m_img_enemy);
			score = cv::mean(m_img_enemy)[2];

			if(anti_afk > 60)
			{
				click_jump();
				anti_afk = 0;
			}
		}

		printf("check enemy returned true\n");
		return true;
	}else{
		return false;
	}
#else // LOOK_FOR_ENEMY
    return false;
#endif

}

void swimmer_finder::BgFromDisplay()
{
	//for(int i = 0; i < 90; ++i)
	//{
	opencv_bg_loader->get_image_data(m_img_reference, m_img_enemy);
	if(checkEnemy())
	{
		opencv_bg_loader->get_image_data(m_img_reference, m_img_enemy);
	}
	//pBackSub->apply(m_img_reference, m_img_work, 0.1);
	//std::this_thread::sleep_for(std::chrono::milliseconds(33));
	//}
	counter+=2;
	opencv_bg_loader->save_to_disk(m_img_reference, "cv_bg" + std::to_string(counter) + ".png", num_failure);
}


static void expand_window(cv::Rect &window, const int width, const int height)
{
	window.x -= (window.width*2)/5;
	window.width *= 9;
	window.width /= 5;

	window.y -= (window.height*2)/5;
	window.height *= 9;
	window.height /= 5;

	if(window.x < 0)
	{
		window.width += window.x;
		window.x = 0;
	}

	if(window.y < 0)
	{
		window.height += window.y;
		window.y = 0;
	}


	if(window.x + window.width > width)
	{
		window.width -= (window.x + window.width - width);
	}
	if(window.y + window.height > height)
	{
		window.height -= (window.y + window.height - height);
	}
}

bool swimmer_finder::FindSwimmer(int &cm_x, int &cm_y, int &x_t, int &y_t,
								 int &cm_x2, int &cm_y2, int &x_t2, int &y_t2)
{

	#ifdef DEBUG
	printf("Searching for swimmer\n");
	#endif
	click_key4();
	opencv_data_loader->get_image_data(m_img, m_img_enemy);
	if(checkEnemy())
	{
		return false;
	}

	//cv::GaussianBlur(m_img, m_img, cv::Size(5,5), 0, 0);
	//cv::GaussianBlur(m_img_reference, m_img_reference, cv::Size(5,5), 0, 0);
	cv::absdiff(m_img, m_img_reference, m_img_work);


	// remove grey colors
	double global_mean = 0.0;
	int num_pixels = 0;

	for(int j=0; j<m_img_work.rows; ++j)
	{
		for(int i=0; i<m_img_work.cols; ++i)
		{

			num_pixels++;
			cv::Vec3b & pix = m_img_work.at<cv::Vec3b>(j,i);

			// substract the mean color or minimum color from each pixel
			double dist = (pix[0] + pix[1] + pix[2]);
			int minc = std::min(std::min(pix[0], pix[1]), pix[2]);
			dist = dist/3;

			int mean = std::min((int)dist, minc);
			pix[0] = pix[0]-mean;
			pix[1] = pix[1]-mean;
			pix[2] = pix[2]-mean;

			dist = (pix[0] + pix[1] + pix[2]);
			dist = dist/3;
			global_mean += dist;
		}
	}

	opencv_data_loader->save_to_disk(m_img_work, "Finder_diff_grey_removed_" + std::to_string(counter) + ".png", num_failure);

	// apply treshold
	global_mean /= (double)(num_pixels);
	global_mean = global_mean*6.0;
	for(int j=0; j<m_img_work.rows; ++j)
	{
		for(int i=0; i<m_img_work.cols; ++i)
		{
			cv::Vec3b & pix = m_img_work.at<cv::Vec3b>(j,i);

			double dist = (pix[0] + pix[1] + pix[2]);
			dist = dist/3;

			if(dist < global_mean)
			{
				pix[0] = 0;
				pix[1] = 0;
				pix[2] = 0;
			}
			else
			{
				pix[0] = 255;
				pix[1] = 255;
				pix[2] = 255;
			}
		}
	}

	cv::GaussianBlur(m_img_work, m_img_work, cv::Size(5,5), 0, 0);
	cv::cvtColor(m_img_work, m_img_work_grey, cv::COLOR_BGR2GRAY);


	opencv_data_loader->save_to_disk(m_img_work_grey, "finder_treshold" + std::to_string(counter) + ".png", num_failure);

	m_img_work_grey = m_img_work_grey > 85;

	std::vector<std::vector<cv::Point> > contours;

    const int erosion_size = 3;
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(erosion_size, 3));
    cv::erode(m_img_work_grey, m_img_work_grey, kernel);

	findContours( m_img_work_grey, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );

	if(contours.size() == 0)
	{
#ifdef DEBUG
		printf("Find Swimmer found no valid contours\n");
#endif
		num_failure++;
		click_jump();
		std::this_thread::sleep_for(std::chrono::seconds(1));
		return false;
	}
	else
	{
#ifdef DEBUG
		printf("Found %ld contours\n", contours.size());
#endif
	}


	size_t max_id = (size_t)-1;
	size_t second_id = (size_t)-1;
	double max_area = 0.0;
	double second_area = 0.0;
	for(size_t i = 0; i < contours.size(); ++i)
	{
		double area = cv::contourArea(contours[i]);


		bool color_test = false;
		if(area > (double)(m_img.rows*m_img.cols)/5000.0)
		{
	    cv::Rect test_window = cv::boundingRect(contours[i]);
		m_img_work_tmp = m_img(test_window).clone();

		opencv_data_loader->save_to_disk(m_img_work_tmp, "Finder_test_" + std::to_string(counter) + "_" + std::to_string(i) + ".png", num_failure);
		cv::cvtColor(m_img_work_tmp, m_img_work_tmp, cv::COLOR_BGR2HSV);

		// check of colors of swimmer are within the window
		int num_red_pixels = 0;
		int num_blue_pixels = 0;
		for(int j = 0; j <  m_img_work_tmp.rows; ++j)
		{
			for(int i = 0; i < m_img_work_tmp.cols; ++i)
			{

				const cv::Vec3b pix = m_img_work_tmp.at<cv::Vec3b>(j,i);

				if(condition_red(pix[0], pix[1], pix[2]))
				{
					++num_red_pixels;
				}
				if(condition_blue(pix[0], pix[1], pix[2]))
				{
					++num_blue_pixels;
				}
			}
		}

		const int promille_area = m_img_work_tmp.rows*m_img_work_tmp.cols / 1000;
		color_test = ((num_red_pixels > promille_area) && (num_blue_pixels > promille_area));

#ifdef DEBUG
		if(!color_test)
		{
			printf("Image %s	failed the test	with red=%d	and blue=%d, area = %.5g\n", ("Finder_test_" + std::to_string(counter) + "_" + std::to_string(i) + ".png").c_str(), num_red_pixels, num_blue_pixels, area);
		}

		if(color_test)
		{
			printf("Test window: (%d	%d)		(%d	%d)	(%d	%d)\n", test_window.x, test_window.y, test_window.x+test_window.width, test_window.y+test_window.height, num_red_pixels, num_blue_pixels);
		}
#endif
		}



		if((area > max_area) && color_test)
		{
			second_id = max_id;
			second_area = max_area;
			max_area = area;
			max_id = i;
		}
		else if((area > second_area) && color_test)
		{
			second_area = area;
			second_id = i;
		}
	}

	if(max_id == (size_t)-1)
	{
#ifdef DEBUG
		printf("Finder found nothing\n");
#endif
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		num_failure++;
		return false;
	}

	cv::Moments Mom = cv::moments(contours[max_id], true);
	//cv::Rect rect = cv::boundingRect(contours[max_id]);
	window1 = cv::boundingRect(contours[max_id]);
	expand_window(window1, m_img.cols, m_img.rows);

	double dmx = (Mom.m10/Mom.m00);
	double dmy = (Mom.m01/Mom.m00);

    bool swap_targets = false;

	if(sqrt(((double)cm_x - dmx)*((double)cm_x - dmx) + ((double)cm_y - dmy)*((double)cm_y - dmy)) < (double)window1.width/3.0)
    {
        swap_targets=true;
    }

	// check if old point is inside new rect
	/*if((cm_x > window1.x && cm_x < window1.x+window1.width) && (cm_y > window1.y && cm_y < window1.x+window1.height))
	{

#ifdef DEBUG
		printf("New Point inside old Rect, move new target Point\n");
		#endif
		// try to factor out the old rectangle from target point
		const double dmx_old = window2.x + window2.width/2;
		const double dmy_old = window2.y + window2.height/2;

		const double dx = dmx - dmx_old;
		const double dy = dmy - dmy_old;

		dmx += dx*0.5;
		dmy += dy*0.5;
	}*/

	cm_x = (int)dmx;
	cm_y = (int)dmy;


	double dt_x = dmx;
	double dt_y = dmy;
	opencv_data_loader->transform_coords(dt_x, dt_y);

	y_t = (int)dt_y;
	x_t = (int)dt_x;

#ifdef DEBUG
	printf("found swimmer at (%d	%d)	(%d	%d)\n", cm_x, cm_y, x_t, y_t);
#endif

	cm_x2=-1;
	if(second_id != (size_t)-1)
	{
		cv::Moments Mom2 = cv::moments(contours[second_id], true);
		//cv::Rect rect2 = cv::boundingRect(contours[second_id]);
		window2 = cv::boundingRect(contours[second_id]);
		expand_window(window2, m_img.cols, m_img.rows);

		double dmx2 = (Mom2.m10/Mom2.m00);
		double dmy2 = (Mom2.m01/Mom2.m00);

		cm_x2 = (int)dmx2;
		cm_y2 = (int)dmy2;


		double dt_x2 = dmx2;
		double dt_y2 = dmy2+5;
		opencv_data_loader->transform_coords(dt_x2, dt_y2);

		y_t2 = (int)dt_y2;
		x_t2 = (int)dt_x2;

#ifdef DEBUG
		printf("second target at (%d	%d)	(%d	%d)\n", cm_x2, cm_y2, x_t2, y_t2);
#endif
		polylines( m_img_work, cv::Mat( contours[second_id] ), true, cv::Scalar(0,255,0)); // resize up the contour and draw
        if(swap_targets)
        {
			#ifdef DEBUG
            printf("New target close to old target, swapping targets\n");
			#endif
            std::swap(cm_x, cm_x2);
            std::swap(cm_y, cm_y2);


            std::swap(x_t, x_t2);
            std::swap(y_t, y_t2);

            std::swap(window1, window2);
        }

	}

	polylines( m_img_work, cv::Mat( contours[max_id] ), true, cv::Scalar(0,0,255)); // resize up the contour and draw
	//pBackSub->apply(m_img, m_img_work, 0.0);
	opencv_data_loader->save_to_disk(m_img_work, "cv_res2" + std::to_string(counter) + ".png", num_failure);
	opencv_data_loader->save_to_disk(m_img, "cv_test" + std::to_string(counter) + ".png", num_failure);
	return true;

}




bool swimmer_finder::CheckSwimmer(const cv::Rect window)
{

#ifdef DEBUG
	printf("Checking on swimmer\n");
#endif
	click_key4();
	opencv_bg_loader->get_image_data(m_img_reference, m_img_enemy);
	if(checkEnemy())
	{
		return false;
	}
	cv::GaussianBlur(m_img_reference, m_img_reference, cv::Size(5,5), 0, 0);

	m_img_reference = m_img_reference(window);
	opencv_bg_loader->save_to_disk(m_img_reference, "cv_vindow" + std::to_string(counter) + ".png", num_failure);

	//cv::cvtColor(m_img_reference, m_img_reference, cv::COLOR_BGR2GRAY);
	//m_img_work.convertTo(m_img_reference, CV_16U);


	auto start = std::chrono::high_resolution_clock::now();

	int num_iterations = 0;
	double avg_diff = 0.0;
	double avg_diff_sq = 0.0;
    bool print_image=true;
	while(true)
	{
		++num_iterations;
		auto now = std::chrono::high_resolution_clock::now();

		// Swimmer only lasts 30 seconds
		auto delta_start = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
		if(delta_start > 26000)
		{
			num_failure++;
#ifdef DEBUG
			printf("Stopping Search because 30s Swimmer duration are over\n");
#endif
			return false;
		}

		auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrame).count();
		if(delta < 33)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(33-delta));
		}

		lastFrame = now;
		click_key4();
		opencv_data_loader->get_image_data(m_img, m_img_enemy);
		if(checkEnemy())
		{
			return false;
		}
		m_img_work_tmp = m_img(window).clone();
		cv::cvtColor(m_img_work_tmp, m_img_work_tmp, cv::COLOR_BGR2HSV);

		// check of colors of swimmer are within the window
		int num_red_pixels = 0;
		int num_blue_pixels = 0;
		for(int j = 0; j <  m_img_work_tmp.rows; ++j)
		{
			for(int i = 0; i < m_img_work_tmp.cols; ++i)
			{

				cv::Vec3b pix = m_img_work_tmp.at<cv::Vec3b>(j,i);

				if(condition_blue(pix[0], pix[1], pix[2]))
				{
					++num_blue_pixels;
				}
				if(condition_red(pix[0], pix[1], pix[2]))
				{
					++num_red_pixels;
				}
			}
		}


		if((num_red_pixels == 0 || num_blue_pixels == 0) && num_iterations > 3)
		{
			num_failure++;
			opencv_data_loader->save_to_disk(m_img_work_tmp, "no_pixels_" + std::to_string(counter) + ".png", num_failure);
			printf("Stopping Search because no red or blue pixels were found in window: red=%d	blue=%d\n", num_red_pixels, num_blue_pixels);
			click_mouse();
			int wait = (int)human_reaction_dist(gen);
			std::this_thread::sleep_for(std::chrono::milliseconds(wait));
			return true;
		}

		//cv::cvtColor(m_img, m_img, cv::COLOR_BGR2GRAY);
		cv::GaussianBlur(m_img, m_img, cv::Size(5,5), 0, 0);
		m_img = m_img(window);

        if(print_image)
        {
			opencv_data_loader->save_to_disk(m_img, "cv_window" + std::to_string(counter) + ".png", num_failure);
        }


		//m_img_work.convertTo(m_img, CV_16U);
		cv::absdiff(m_img, m_img_reference, m_img_work);

        if(print_image)
        {
		opencv_data_loader->save_to_disk(m_img_work, "cv_window_res" + std::to_string(counter) + ".png", num_failure);
        print_image=false;
        }
		//cv::cvtColor(m_img_work, m_img_work, cv::COLOR_BGR2GRAY);
		

		// remove grey = remove water ripples
		for(int j=0; j<m_img_work.rows; ++j)
		{
			for(int i=0; i<m_img_work.cols; ++i)
			{
				cv::Vec3b & pix = m_img_work.at<cv::Vec3b>(j,i);
				// substract the mean color or minimum color from each pixel
				double dist = (pix[0] + pix[1] + pix[2]);
				int minc = std::min(std::min(pix[0], pix[1]), pix[2]);
				dist = dist/3;

				int mean = std::min((int)dist, minc);
				pix[0] = pix[0]-mean;
				pix[1] = pix[1]-mean;
				pix[2] = pix[2]-mean;

				dist = (pix[0] + pix[1] + pix[2]);
				dist = dist/3;
			}
		}
		//opencv_data_loader->save_to_disk(m_img_work, "cv_window_res2" + std::to_string(counter) + ".png");


		double score = cv::mean(m_img_work)[0];

		double diff = score;
		avg_diff += diff;
		avg_diff_sq += diff*diff;

		double sigma = sqrt(avg_diff_sq/(double)num_iterations - avg_diff*avg_diff/(double)(num_iterations*num_iterations));
		//printf("Searching at %d %d for %ld seconds diff = %.7e < %.7e %.7e	(%d	%d)\n", cm_x, cm_y, delta_start, diff, avg_diff/(double)num_iterations, sigma, num_red_pixels, num_blue_pixels);
		if(num_iterations > 60)
		{
		//if(fabs(diff-avg_diff/(double)num_iterations) > 3.415*sigma) // original, started to miss in cata
		if(fabs(diff-avg_diff/(double)num_iterations) > 3.0*sigma)
		{
			num_failure = 0;
			#ifdef DEBUG
			printf("Observed %d	%d pixels around (%d	%d\n", num_red_pixels, num_blue_pixels, cm_x, cm_y);
			printf("Check Swimmer returned with diff %.3e > %.3e %.3e\n", diff, avg_diff/(double)num_iterations, sigma);
#endif
			return false;
		}
		}
	}
}

void swimmer_finder::main_loop()
{


	const int blobber_duration = 10*60;
#ifdef OPEN_CLAMS
	const bool open_clamps = true;
#else
	const bool open_clamps = false;
#endif
	const bool login = false;
	const bool delay = false;

	std::tm te = {}; // only use when: delay=true;
	strptime("Jan 22 2020 19:03:15", "%b %d %Y %H:%M:%S", &te); // End time
	auto t_end = std::chrono::system_clock::from_time_t(std::mktime(&te));


	if(delay)
	{

		std::tm ts = {};
		 strptime("Jan 21 2020 10:01:38", "%b %d %Y %H:%M:%S", &ts); // Start time
		//strptime("Jan 7 2020 23:02:34", "%b %d %Y %H:%M:%S", &tm);
		auto t_start = std::chrono::system_clock::from_time_t(std::mktime(&ts));




		auto time_now = std::chrono::high_resolution_clock::now();



		while(time_now < t_start)
		{
			auto tmp = std::chrono::duration_cast<std::chrono::seconds>(t_start - time_now);
			std::string result = std::to_string(tmp.count());

			auto tmp2 = std::chrono::duration_cast<std::chrono::seconds>(t_end - time_now);
			std::string result2 = std::to_string(tmp2.count());

			std::time_t t = std::chrono::system_clock::to_time_t(t_start);

			int sleepy = (int)(5.0*clam_timer_dist(gen));
			std::time_t t2 = std::chrono::system_clock::to_time_t(t_end);
			std::cout << result << "	to start	at	" <<  std::ctime(&t) << "\n";
			std::cout <<  result2 << "	to end  	at	" << std::ctime(&t2) << std::endl;
			std::cout << "sleepy for " << sleepy << "s" << std::endl;

			std::this_thread::sleep_for(std::chrono::seconds(sleepy));

			sleepy = (int)wait_dist(gen);
			std::this_thread::sleep_for(std::chrono::milliseconds(sleepy));

			std::normal_distribution<double> disx((double)width/2.0, (double)width/5.0); //mean followed by stdiv
			std::normal_distribution<double> disy((double)height/2.0, (double)width/5.0); //mean followed by stdiv
			int x = (int)disx(gen);
			int y = (int)disy(gen) + height/2;
			y = std::min(height-72, std::max(273, y));
			x = std::min(width-144, std::max(256, x));

			mouse->set_target(x,y);
			mouse->move_mouse_to_direct();
			time_now = std::chrono::high_resolution_clock::now();
		}

	}



	if(login)
	{
		click_enter();
		std::this_thread::sleep_for(std::chrono::seconds(15));
		click_enter();
		std::this_thread::sleep_for(std::chrono::seconds(10));
		camera_zoom();
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	click_jump();
	int wait = ((int)wait_dist(gen));
	#ifdef DEBUG
	printf("Sleeping for %d milliseconds\n", wait);
#endif
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

	time_last_break = std::chrono::high_resolution_clock::now();

	click_key2();
	wait = 5100 + 2*((int)human_reaction_dist(gen));
	std::this_thread::sleep_for(std::chrono::milliseconds(wait));

#ifdef APPLY_BLOBBER
	time_last_blobber = std::chrono::high_resolution_clock::now();
#endif
	time_last_4 = std::chrono::high_resolution_clock::now();
	wait_key4 = (int)((2.0*wait_dist(gen)) + 7.0*human_reaction_dist(gen));
    num_enemy = 0;


	while(true)
	{

#ifdef DEBUG
		printf("num_failure = %d\n", num_failure);
#endif
		auto time_now = std::chrono::high_resolution_clock::now();
		if((t_end < time_now) && delay)
		{
			printf("End time reached\n");
			fflush(stdout);

			exit_wow();
			return;
		}

		if(num_failure % 3 == 0 && num_failure > 2)
		{
			camera_reset();


#ifdef APPLY_BLOBBER
#ifdef DEBUG
			printf("Trying to apply Blobber due to high failure rate\n");
#endif
			std::this_thread::sleep_for(std::chrono::seconds(1));
			click_key2();
			wait = 5100 + 2*((int)human_reaction_dist(gen));
			std::this_thread::sleep_for(std::chrono::milliseconds(wait));
			time_last_blobber = std::chrono::high_resolution_clock::now();
			std::this_thread::sleep_for(std::chrono::seconds(2));
			click_jump(); // To walk on water with Path of Frost
#endif

		}

		if(num_failure > 13)
		{
			//sync();
			//reboot(RB_POWER_OFF);
			printf("reached max failues\n");
			fflush(stdout);

			exit_wow();
			return;
		}

#ifdef DEBUG
		printf("clam timer = %d / %d\n", open_clam, open_clam_timer);

#endif
		if(open_clam_timer < 1)
			open_clam_timer = 1;
		if(open_clam_timer > 9)
			open_clam_timer = 9;
		if((open_clam > open_clam_timer) && open_clamps)
		{

			open_clam_timer = (int)clam_timer_dist(gen);

			click_key3();
			open_clam = 0;
			wait = 600 + (int)human_reaction_dist(gen);
			std::this_thread::sleep_for(std::chrono::milliseconds(wait));
		}

		auto now = std::chrono::high_resolution_clock::now();
		auto delta = std::chrono::duration_cast<std::chrono::seconds>(now - time_last_break).count();

		if(delta > 60*60)
		{
			wait = (int)human_reaction_dist(gen);
            wait = std::min(473, wait);
#ifdef DEBUG
		printf("Taking a break for %ds\n", wait);

#endif
		click_stealth();
		std::this_thread::sleep_for(std::chrono::seconds(wait/2));
		click_jump();
		wait = (int)human_reaction_dist(gen);
        wait = std::min(473, wait);
		std::this_thread::sleep_for(std::chrono::seconds(wait/2));
		click_jump();
		std::this_thread::sleep_for(std::chrono::seconds(1));
		time_last_break = std::chrono::high_resolution_clock::now();
		click_stealth();

		}

		now = std::chrono::high_resolution_clock::now();
		delta = std::chrono::duration_cast<std::chrono::seconds>(now - time_last_blobber).count();

#ifdef APPLY_BLOBBER
		if(delta > blobber_duration)
		{
#ifdef DEBUG
			printf("Applying Blobber\n");
#endif
			std::this_thread::sleep_for(std::chrono::seconds(1));
			click_key2();
			wait = 5100 + 2*((int)human_reaction_dist(gen));
			std::this_thread::sleep_for(std::chrono::milliseconds(wait));
			time_last_blobber = std::chrono::high_resolution_clock::now();
			std::this_thread::sleep_for(std::chrono::seconds(2));
			click_jump(); // To walk on water with Path of Frost
		}
#endif // ALLPY_BLOBBER

#ifdef DEBUG
		std::chrono::system_clock::time_point print_time = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(print_time);
		std::tm now_tm = *std::localtime(&now_c);

		char buff[70];
		strftime(buff, sizeof buff, "%A %c", &now_tm);
		printf("Running	%s\n", buff);
#endif

		BgFromDisplay();

		click_key();

		wait = 1200+(int)(wait_dist(gen));
		#ifdef DEBUG
		printf("Sleeping for %d milliseconds\n", wait);
#endif
		std::this_thread::sleep_for(std::chrono::milliseconds(wait));

		int x_target;
		int y_target;

		int x_target2;
		int y_target2;

		int cm_x2;
		int cm_y2;
		bool found = FindSwimmer(cm_x, cm_y, x_target, y_target,
								 cm_x2, cm_y2, x_target2, y_target2);

		if(found)
		{
			int x = x_target;
			int y = y_target;

			mouse->set_target(x,y);
			mouse->move_mouse_to();
			std::thread th(&mouse_mover::move_mouse_to, mouse);


			bool do_jump = CheckSwimmer(window1);

			th.join();

			wait = 300 + (int)human_reaction_dist(gen);
			std::this_thread::sleep_for(std::chrono::milliseconds(wait));
			if(!do_jump)
			{
				click_mouse();
				open_clam++;

				wait = ((int)human_reaction_dist(gen));
				std::this_thread::sleep_for(std::chrono::milliseconds(wait));
			}

			if(do_jump && cm_x2 > 0)
			{

                counter++;
#ifdef DEBUG
				printf("try second largest contour\n");
#endif
				int x = x_target2;
				int y = y_target2;

				mouse->set_target(x,y);
				mouse->move_mouse_to();
				std::thread th(&mouse_mover::move_mouse_to, mouse);

				do_jump = CheckSwimmer(window2);

				th.join();

				wait = 200 + (int)human_reaction_dist(gen);
				std::this_thread::sleep_for(std::chrono::milliseconds(wait));
				if(!do_jump)
				{
					click_mouse();
					open_clam++;
					cm_x = cm_x2;
					cm_y = cm_y2;
				}
			}
			else
			{
				std::swap(window1, window2); // store old window as window2
			}


			if(do_jump)
			{
				click_jump();
			}

			wait = 330 + ((int)human_reaction_dist(gen));
			std::this_thread::sleep_for(std::chrono::milliseconds(wait));

			std::normal_distribution<double> disx((double)width/2.0, 317.0); //mean followed by stdiv
			std::normal_distribution<double> disy((double)height/4.0, 154.0); //mean followed by stdiv
			x = (int)disx(gen);
			y = (int)disy(gen) + height/2;
			y = std::min(height-72, std::max(273, y));
			x = std::min(width-144, std::max(256, x));

			mouse->set_target(x,y);
			//mouse->move_mouse_to_direct();
			std::thread th2(&mouse_mover::move_mouse_to_direct, mouse);

			wait = ((int)human_reaction_dist(gen))/4;
			std::this_thread::sleep_for(std::chrono::milliseconds(wait));
			th2.join();

		}
	}
	return;
}

