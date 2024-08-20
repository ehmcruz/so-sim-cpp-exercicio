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
			VmemPageFault             = 0,
			VmemGPFnotReadable        = 1,
			VmemGPFnotWritable        = 2,
			VmemGPFnotExecutable      = 3,
			InvalidInstruction        = 4,
		};
		Type type;
		uint16_t vaddr;
	};

	enum class PteFieldPos : uint16_t {
		PhyFrameID   = 0,
		Present      = 12,
		Readable     = 13,
		Writable     = 14,
		Executable   = 15,
		Dirty        = 16,
		Accessed     = 17,
		Foo          = 18,
	};

	enum class PteFieldSize : uint16_t {
		PhyFrameID   = 12,
		Present      = 1,
		Readable     = 1,
		Writable     = 1,
		Executable   = 1,
		Dirty        = 1,
		Accessed     = 1,
		Foo          = 14,
	};

	using PageTableEntry = Mylib::BitSet<32>;
	using PageTable = std::array<PageTableEntry, Config::ptes_per_table>;

private:
	using Instruction = Mylib::BitSet<16>;

	std::array<uint16_t, Config::nregs> gprs;
	InterruptCode interrupt_code;
	bool has_interrupt = false;
	uint16_t backup_pc;

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
	void execute_r (const Instruction instruction);
	void execute_i (const Instruction instruction);

	uint16_t vmem_to_phys (const uint16_t vaddr, const MemAccessType access_type);

	inline uint16_t vmem_read_instruction (const uint16_t vaddr)
	{
		const uint16_t paddr = this->vmem_to_phys(vaddr, MemAccessType::Execute);
		return this->pmem_read(paddr);
	}

	inline uint16_t vmem_read (const uint16_t vaddr)
	{
		const uint16_t paddr = this->vmem_to_phys(vaddr, MemAccessType::Read);
		return this->pmem_read(paddr);
	}

	inline void vmem_write (const uint16_t vaddr, const uint16_t value)
	{
		const uint16_t paddr = this->vmem_to_phys(vaddr, MemAccessType::Write);
		this->pmem_write(paddr, value);
	}
};

// ---------------------------------------

} // end namespace

#endif