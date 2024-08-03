#include <stdexcept>
#include <string>
#include <string_view>

#include <cstdint>
#include <cstdlib>

#include "config.h"
#include "lib.h"
#include "arq-sim.h"
#include "os.h"


namespace OS {

// ---------------------------------------

bool terminal_is_backspace (const char c)
{
	return (c == 8);
}

bool terminal_is_alpha (const char c)
{
	return (c >= 'a') && (c <= 'z');
}

bool terminal_is_num (const char c)
{
	return (c >= '0') && (c <= '9');
}

bool terminal_is_return (const char c)
{
	return (c == '\n');
}

void terminal_print_str (Arch::Cpu *cpu, const Arch::Terminal::Type video, const std::string_view str)
{
	// TODO
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

void boot (Arch::Cpu *cpu)
{
	terminal_println(Arch::Terminal::Type::Command, "Type commands here");
	terminal_println(Arch::Terminal::Type::App, "Apps output here");
	terminal_println(Arch::Terminal::Type::Kernel, "Kernel output here");
}

// ---------------------------------------

void interrupt (const Arch::InterruptCode interrupt)
{

}

// ---------------------------------------

void syscall ()
{

}

// ---------------------------------------

} // end namespace OS
