#ifndef __ARQSIM_HEADER_ARCH_CPU_H__
#define __ARQSIM_HEADER_ARCH_CPU_H__

#include <array>

#include <my-lib/std.h>
#include <my-lib/macros.h>
#include <my-lib/bit.h>

#include "../config.h"
#include "arch-lib.h"
#include "memory.h"
#include "arch.h"

namespace Arch {

// ---------------------------------------

class Cpu : public Device
{
private:
	std::array<uint16_t, Config::nregs> gprs;
	InterruptCode interrupt_code;
	bool has_interrupt = false;

	OO_ENCAPSULATE_SCALAR(uint16_t, pc)
	OO_ENCAPSULATE_SCALAR_INIT(uint16_t, vmem_paddr_init, 0)
	OO_ENCAPSULATE_SCALAR_INIT(uint16_t, vmem_paddr_end, Config::memsize_words-1)

	OO_ENCAPSULATE_SCALAR_INIT_READONLY(uint16_t, pmem_size_words, Config::memsize_words)

public:
	Cpu (Computer& computer);
	~Cpu ();

	void run_cycle () override final;
	void dump () const;

	inline uint16_t get_gpr (const uint8_t code) const
	{
		mylib_assert_exception(code < this->gprs.size())
		return this->gprs[code];
	}

	inline void set_gpr (const uint8_t code, const uint16_t v)
	{
		mylib_assert_exception(code < this->gprs.size())
		this->gprs[code] = v;
	}

	inline uint16_t pmem_read (const uint16_t paddr) const
	{
		return this->computer.get_memory()[paddr];
	}

	inline void pmem_write (const uint16_t paddr, const uint16_t value)
	{
		this->computer.get_memory()[paddr] = value;
	}

	inline uint16_t read_io (const uint16_t port)
	{
		return this->computer.get_io_port(port).read(port);
	}

	inline uint16_t read_io (const IO_Port port)
	{
		return this->read_io(std::to_underlying(port));
	}

	inline void write_io (const uint16_t port, const uint16_t value)
	{
		this->computer.get_io_port(port).write(port, value);
	}

	inline void write_io (const IO_Port port, const uint16_t value)
	{
		this->write_io(std::to_underlying(port), value);
	}

	bool interrupt (const InterruptCode interrupt_code);
	void force_interrupt (const InterruptCode interrupt_code);
	void turn_off ();

private:
	void execute_r (const Mylib::BitSet<16> instruction);
	void execute_i (const Mylib::BitSet<16> instruction);

	inline uint16_t vmem_read (const uint16_t vaddr)
	{
		const uint16_t paddr = vaddr + this->vmem_paddr_init;

		if (paddr > this->vmem_paddr_end) {
			this->force_interrupt(InterruptCode::GPF);
			return 0;
		}

		return this->pmem_read(paddr);
	}

	inline void vmem_write (const uint16_t vaddr, const uint16_t value)
	{
		const uint16_t paddr = vaddr + this->vmem_paddr_init;

		if (paddr > this->vmem_paddr_end) {
			this->force_interrupt(InterruptCode::GPF);
			return;
		}

		this->pmem_write(paddr, value);
	}
};

// ---------------------------------------

} // end namespace

#endif