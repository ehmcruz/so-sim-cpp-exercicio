#include "timer.h"

// ---------------------------------------

namespace Arch {

// ---------------------------------------

Timer::Timer (Computer& computer)
	: IO_Device(computer)
{
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
	const Config::IO_Ports port_enum = static_cast<Config::IO_Ports>(port);

	switch (port_enum) {
		using enum Config::IO_Ports;

		case TimerInterruptCycles:
			return this->timer_interrupt_cycles;

		default:
			mylib_throw_exception_msg("Timer read invalid port ", port);
	}
}

void Timer::write (const uint16_t port, const uint16_t value)
{
	const Config::IO_Ports port_enum = static_cast<Config::IO_Ports>(port);

	switch (port_enum) {
		using enum Config::IO_Ports;

		case TimerInterruptCycles:
			this->timer_interrupt_cycles = value;

		default:
			mylib_throw_exception_msg("Timer read invalid port ", port);
	}
}

// ---------------------------------------

} // end namespace