#include "mouse_mover.h"

#include <X11/extensions/XTest.h>
#include <unistd.h>
#include <cmath>
#include <thread>

#include "randutils.h"

static int clamp(int x, const int low, const int high)
{
	return std::min(high, std::max(x, low));
}

mouse_mover::mouse_mover(Display* dpy_)
{


	dpy = dpy_;
	XWindowAttributes attributes;
	Window root = DefaultRootWindow(dpy);
	XGetWindowAttributes(dpy, root, &attributes);

	width=attributes.width;
	height = attributes.height;


	uint32_t seeds[4];
	randutils::auto_seed_256 seeder;
	seeder.generate(seeds, seeds+4);
	uint64_t seed1;
	uint64_t seed2;
	seed1 = (uint64_t)seeds[0] << 32 | seeds[1];
	seed2 = (uint64_t)seeds[2] << 32 | seeds[3];

	gen = gjrand64(seed1, seed2);
	rand12 = std::uniform_int_distribution<> (1,2);
	rand02 = std::uniform_int_distribution<> (0,0);
	end_to_end_dist = std::normal_distribution<double> (170.0, 15.0);
	acceleration_dist = std::normal_distribution<double> (72.0, 5.0);
	random_vel = std::normal_distribution<double> (1.0, 0.1);

	POLLING_RATE = 500; // polling rate of the mouse
	dt = 1000.0/(double)POLLING_RATE; // timestep in ms

	simulation_time = 0.0;

	const double  resolution_factor = (double)width / (double)2560;
	accuracy_hand = 200.0*resolution_factor;
	accuracy_finger = 50.0*resolution_factor;
	accuracy_final = 3.0*resolution_factor;

	vx = 0.0;
	vy = 0.0;

	x = 0.0;
	y = 0.0;

	xt = 0.0;
	yt = 0.0;


	const double time_end_to_end = 376.0; // estimated time to move mouse from one end of the monitor to the other
	max_speed = (double)width / time_end_to_end; // mouse speed in pixels / ms
	min_speed = max_speed / 20.0;
	const double acceleration_time = 109.0;
	max_acceleration = max_speed	/ acceleration_time;

}

static double pow2(const double x)
{
	return x*x;
}

/*static double pow5(const double x)
{
	return x*x * x*x * x;
}*/


static void rotate(double &x, double &y, const double phi)
{
	const double x_tmp = x;
	const double y_tmp = y;

	x = x_tmp*std::cos(phi) - y_tmp * std::sin(phi);
	y = y_tmp*std::cos(phi) + x_tmp	 * std::sin(phi);
}

static double get_acceleration(const double max_acceleration, const double max_speed, const double current_speed)
{
	double a = max_acceleration * sqrt(1.0 - std::min(1.0, pow2(current_speed / max_speed)));
	return a;
}


bool mouse_mover::euler(
		const double dt,
		const double x_target,
		const double y_target,
		bool final_target,
		bool finger_movement)
{



	const double del_x = x_target - x;
	const double del_y = y_target - y;

	const double r = sqrt(pow2(del_x) + pow2(del_y));

	const double e_x = del_x / r;
	const double e_y = del_y / r;

	double vel = sqrt(pow2(vx) + pow2(vy));

	if(vel == 0.0)
		vel = 1.0;

	const double e_vx = vx/vel;
	const double e_vy = vy/vel;


	const double s = 0.5 * (pow2(vx) + pow2(vy)) / max_acceleration;
	const double x_stop = x + s*e_vx;
	const double y_stop = y + s*e_vy;

	if(pow2(x_stop - x_target) + pow2(y_stop - y_target) < pow2(accuracy_final)*0.8 && final_target)
	{

		x = x + vx*dt - max_acceleration * pow2(dt) * e_vx;
		y = y + vy*dt - max_acceleration * pow2(dt) * e_vy;

		vx = vx - max_acceleration * dt * e_vx;
		vy = vy - max_acceleration * dt * e_vy;

		simulation_time += dt;

		if(pow2(x - x_target) + pow2(y - y_target) < pow2(accuracy_final))
		{
			//printf("exit target reached\n");
			return true;
		}
	}
	else
	{

		const double correlation = e_x*e_vx + e_y*e_vy;
		double local_max_accel = 2.0*max_acceleration;


		double phi = 0.0;
		if(finger_movement)
		{

			//local_max_accel *= 2.0;
			if(vel > 0.01*max_speed && correlation < 0.89)
			{
			x = x + vx*dt - local_max_accel * dt*dt * e_vx;
			y = y + vy*dt - local_max_accel * dt*dt * e_vy;

			vx = vx - local_max_accel * dt * e_vx;
			vy = vy - local_max_accel * dt * e_vy;

			simulation_time += dt;
			}
			else
			{


				const double a = get_acceleration(local_max_accel, max_speed, vel);

				x = x + vx*dt + a * dt*dt * e_x;
				y = y + vy*dt + a * dt*dt * e_y;

				vx = vx + a * dt * e_x;
				vy = vy + a * dt * e_y;

				simulation_time+=dt;

				vel = sqrt(pow2(vx) + pow2(vy));
				const double a_steer = max_acceleration * (1.0 - (correlation)) *(vel / max_speed);
				phi = std::asin(a_steer * dt / vel);

				rotate(vx, vy, phi);


			}
		}
		else
		{

			const double a = get_acceleration(local_max_accel, max_speed, vel);

			x = x + vx*dt + a * dt*dt * e_x;
			y = y + vy*dt + a * dt*dt * e_y;

			vx = vx + a * dt * e_x;
			vy = vy + a * dt * e_y;

			simulation_time+=dt;

			vel = sqrt(pow2(vx) + pow2(vy));
			const double a_steer = max_acceleration * (1.0 - (correlation)) *sqrt(vel / max_speed);
			phi = std::asin(a_steer * dt / vel);


			rotate(vx, vy, phi);

			vx = vx * (1.0 - 0.15*a_steer/max_acceleration);
			vy = vy * (1.0 - 0.15*a_steer/max_acceleration);


			const double del_x = x_target - x;
			const double del_y = y_target - y;

			const double e_x = del_x / r;
			const double e_y = del_y / r;

			vel = sqrt(pow2(vx) + pow2(vy));

			if(vel == 0.0)
				vel = 1.0;

			double e_vx = vx/vel;
			double e_vy = vy/vel;

			const double correlation_new = e_x*e_vx + e_y*e_vy;
			if(correlation_new < 0.95*correlation)
			{
				//printf("return hand corr	%.5e->%.5e	(%.5e	%.5e)	(%.5e	%.5e)\n", correlation, correlation_new, x,y,vx,vy);
				return true;
			}

			if(sqrt(pow2(xt-x) + pow2(yt -y)) < r)
			{
				//printf("return finger dist\n");
				return true;
			}


		}
	}

	return false;
}


void mouse_mover::integrate_dt(const double x_target, const double y_target, const double accuracy, bool finger_movement, bool final_step)
{
	const int N_super = 2400;
	const double super_dt = dt/(double)N_super;
	double dist = sqrt(pow2(x - x_target) + pow2(y-y_target));

	while(dist > accuracy)
	{
	for(int i = 0; i < N_super; ++i)
	{

		dist = sqrt(pow2(x - x_target) + pow2(y-y_target));
		bool finished = euler(super_dt, x_target, y_target, final_step, finger_movement);

		simulation_time += super_dt;


		if((simulation_time > dt) || (finished && final_step))
		{
			simulation_time -= dt;

			vx = vx*random_vel(gen);
			vy = vy*random_vel(gen);
			auto now = std::chrono::high_resolution_clock::now();
			auto delta = std::chrono::duration_cast<std::chrono::microseconds>(now - lastFrame).count();

			if(delta < 10000)
			{
				std::this_thread::sleep_for(std::chrono::microseconds(10000-delta));
			}

			lastFrame = std::chrono::high_resolution_clock::now();

			int x_position = clamp((int)x, 0, width);
			int y_position = clamp((int)y, 0, height);
			XTestFakeMotionEvent(dpy, 0, x_position, y_position, CurrentTime);
			XSync(dpy, 0);
		}



		if(finished)
		{
			return;
		}
	}
	}
}




void mouse_mover::set_target(const int x_target, const int y_target)
{

	printf("mouse_mover setting taget to (%d	%d)\n", x_target, y_target);
	xt = (double)x_target;
	yt = (double)y_target;
	return;
}


void mouse_mover::move_mouse_to()
{


	//printf("mouse_mover starts moving mouse\n");
	const double time_end_to_end = end_to_end_dist(gen); // estimated time to move mouse from one end of the monitor to the other
	max_speed = 6.0*(double)width / time_end_to_end; // mouse speed in pixels / ms
	const double acceleration_time = acceleration_dist(gen);
	max_acceleration = 3.0*max_speed	/ acceleration_time;

	XQueryPointer (dpy, RootWindow (dpy, 0), &event.xbutton.root,
	 &event.xbutton.window, &event.xbutton.x_root,
	 &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y,
	 &event.xbutton.state);


	lastFrame = std::chrono::high_resolution_clock::now();

	x =  event.xbutton.x;
	y =  event.xbutton.y;

	double del_x = xt - x;
	double del_y = yt - y;

	double r = sqrt(pow2(del_x) + pow2(del_y));


	vx = 0.01*max_speed*del_x/r;
	vy = 0.01*max_speed*del_y/r;

	const int mid_points = rand12(gen);
	bool finger_movement = false;
	for (int i = 0; i < mid_points; ++i)
	{



		del_x = xt - x;
		del_y = yt - y;

		r = sqrt(pow2(del_x) + pow2(del_y));

		//const double x_midp = r/(double(mid_points+1)) + 0.5*r*(double)i;
		const double t_midp = 0.5;
		const double x_spread = 0.15*r/(double(i*3+1));
		std::normal_distribution<> dy{0.0 ,x_spread};


reroll:
		double x_i = x + t_midp*del_x + dy(gen);
		double y_i = y + t_midp*del_y + dy(gen);


		if(x < xt)
		{
			if(x > x_i)
			{
				goto reroll;
			}
		}

		if(x > xt) // if x is greater than x_target we move backwards thus x_i must be smaller than x
		{
			if(x < x_i)
			{
				goto reroll;
			}
		}

		if(x < xt) // if x is smaller than x_target we move forwards thus x_i must be greager than x
		{
			if(x > x_i)
			{
				goto reroll;
			}
		}


		if(y > yt) // if y is greater than y_target we move backwards thus y_i must be smaller than y
		{
			if(y < y_i)
			{
				goto reroll;
			}
		}

		if(y < yt) // if y is smaller than y_target we move forwards thus y_i must be greager than y
		{
			if(y > y_i)
			{
				goto reroll;
			}
		}

		integrate_dt(x_i, y_i, accuracy_hand, finger_movement, false);

	}


	finger_movement = true;
	const int end_points = rand02(gen);
	for (int i = 0; i < end_points; ++i)
	{

		const double spread = 10.0;
		std::normal_distribution<> dx{0.0,spread/(double(i*2+1))};

		double x_i = xt + dx(gen);
		double y_i = yt + dx(gen);

		integrate_dt(x_i, y_i, accuracy_finger, finger_movement, false);

	}


	max_speed /= 10.0;
		integrate_dt(xt, yt, accuracy_final, finger_movement, true);

}


void mouse_mover::move_mouse_to_direct()
{


	const double time_end_to_end = end_to_end_dist(gen); // estimated time to move mouse from one end of the monitor to the other
	max_speed = 10.0*(double)width / time_end_to_end; // mouse speed in pixels / ms
	const double acceleration_time = acceleration_dist(gen);
	max_acceleration = 3.0*max_speed	/ acceleration_time;

	XQueryPointer (dpy, RootWindow (dpy, 0), &event.xbutton.root,
	 &event.xbutton.window, &event.xbutton.x_root,
	 &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y,
	 &event.xbutton.state);


	lastFrame = std::chrono::high_resolution_clock::now();

	x =  event.xbutton.x;
	y =  event.xbutton.y;

	double del_x = xt - x;
	double del_y = yt - y;

	double r = sqrt(pow2(del_x) + pow2(del_y));


	vx = 0.01*max_speed*del_x/r;
	vy = 0.01*max_speed*del_y/r;

	const int mid_points = rand12(gen);
	bool finger_movement = false;
	for (int i = 0; i < mid_points; ++i)
	{


		del_x = xt - x;
		del_y = yt - y;

		r = sqrt(pow2(del_x) + pow2(del_y));

		//const double x_midp = r/(double(mid_points+1)) + 0.5*r*(double)i;
		const double t_midp = 0.5;
		const double x_spread = 0.15*r/(double(i*3+1));
		std::normal_distribution<> dy{0.0 ,x_spread};


reroll:
		double x_i = x + t_midp*del_x + dy(gen);
		double y_i = y + t_midp*del_y + dy(gen);


		if(x < xt)
		{
			if(x > x_i)
			{
				goto reroll;
			}
		}

		if(x > xt) // if x is greater than x_target we move backwards thus x_i must be smaller than x
		{
			if(x < x_i)
			{
				goto reroll;
			}
		}

		if(x < xt) // if x is smaller than x_target we move forwards thus x_i must be greager than x
		{
			if(x > x_i)
			{
				goto reroll;
			}
		}


		if(y > yt) // if y is greater than y_target we move backwards thus y_i must be smaller than y
		{
			if(y < y_i)
			{
				goto reroll;
			}
		}

		if(y < yt) // if y is smaller than y_target we move forwards thus y_i must be greager than y
		{
			if(y > y_i)
			{
				goto reroll;
			}
		}

		integrate_dt(x_i, y_i, accuracy_hand, finger_movement, false);

	}


	finger_movement = true;

	max_speed /= 10.0;
		integrate_dt(xt, yt, accuracy_finger, finger_movement, true);

}




