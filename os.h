#ifndef __ARQSIM_HEADER_OS_H__
#define __ARQSIM_HEADER_OS_H__

#include <cstdint>

#include <my-lib/std.h>
#include <my-lib/macros.h>

#include "config.h"
#include "arq-sim.h"

namespace OS {

// ---------------------------------------

void boot (Arch::Cpu *cpu);

void interrupt (const Arch::InterruptCode interrupt);

void syscall ();

// ---------------------------------------

inline bool terminal_is_backspace (const char c)
{
	return (c == 8);
}

inline bool terminal_is_alpha (const char c)
{
	return (c >= 'a') && (c <= 'z');
}

inline bool terminal_is_num (const char c)
{
	return (c >= '0') && (c <= '9');
}

inline bool terminal_is_return (const char c)
{
	return (c == '\n');
}

static void terminal_print_str (Arch::Cpu *cpu, const Arch::Terminal::Type video, const std::string_view str)
{
	cpu->write_io(Config::IO_Ports::TerminalSet, static_cast<uint16_t>(video));

	for (const char c : str)
		cpu->write_io(Config::IO_Ports::TerminalUpload, static_cast<uint16_t>(c));
}

template <typename... Types>
void terminal_print (const Arch::Terminal::Type video, Types&&... vars)
{
	const std::string str = Mylib::build_str_from_stream(vars...);
	terminal_print_str(video, str);
}

template <typename... Types>
void terminal_println (const Arch::Terminal::Type video, Types&&... vars)
{
	terminal_print(video, vars..., '\n');
}



// ---------------------------------------

} // end namespace

#endif