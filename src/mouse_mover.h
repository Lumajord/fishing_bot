#ifndef MOUSE_MOVER_H
#define MOUSE_MOVER_H

#include <chrono>
#include <cmath>
#include <random>

#include <X11/extensions/XTest.h>
#include <unistd.h>
#include "mouse_mover.h"

#include "gjrand.h"

class mouse_mover
{
private:
	gjrand64 gen;	//Standard mersenne_twister_engine seeded with rd()
	std::uniform_int_distribution<> rand12;
	std::uniform_int_distribution<> rand02;
	std::normal_distribution<double> end_to_end_dist;
	std::normal_distribution<double> acceleration_dist;
	std::normal_distribution<double> random_vel;

	int POLLING_RATE; // polling rate of the mouse
	double dt; // timestep in ms
	double simulation_time;

	std::chrono::system_clock::time_point lastFrame;

	int width;
	int height;

	double max_speed; // mouse speed in pixels / ms
	double min_speed;
	double max_acceleration;

	double accuracy_hand;
	double accuracy_finger;
	double accuracy_final;

	double vx;
	double vy;

	double x;
	double y;

	double xt;
	double yt;

	mouse_mover *mouse;

	Display *dpy;
	XEvent event;

	bool euler(const double dt,
			const double x_target,
			const double y_target,
			bool final_target,
			bool finger_movement);
	void integrate_dt(const double x_target, const double y_target, const double accuracy, bool finger_movement, bool final_step);

public:
	mouse_mover(Display *dpy_);
	void set_target(const int x_target, const int y_target);
	void move_mouse_to();
	void move_mouse_to_direct();
};

#endif // MOUSE_MOVER_H
