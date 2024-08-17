#include <array>

#include "arch-lib.h"

// ---------------------------------------

namespace Arch {

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

} // end namespace