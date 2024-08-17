#ifndef __ARQSIM_HEADER_ARCH_H__
#define __ARQSIM_HEADER_ARCH_H__

#include <array>
#include <memory>
#include <list>

#include <cstdint>

#include <my-lib/std.h>
#include <my-lib/macros.h>

#include "config.h"
#include "arch-lib.h"

namespace Arch {

// ---------------------------------------

class Terminal;
class Disk;
class Timer;
class Memory;
class Cpu;

class Computer
{
private:
	std::list<Device*> devices;
	std::array<IO_Device*, 1 << 16> io_ports;
	std::unique_ptr<Terminal> terminal;
	std::unique_ptr<Disk> disk;
	std::unique_ptr<Timer> timer;
	std::unique_ptr<Memory> memory;
	std::unique_ptr<Cpu> cpu;

	bool alive = true;
	uint64_t cycle = 0;

	std::string turn_off_msg;

	inline static Computer *computer = nullptr;

private:
	Computer ();

public:
	static void init ()
	{
		if (computer == nullptr)
			computer = new Computer;
	}

	static Computer& get ()
	{
		return *computer;
	}

	static void destroy ()
	{
		if (computer != nullptr)
			delete computer;
		computer = nullptr;
	}

	void run ();

	inline Terminal& get_terminal () const
	{
		return *this->terminal;
	}

	inline Disk& get_disk () const
	{
		return *this->disk;
	}

	inline Timer& get_timer () const
	{
		return *this->timer;
	}

	inline Memory& get_memory () const
	{
		return *this->memory;
	}

	inline Cpu& get_cpu () const
	{
		return *this->cpu;
	}

	inline void set_io_port (const uint16_t port, IO_Device *device)
	{
		mylib_assert_exception(port < this->io_ports.size())
		this->io_ports[port] = device;
	}

	inline void set_io_port (const IO_Port port, IO_Device *device)
	{
		this->set_io_port(std::to_underlying(port), device);
	}

	inline IO_Device& get_io_port (const uint16_t port) const
	{
		mylib_assert_exception(port < this->io_ports.size())
		mylib_assert_exception(this->io_ports[port] != nullptr)
		return *this->io_ports[port];
	}

	inline IO_Device& get_io_port (const IO_Port port) const
	{
		return this->get_io_port(std::to_underlying(port));
	}
};

// ---------------------------------------

} // end namespace

#endif