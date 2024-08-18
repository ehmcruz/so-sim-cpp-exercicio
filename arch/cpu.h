#ifndef __ARQSIM_HEADER_ARCH_CPU_H__
#define __ARQSIM_HEADER_ARCH_CPU_H__

#include <array>

#include <my-lib/std.h>
#include <my-lib/macros.h>
#include <my-lib/bit.h>

#include "../config.h"
#include "device.h"
#include "memory.h"
#include "computer.h"

namespace Arch {

// ---------------------------------------

class Cpu : public Device
{
public:
	enum VmemMode : uint16_t {
		Disabled       = 0,
		BaseLimit      = 1,
		Paging         = 2
	};

	enum class MemAccessType : uint16_t {
		Execute        = 0,
		Read           = 1,
		Write          = 2,
	};

	struct CpuException {
		enum class Type : uint16_t {
			VmemGPF           = 0,
			VmemPageFault     = 1
		};
		Type type;
		uint16_t vaddr;
	};

	struct PageTableEntry {
		enum class Flags : uint16_t {
			Readable   = 0,
			Writable   = 1,
			Executable = 2,
			Dirty      = 3,
			Accessed   = 4,
			Present    = 5
		};
		uint16_t paddr;
		Mylib::BitSet<16> flags;
	};

	using PageTable = std::array<PageTableEntry, Config::ptes_per_table>;

private:
	std::array<uint16_t, Config::nregs> gprs;
	InterruptCode interrupt_code;
	bool has_interrupt = false;

	OO_ENCAPSULATE_SCALAR(uint16_t, pc)
	OO_ENCAPSULATE_SCALAR_INIT(VmemMode, vmem_mode, VmemMode::Disabled)
	OO_ENCAPSULATE_SCALAR_INIT(uint16_t, vmem_paddr_init, 0)
	OO_ENCAPSULATE_SCALAR_INIT(uint16_t, vmem_paddr_end, Config::phys_mem_size_words-1)
	OO_ENCAPSULATE_PTR_INIT(PageTable*, page_table, nullptr)
	OO_ENCAPSULATE_OBJ_READONLY(CpuException, cpu_exception)

	OO_ENCAPSULATE_SCALAR_INIT_READONLY(uint16_t, pmem_size_words, Config::phys_mem_size_words)

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

	inline uint16_t vmem_to_phys (const uint16_t vaddr)
	{
		uint16_t paddr;

		switch (this->vmem_mode) {
			case VmemMode::Disabled:
				paddr = vaddr;
				break;

			case VmemMode::BaseLimit:
				paddr = vaddr + this->vmem_paddr_init;

				if (paddr > this->vmem_paddr_end) {
					throw CpuException {
						.type = CpuException::Type::VmemGPF,
						.vaddr = vaddr
						};
				}
				break;

			case VmemMode::Paging:
				mylib_throw_exception_msg("Paging not implemented");
				break;
		}

		return paddr;
	}

	inline uint16_t vmem_read (const uint16_t vaddr)
	{
		const uint16_t paddr = this->vmem_to_phys(vaddr);
		return this->pmem_read(paddr);
	}

	inline void vmem_write (const uint16_t vaddr, const uint16_t value)
	{
		const uint16_t paddr = this->vmem_to_phys(vaddr);
		this->pmem_write(paddr, value);
	}
};

// ---------------------------------------

} // end namespace

#endif