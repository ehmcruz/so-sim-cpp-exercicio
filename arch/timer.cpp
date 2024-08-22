#include "timer.h"
#include "computer.h"
#include "cpu.h"


// ---------------------------------------

namespace Arch {

// ---------------------------------------

Timer::Timer (Computer& computer)
	: IO_Device(computer)
{
	this->computer.set_io_port(IO_Port::TimerInterruptCycles, this);
}

void Timer::run_cycle ()
{
	if (this->count >= this->timer_interrupt_cycles) {
		if (this->computer.get_cpu().interrupt(InterruptCode::Timer))
			this->count = 0;
	}
	else
		this->count++;
}

uint16_t Timer::read (const uint16_t port)
{
	const IO_Port port_enum = static_cast<IO_Port>(port);
	uint16_t r;

	switch (port_enum) {
		using enum IO_Port;

		case TimerInterruptCycles:
			r = this->timer_interrupt_cycles;
		break;

		default:
			mylib_throw_exception_msg("Timer read invalid port ", port);
	}

	return r;
}

void Timer::write (const uint16_t port, const uint16_t value)
{
	const IO_Port port_enum = static_cast<IO_Port>(port);

	switch (port_enum) {
		using enum IO_Port;

		case TimerInterruptCycles:
			this->timer_interrupt_cycles = value;
		break;

		default:
			mylib_throw_exception_msg("Timer write invalid port ", port);
	}
}

// ---------------------------------------

} // end namespace