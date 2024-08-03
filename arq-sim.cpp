#include <stdexcept>
#include <utility>
#include <bitset>
#include <utility>
#include <limits>

#include <cstdint>
#include <cstdlib>

#include <signal.h>

#include "config.h"
#include "lib.h"
#include "arq-sim.h"

#include <my-lib/bit.h>

#ifndef CPU_DEBUG_MODE
	#include "os.h"
#endif

namespace Arch {

// ---------------------------------------

#ifdef CPU_DEBUG_MODE
	#define terminal_print(type, msg) { \
			std::cout << msg; \
		}

	#define terminal_println(type, msg) terminal_print(type, msg << std::endl)
#else
	#define terminal_print(type, msg) { \
			std::ostringstream str_stream; \
			str_stream << msg; \
			const std::string str = str_stream.str(); \
			terminal->print_str(Arch::Terminal::Type::type, str); \
		}

	#define terminal_println(type, msg) terminal_print(type, msg << std::endl)
#endif

// ---------------------------------------

static Computer *computer = nullptr;

// ---------------------------------------

static const char* get_reg_name_str (const uint16_t code)
{
	static constexpr auto strs = std::to_array<const char*>({
		"r0",
		"r1",
		"r2",
		"r3",
		"r4",
		"r5",
		"r6",
		"r7"
		});

	mylib_assert_exception_msg(code < strs.size(), "invalid register code ", code)

	return strs[code];
}

const char* InterruptCode_str (const InterruptCode code)
{
	static constexpr auto strs = std::to_array<const char*>({
		"Keyboard",
		"Timer",
		"GPF"
		});

	mylib_assert_exception_msg(std::to_underlying(code) < strs.size(), "invalid interrupt code ", std::to_underlying(code))

	return strs[ std::to_underlying(code) ];
}

// ---------------------------------------

VideoOutput::VideoOutput (const uint32_t xinit, const uint32_t xend, const uint32_t yinit, const uint32_t yend)
{
	const uint32_t w = xend - xinit;
	const uint32_t h = yend - yinit;

	this->buffer = MatrixBuffer(h - 2, w - 2);
	this->buffer.set_all(' ');

	this->x = 0;
	this->y = 0;

	this->win = newwin(h, w, yinit, xinit);
	refresh();
	box(this->win, 0, 0);
	wrefresh(this->win);

	this->update();
}

VideoOutput::~VideoOutput ()
{

}

void VideoOutput::print (const std::string_view str)
{
	const auto len = str.size();
	const auto nrows = this->buffer.get_nrows();
	const auto ncols = this->buffer.get_ncols();

	for (uint32_t i = 0; i < len; i++) {
		if (this->x >= ncols) {
			this->x = 0;
			this->y++;

			if (this->y >= nrows) {
				this->roll();
				this->y--;
			}
		}

		if (str[i] == '\n') {
			this->x = 0;
			this->y++;

			if (this->y >= nrows) {
				this->roll();
				this->y--;
			}
		}
		else if (str[i] == '\r') {
			this->x = 0;

			for (uint32_t i = 0; i < ncols; i++)
				this->buffer(this->y, i) = ' ';
		}
		else {
			this->buffer(this->y, this->x) = str[i];
			this->x++;
		}
	}

	this->update();
}

void VideoOutput::roll ()
{
	const auto nrows = this->buffer.get_nrows();
	const auto ncols = this->buffer.get_ncols();

	for (uint32_t row = 1; row < nrows; row++) {
		for (uint32_t col = 0; col < ncols; col++)
			this->buffer(row-1, col) = this->buffer(row, col);
	}

	// clear last line
	for (uint32_t col = 0; col < ncols; col++)
		this->buffer(nrows-1, col) = ' ';
}

void VideoOutput::update ()
{
	const auto nrows = this->buffer.get_nrows();
	const auto ncols = this->buffer.get_ncols();

	for (uint32_t row = 0; row < nrows; row++) {
		for (uint32_t col = 0; col < ncols; col++)
			mvwprintw(this->win, row+1, col+1, "%c", this->buffer(row, col));
	}

	refresh();
	wrefresh(this->win);
}

void VideoOutput::dump () const
{
	const auto nrows = this->buffer.get_nrows();
	const auto ncols = this->buffer.get_ncols();

	for (uint32_t row = 0; row < nrows; row++) {
		for (uint32_t col = 0; col < ncols; col++)
			std::cout << this->buffer(row, col);
		std::cout << std::endl;
	}
}

// ---------------------------------------

Terminal::Terminal (Computer& computer)
	: IO_Device(computer)
{
	const uint32_t total_w = COLS;
	const uint32_t total_h = LINES;

	this->videos.reserve( std::to_underlying(Type::Count) );

	// arch video
	this->videos.emplace_back(1, total_w/3, 1, total_h);

	// kernel video
	this->videos.emplace_back(total_w/3 + 1, 2*(total_w/3), 1, total_h/2);

	// command video
	this->videos.emplace_back(total_w/3 + 1, 2*(total_w/3), total_h/2 + 1, total_h);

	// app video
	this->videos.emplace_back(2*(total_w/3) + 1, total_w, 1, total_h);

	this->computer.set_io_port(Config::IO_Ports::TerminalSet, this);
	this->computer.set_io_port(Config::IO_Ports::TerminalUpload, this);
}

Terminal::~Terminal ()
{
}

void Terminal::run_cycle ()
{
	const int typed = getch();

	if (typed != ERR) {
		this->has_char = true;

		if (typed == KEY_BACKSPACE || typed == 127) // || '\b'
			this->typed_char = 8;

		this->typed_char = typed;
	}

	if (this->has_char)
		this->computer.get_cpu().interrupt(InterruptCode::Keyboard);
}

uint16_t Terminal::read (const uint16_t port)
{
	const Config::IO_Ports port_enum = static_cast<Config::IO_Ports>(port);

	switch (port_enum) {
		using enum Config::IO_Ports;

		case TerminalReadTypedChar:
			this->has_char = false;
			return this->typed_char;

		default:
			mylib_throw_exception_msg("Terminal read invalid port ", port);
	}
}

void Terminal::write (const uint16_t port, const uint16_t value)
{
	const Config::IO_Ports port_enum = static_cast<Config::IO_Ports>(port);

	switch (port_enum) {
		using enum Config::IO_Ports;

		case TerminalSet:
			this->current_video = static_cast<Type>(value);
		break;

		case TerminalUpload: {
			char str[2] = { static_cast<char>(value), 0 };
			this->videos[ std::to_underlying(this->current_video) ].print(str);
		}
		break;

		default:
			mylib_throw_exception_msg("Terminal write invalid port ", port);
	}
}

// ---------------------------------------

Disk::Disk (Computer& computer)
	: IO_Device(computer)
{
}

Disk::~Disk ()
{
	for (auto& it: this->file_descriptors) {
		auto& d = it.second;
		if (d->file.is_open())
			d->file.close();
	}
}

void Disk::run_cycle ()
{
	switch (state) {
		using enum State;

		case WaitingRead:
			if (this->count >= Config::disk_interrupt_cycles) {
				auto& file = this->current_file_descriptor->file;
				const uint16_t request_size = this->data_written;

				const auto fpos = file.tellg();
				file.seekg(0, std::ios::end);
				const auto fsize = file.tellg();
				file.seekg(fpos);

				const uint16_t available = fsize - fpos;

				uint16_t must_read;

				if (request_size > available)
					must_read = available;
				else
					must_read = request_size;

				this->buffer.resize(must_read);

				file.read(reinterpret_cast<char*>(this->buffer.data()), must_read);

				const auto readed = file.gcount();
				if (readed < must_read)
					this->buffer.resize(readed);

				this->state = State::ReadingReadSize;
			}
			else
				this->count++;
		break;

		case ReadingReadSize:
			this->computer.get_cpu().interrupt(InterruptCode::Disk);

		default: ;
	}
}

uint16_t Disk::read (const uint16_t port)
{
	const Config::IO_Ports port_enum = static_cast<Config::IO_Ports>(port);

	switch (port_enum) {
		using enum Config::IO_Ports;

		case DiskData:
			return this->process_data_read();
		break;

		default:
			mylib_throw_exception_msg("Disk read invalid port ", port);
	}
}

void Disk::write (const uint16_t port, const uint16_t value)
{
	const Config::IO_Ports port_enum = static_cast<Config::IO_Ports>(port);

	switch (port_enum) {
		using enum Config::IO_Ports;

		case DiskCmd:
			this->process_cmd(value);
		break;
		
		case DiskData:
			this->process_data_write(value);
		break;

		default:
			mylib_throw_exception_msg("Disk read invalid port ", port);
	}
}

void Disk::process_cmd (const uint16_t cmd_)
{
	const Cmd cmd = static_cast<Cmd>(cmd_);

	switch (cmd)
	{
		using enum Cmd;

		case SetFname:
			this->fname = "";
			this->state = State::SettingFname;
		break;

		case OpenFile: {
			auto desc = std::make_unique<FileDescriptor>();
			const uint16_t id = this->next_id++;
			mylib_assert_exception(id < std::numeric_limits<uint16_t>::max())
			desc->fname = std::move(this->fname);
			desc->file.open(fname.data(), std::ios::binary | std::ios_base::in);
			this->file_descriptors.insert(std::make_pair(id, std::move(desc)));
			this->current_file_descriptor = desc.get();

			this->state = State::Idle;
		};
		break;

		case CloseFile: {
			auto it = this->file_descriptors.find(this->data_written);
			mylib_assert_exception_msg(it != this->file_descriptors.end(), "file descriptor not found")
			auto& desc = it->second;
			desc->file.close();
			this->file_descriptors.erase(it);
			this->current_file_descriptor = nullptr;
		}
		break;

		case ReadFile: {
			this->state = State::WaitingReadSize;
			this->count = 0;
			auto it = this->file_descriptors.find(this->data_written);

			mylib_assert_exception_msg(it != this->file_descriptors.end(), "file descriptor not found")

			this->current_file_descriptor = it->second.get();
		}
		break;

		default:
			mylib_throw_exception_msg("Disk invalid command ", cmd_);
	}
}

uint16_t Disk::process_data_read ()
{
	uint16_t r;

	switch (this->state) {
		using enum State;

		case ReadingReadSize:
			r = this->buffer.size();
			this->buffer_pos = 0;
			this->state = ReadingSector;
		break;

		case ReadingSector:
			mylib_assert_exception_msg(this->buffer_pos < this->buffer.size(), "buffer is empty")
			r = this->buffer[this->buffer_pos++];

			if (this->buffer_pos == this->buffer.size())
				this->state = Idle;

		default:
			mylib_throw_exception_msg("Disk invalid state ", static_cast<uint16_t>(this->state));
	}

	return r;
}

void Disk::process_data_write (const uint16_t value)
{
	switch (this->state) {
		using enum State;

		case Idle:
			this->data_written = value;
		break;

		case SettingFname:
			this->fname += static_cast<char>(value);
		break;

		case WaitingReadSize:
			this->data_written = value;
			this->state = State::WaitingRead;
		break;

		default:
			mylib_throw_exception_msg("Disk invalid state ", static_cast<uint16_t>(this->state));
	}
}

std::fstream::pos_type Disk::get_file_size (std::fstream& file)
{
	const auto pos = file.tellg();
	file.seekg(0, std::ios::end);
	const auto size = file.tellg();
	file.seekg(pos);

	return size;
}

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

#ifdef CPU_DEBUG_MODE

static void fake_syscall_handler ()
{
	const uint16_t syscall = cpu->get_gpr(0);

	if (syscall == 0) {
		terminal_println(Kernel, "halt service called")
		alive = false;
	}
	else
		terminal_println(Kernel, "unknown service " << syscall << " called")
}

#endif

// ---------------------------------------

Cpu::Cpu ()
	: memory(Arch::memory)
{
	for (auto& r: this->gprs)
		r = 0;
}

Cpu::~Cpu ()
{
	
}

void Cpu::run_cycle ()
{
	enum class InstrType : uint16_t {
		R = 0,
		I = 1
	};

	if (this->has_interrupt) { // check first if external interrupt
		this->has_interrupt = false;
		OS::interrupt(this->interrupt_code);
		return;
	}

	const Mylib::BitSet<16> instruction = this->vmem_read(this->pc);

	if (this->has_interrupt) {
		this->has_interrupt = false;
		OS::interrupt(this->interrupt_code);
		return;
	}

	terminal_println(Arch, "\tPC = " << this->pc << " instr 0x" << std::hex << instruction.underlying() << std::dec << " binary " << instruction.underlying())
	
	this->pc++;

	const InstrType type = static_cast<InstrType>( instruction[15] );

	if (type == InstrType::R)
		this->execute_r(instruction);
	else
		this->execute_i(instruction);

	if (this->has_interrupt) {
		this->has_interrupt = false;
		OS::interrupt(this->interrupt_code);
	}

	this->dump();
}

void Cpu::turn_off ()
{
	alive = false;
}

bool Cpu::interrupt (const InterruptCode interrupt_code)
{
	if (this->has_interrupt)
		return false;
	this->interrupt_code = interrupt_code;
	this->has_interrupt = true;
	return true;
}

void Cpu::force_interrupt (const InterruptCode interrupt_code)
{
	mylib_assert_exception(this->has_interrupt == false)
	this->interrupt(interrupt_code);
}

void Cpu::execute_r (const Mylib::BitSet<16> instruction)
{
	enum class OpcodeR : uint16_t {
		Add = 0,
		Sub = 1,
		Mul = 2,
		Div = 3,
		Cmp_equal = 4,
		Cmp_neq = 5,
		Load = 15,
		Store = 16,
		Syscall = 63
	};

	const OpcodeR opcode = static_cast<OpcodeR>( instruction(9, 6) );
	const uint16_t dest = instruction(6, 3);
	const uint16_t op1 = instruction(3, 3);
	const uint16_t op2 = instruction(0, 3);

	switch (opcode) {
		using enum OpcodeR;

		case Add:
			terminal_println(Arch, "\tadd " << get_reg_name_str(dest) << ", " << get_reg_name_str(op1) << ", " << get_reg_name_str(op2))
			this->gprs[dest] = this->gprs[op1] + this->gprs[op2];
		break;

		case Sub:
			terminal_println(Arch, "\tsub " << get_reg_name_str(dest) << ", " << get_reg_name_str(op1) << ", " << get_reg_name_str(op2))
			this->gprs[dest] = this->gprs[op1] - this->gprs[op2];
		break;

		case Mul:
			terminal_println(Arch, "\tmul " << get_reg_name_str(dest) << ", " << get_reg_name_str(op1) << ", " << get_reg_name_str(op2))
			this->gprs[dest] = this->gprs[op1] * this->gprs[op2];
		break;

		case Div:
			terminal_println(Arch, "\tdiv " << get_reg_name_str(dest) << ", " << get_reg_name_str(op1) << ", " << get_reg_name_str(op2))
			this->gprs[dest] = this->gprs[op1] / this->gprs[op2];
		break;

		case Cmp_equal:
			terminal_println(Arch, "\tcmp_equal " << get_reg_name_str(dest) << ", " << get_reg_name_str(op1) << ", " << get_reg_name_str(op2))
			this->gprs[dest] = (this->gprs[op1] == this->gprs[op2]);
		break;

		case Cmp_neq:
			terminal_println(Arch, "\tcmp_neq " << get_reg_name_str(dest) << ", " << get_reg_name_str(op1) << ", " << get_reg_name_str(op2))
			this->gprs[dest] = (this->gprs[op1] != this->gprs[op2]);
		break;

		case Load:
			terminal_println(Arch, "\tload " << get_reg_name_str(dest) << ", [" << get_reg_name_str(op1) << "]")
			this->gprs[dest] = this->vmem_read( this->gprs[op1] );
		break;

		case Store:
			terminal_println(Arch, "\tstore [" << get_reg_name_str(op1) << "], " << get_reg_name_str(op2))
			this->vmem_write(this->gprs[op1], this->gprs[op2]);
		break;

		case Syscall:
			terminal_println(Arch, "\tsyscall");
			#ifdef CPU_DEBUG_MODE
				fake_syscall_handler();
			#else
				OS::syscall();
			#endif
		break;

		default:
			mylib_assert_exception_diecode_msg(false, endwin();, "Unknown opcode ", static_cast<uint16_t>(opcode));
	}
}

void Cpu::execute_i (const Mylib::BitSet<16> instruction)
{
	enum class OpcodeI : uint16_t {
		Jump = 0,
		Jump_cond = 1,
		Mov = 3
	};

	const OpcodeI opcode = static_cast<OpcodeI>( instruction(13, 2) );
	const uint16_t reg = instruction(10, 3);
	const uint16_t imed = instruction(0, 9);

	switch (opcode) {
		using enum OpcodeI;

		case Jump:
			terminal_println(Arch, "\tjump " << imed)
			this->pc = imed;
		break;

		case Jump_cond:
			terminal_println(Arch, "\tjump_cond " << get_reg_name_str(reg) << ", " << imed)
			if (this->gprs[reg] == 1)
				this->pc = imed;
		break;

		case Mov:
			terminal_println(Arch, "\tmov " << get_reg_name_str(reg) << ", " << imed)
			this->gprs[reg] = imed;
		break;

		default:
			mylib_assert_exception_diecode_msg(false, endwin();, "Unknown opcode ", static_cast<uint16_t>(opcode));
	}
}

void Cpu::dump () const
{
	terminal_print(Arch, "gprs:")
	for (uint32_t i = 0; i < this->gprs.size(); i++)
		terminal_print(Arch, " " << this->gprs[i])
	terminal_println(Arch, "")
}

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
}

// ---------------------------------------

void init ()
{
#ifndef CPU_DEBUG_MODE
	terminal = new Terminal;
#endif

	terminal_println(Arch, "teste arch 123456789123456789123456789123456789123456789123456789");
	terminal_println(Kernel, "teste kernel");
	terminal_println(Command, "teste command");
	terminal_println(App, "teste app");

	cpu = new Cpu;
}

void run_cycle ()
{
	terminal_println(Arch, "starting cycle " << cycle);

#ifndef CPU_DEBUG_MODE
	terminal->run_cycle();
	timer.run_cycle();
#endif
	cpu->run_cycle();

#ifdef CPU_DEBUG_MODE
//	getchar();
#endif

	cycle++;
}

void run ()
{
	while (alive)
		run_cycle();
}

// ---------------------------------------

} // end namespace Arch

// ---------------------------------------

static void interrupt_handler (int dummy)
{
#ifndef CPU_DEBUG_MODE
	endwin();
#endif

#ifdef CPU_DEBUG_MODE
	Arch::cpu->dump();
	Arch::memory.dump(0, 255);
#endif

	exit(1);
}

int main (int argc, char **argv)
{
#ifdef CPU_DEBUG_MODE
	if (argc != 2) {
		printf("usage: %s [bin_name]\n", argv[0]);
		exit(1);
	}
#endif

	signal(SIGINT, interrupt_handler);

#ifndef CPU_DEBUG_MODE
	initscr();
	timeout(0); // non-blocking input
	noecho(); // don't print input
#endif

	Arch::init();

#ifdef CPU_DEBUG_MODE
	Lib::load_binary_to_memory(argv[1], static_cast<void*>(Arch::memory.get_raw()), Config::memsize_words * sizeof(uint16_t));
	Arch::cpu->set_pc(1);
#else
	OS::boot(Arch::terminal, Arch::cpu);
#endif

	Arch::run();

#ifdef CPU_DEBUG_MODE
	Arch::cpu->dump();
	Arch::memory.dump(0, 255);
#endif

#ifndef CPU_DEBUG_MODE
	endwin();

	// print kernel msgs
	Arch::terminal->dump(Arch::Terminal::Type::Kernel);
	std::cout << std::endl;
#endif

	return 0;
}