#ifndef __ARQSIM_HEADER_ARCH_DISK_H__
#define __ARQSIM_HEADER_ARCH_DISK_H__

#include <my-lib/std.h>
#include <my-lib/macros.h>

#include "config.h"
#include "arch-lib.h"

namespace Arch {

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
	std::unordered_map<uint16_t, FileDescriptor> file_descriptors;
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

} // end namespace

#endif