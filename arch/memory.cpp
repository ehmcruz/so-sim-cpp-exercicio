#include "memory.h"

// ---------------------------------------

namespace Arch {

// ---------------------------------------

Memory::Memory (Computer& computer)
	: Device(computer)
{
	for (auto& v: this->data)
		v = 0;
}

Memory::~Memory ()
{
	
}

void Memory::run_cycle ()
{	
}

void Memory::dump (const uint16_t init, const uint16_t end) const
{
	terminal_println(Arch, "memory dump from paddr " << init << " to " << end)
	for (uint16_t i = init; i < end; i++)
		terminal_print(Arch, this->data[i] << " ")
	terminal_println(Arch, "")
}

// ---------------------------------------

} // end namespace