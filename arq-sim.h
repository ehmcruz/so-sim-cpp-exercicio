#ifndef __ARQSIM_HEADER_ARQSIM_H__
#define __ARQSIM_HEADER_ARQSIM_H__

#include <array>
#include <vector>
#include <list>
#include <string>
#include <string_view>
#include <fstream>
#include <utility>
#include <memory>

#include <cstdint>

#if defined(CONFIG_TARGET_LINUX)
	#include <ncurses.h>
#elif defined(CONFIG_TARGET_WINDOWS)
	#include <ncurses/ncurses.h>
#else
	#error Untested platform
#endif

#include <my-lib/std.h>
#include <my-lib/macros.h>
#include <my-lib/matrix.h>
#include <my-lib/bit.h>

#include "config.h"
#include "lib.h"

namespace Arch {

// ---------------------------------------

enum class InterruptCode : uint16_t
{
	Keyboard,
	Disk,
	Timer,
	GPF
};

const char* InterruptCode_str (const InterruptCode code);

// ---------------------------------------

class Computer;

class Device
{
protected:
	Computer& computer;

public:
	Device (Computer& computer)
		: computer(computer)
	{
	}

	virtual void run_cycle () = 0;
};

// ---------------------------------------

class IO_Device : public Device
{
public:
	IO_Device (Computer& computer)
		: Device(computer)
	{
	}

	virtual uint16_t read (const uint16_t port) = 0;
	virtual void write (const uint16_t port, const uint16_t value) = 0;
};

// ---------------------------------------

class VideoOutput
{
private:
	using MatrixBuffer = Mylib::Matrix<char, true>;

	WINDOW *win;

	MatrixBuffer buffer;

	// cursor position in the buffer
	uint32_t x;
	uint32_t y;

public:
	VideoOutput (const uint32_t xinit, const uint32_t xend, const uint32_t yinit, const uint32_t yend);
	~VideoOutput ();

	void print (const std::string_view str);
	void dump () const;

private:
	void roll ();
	void update ();
};

// ---------------------------------------

class Terminal : public IO_Device
{
public:
	enum class Type : uint16_t {
		Arch,
		Kernel,
		Command,
		App,

		Count // must be the last one
	};

private:
	std::vector<VideoOutput> videos;
	uint16_t typed_char;
	bool has_char = false;
	Type current_video = Type::Arch;

public:
	Terminal (Computer& computer);
	~Terminal ();

	void run_cycle () override final;
	uint16_t read (const uint16_t port) override final;
	void write (const uint16_t port, const uint16_t value) override final;

	void dump (const Type video) const
	{
		this->videos[ std::to_underlying(video) ].dump();
	}
};

// ---------------------------------------

class Disk : public IO_Device
{
public:
	enum class Cmd : uint16_t {
		SetFnameStart      = 0,
		SetFnameEnd        = 1,
		OpenFile           = 2,
		CloseFile          = 3,
		ReadFile           = 4,
		GetFileSize        = 5,
	};

private:
	enum class State {
		Idle,
		WaitingFname,
		Reading
	};

	struct FileDescriptor {
		std::fstream file;
		std::string fname;
	};

	struct Job {
		enum class Type {
			Read
		};

		Type type;
		Descriptor descriptor;
		uint64_t sector;
		std::vector<uint8_t> buffer;
	};

private:
	std::vector<std::unique_ptr<FileDescriptor>> file_descriptors;
	OO_ENCAPSULATE_SCALAR_CONST_INIT_READONLY(uint32_t, sector_size_bytes, Config::disk_sector_size)
	uint32_t count = 0;
	State state = State::Idle;

public:
	Disk (Computer& computer);
	~Disk ();

	void run_cycle () override final;
	uint16_t read (const uint16_t port) override final;
	void write (const uint16_t port, const uint16_t value) override final;

private:
	Descriptor open (const std::string_view fname);
	void close (Descriptor descriptor);
	uint32_t size (Descriptor descriptor) const;
	uint64_t request_read_sector (Descriptor descriptor, std::vector<uint8_t>& buffer);
	std::unique_ptr<Job> fetch_finished_job ();
	void process_job (Job& job);
};

// ---------------------------------------

class Memory : public Device
{
private:
	std::array<uint16_t, Config::memsize_words> data;

public:
	Memory (Computer& computer);
	~Memory ();

	void run_cycle () override final;

	inline uint16_t* get_raw ()
	{
		return this->data.data();
	}

	inline uint16_t operator[] (const uint32_t paddr) const
	{
		mylib_assert_exception(paddr < this->data.size())
		return this->data[paddr];
	}

	inline uint16_t& operator[] (const uint32_t paddr)
	{
		mylib_assert_exception(paddr < this->data.size())
		return this->data[paddr];
	}

	void dump (const uint16_t init = 0, const uint16_t end = Config::memsize_words-1) const;
};

// ---------------------------------------

class Timer : public IO_Device
{
private:
	uint16_t count = 0;
	uint16_t timer_interrupt_cycles = Config::timer_default_interrupt_cycles;

public:
	Timer (Computer& computer);

	void run_cycle () override final;
	uint16_t read (const uint16_t port) override final;
	void write (const uint16_t port, const uint16_t value) override final;
};

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

private:
	Memory& memory;

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
		return this->memory[paddr];
	}

	inline void pmem_write (const uint16_t paddr, const uint16_t value)
	{
		this->memory[paddr] = value;
	}

	inline uint16_t read_io (const uint16_t port)
	{
		return this->computer.get_io_port(port).read(port);
	}

	inline uint16_t read_io (const Config::IO_Ports port)
	{
		return this->read_io(std::to_underlying(port));
	}

	inline void write_io (const uint16_t port, const uint16_t value)
	{
		this->computer.get_io_port(port).write(port, value);
	}

	inline void write_io (const Config::IO_Ports port, const uint16_t value)
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

class Computer
{
private:
	std::array<IO_Device*, 1 << 16> io_ports;
	std::unique_ptr<Terminal> terminal;
	std::unique_ptr<Disk> disk;
	std::unique_ptr<Timer> timer;
	std::unique_ptr<Memory> memory;
	std::unique_ptr<Cpu> cpu;

	bool alive = false;
	uint64_t cycle = 0;

	std::string turn_off_msg;

public:
	Computer ();

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

	inline void set_io_port (const Config::IO_Ports port, IO_Device *device)
	{
		this->set_io_port(std::to_underlying(port), device);
	}

	inline IO_Device& get_io_port (const uint16_t port) const
	{
		mylib_assert_exception(port < this->io_ports.size())
		mylib_assert_exception(this->io_ports[port] != nullptr)
		return *this->io_ports[port];
	}

	inline IO_Device& get_io_port (const Config::IO_Ports port) const
	{
		return this->get_io_port(std::to_underlying(port));
	}
};

// ---------------------------------------

} // end namespace

#endif