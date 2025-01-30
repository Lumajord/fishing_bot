
#include "swimmer_finder.h"
#include <X11/Xlib.h>
#include <thread>

#include <unistd.h>

int main (int argc, char* argv[])
{


	(void) argc;
	(void) argv;
	swimmer_finder angler;
	angler.main_loop();

	return 0;
} /* main() */
