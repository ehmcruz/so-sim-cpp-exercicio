#include <cstdint>
#include <cstdlib>

#include <signal.h>

#include "config.h"
#include "lib.h"
#include "arch/computer.h"
#include "arch/terminal.h"
#include "os/os.h"

// ---------------------------------------

static void interrupt_handler (int dummy)
{
	endwin();
	std::exit(1);
}

int main (int argc, char **argv)
{
	signal(SIGINT, interrupt_handler);

	// ncurses start
	initscr();
	timeout(0); // non-blocking input
	noecho(); // don't print input

	Arch::Computer::init();
	OS::boot(&Arch::Computer::get().get_cpu());
	Arch::Computer::get().run();

	endwin();

	// print kernel msgs
	Arch::Computer::get().get_terminal().dump(Arch::Terminal::Type::Kernel);
	std::cout << std::endl;

	Arch::Computer::destroy();

	return 0;
}