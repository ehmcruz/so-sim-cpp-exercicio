#include <array>

#include "device.h"

// ---------------------------------------

namespace Arch {

// ---------------------------------------

const char* InterruptCode_str (const InterruptCode code)
{
	static constexpr auto strs = std::to_array<const char*>({
		"Keyboard",
		"Disk",
		"Timer",
		"GPF",
		"PageFault"
		});

	mylib_assert_exception_msg(std::to_underlying(code) < strs.size(), "invalid interrupt code ", std::to_underlying(code))

	return strs[ std::to_underlying(code) ];
}

// ---------------------------------------

} // end namespace