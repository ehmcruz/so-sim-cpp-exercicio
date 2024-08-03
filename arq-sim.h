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
#include <unordered_map>

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
	Keyboard         = 0,
	Disk             = 1,
	Timer            = 2,
	GPF              = 3,
};

const char* InterruptCode_str (const InterruptCode code);

enum class IO_Port : uint16_t {
	TerminalSet               = 0,   // write
	TerminalUpload            = 1,   // write
	TerminalReadTypedChar     = 2,   // read
	TimerInterruptCycles      = 10,   // read/write
	DiskCmd                   = 20,  // write
	DiskData		          = 21,  // read/write
	DiskState                 = 22,  // read
};

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
		Arch        = 0,
		Kernel      = 1,
		Command     = 2,
		App         = 3,

		Count       = 4 // amount of sub-terminals
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
		SetFname           = 0,
		OpenFile           = 1,
		CloseFile          = 2,
		ReadFile           = 3,
		WriteFile          = 4,
		GetFileSize        = 5,
		SeekFilePos        = 6,
	};

	enum class State : uint16_t {
		Idle                    = 0,
		SettingFname            = 1,
		WaitingReadSize         = 2,
		WaitingRead             = 3,
		ReadingReadSize         = 4,
		ReadingFile             = 5,
	};

private:
	struct FileDescriptor {
		std::string fname;
		std::fstream file;
	};

private:
	std::unordered_map<uint16_t, std::unique_ptr<FileDescriptor>> file_descriptors;
	uint32_t count = 0;
	uint16_t next_id = 1;
	State state = State::Idle;
	std::string fname;
	uint16_t data_written;
	std::vector<uint8_t> buffer;
	uint32_t buffer_pos;
	FileDescriptor *current_file_descriptor = nullptr;

public:
	Disk (Computer& computer);
	~Disk ();

	void run_cycle () override final;
	uint16_t read (const uint16_t port) override final;
	void write (const uint16_t port, const uint16_t value) override final;

private:
	void process_cmd (const uint16_t cmd_);
	uint16_t process_data_read ();
	void process_data_write (const uint16_t value);

	static std::fstream::pos_type get_file_size (std::fstream& file);
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