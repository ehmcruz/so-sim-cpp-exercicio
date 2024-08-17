#include "arch.h"

// ---------------------------------------

namespace Arch {

// ---------------------------------------

Computer::Computer ()
{
	for (auto& port: this->io_ports)
		port = nullptr;
	
	this->terminal = std::make_unique<Terminal>();
	this->disk = std::make_unique<Disk>();
	this->timer = std::make_unique<Timer>();
	this->memory = std::make_unique<Memory>();
	this->cpu = std::make_unique<Cpu>();

	this->devices.push_back(this->terminal.get());
	this->devices.push_back(this->disk.get());
	this->devices.push_back(this->timer.get());
	this->devices.push_back(this->memory.get());
	this->devices.push_back(this->cpu.get());
}

void Computer::run ()
{
	while (this->alive) {
		for (auto *device: this->devices)
			device->run_cycle();
		this->cycle++;
	}
}

// ---------------------------------------

} // end namespace