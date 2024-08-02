#ifndef __ARQSIM_HEADER_CONFIG_H__
#define __ARQSIM_HEADER_CONFIG_H__

#include <cstdint>

//#define CPU_DEBUG_MODE

namespace Config {

	inline constexpr uint32_t nregs = 8;

	inline constexpr uint16_t memsize_words = 1 << 15;

	inline constexpr uint16_t timer_default_interrupt_cycles = 1024;

	inline constexpr uint32_t page_size_bits = 4;

	inline constexpr uint32_t disk_interrupt_cycles = 1024;

	enum class IO_Ports : uint16_t {
		TerminalSet               = 0,  // read
		TerminalUpload            = 1,  // write
		TerminalReadTypedChar     = 2,  // read
		TimerInterruptCycles      = 3,  // read/write
		DiskCmd                   = 4,  // write
		DiskFileHandler		      = 5,  // read/write
	};

	// ---------------------------------------

	// Don't change this

	inline constexpr uint32_t page_size = 1 << page_size_bits;

	inline constexpr uint32_t disk_sector_size = page_size;

}
#endif