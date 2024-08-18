#include <limits>

#include "disk.h"
#include "computer.h"
#include "cpu.h"

// ---------------------------------------

namespace Arch {

// ---------------------------------------

Disk::Disk (Computer& computer)
	: IO_Device(computer)
{
	this->computer.set_io_port(IO_Port::DiskCmd, this);
	this->computer.set_io_port(IO_Port::DiskData, this);
	this->computer.set_io_port(IO_Port::DiskState, this);
}

Disk::~Disk ()
{
	for (auto& it: this->file_descriptors) {
		auto& d = it.second;
		if (d.file.is_open())
			d.file.close();
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
	const IO_Port port_enum = static_cast<IO_Port>(port);
	uint16_t r;

	switch (port_enum) {
		using enum IO_Port;

		case DiskData:
			r = this->process_data_read();
		break;

		case DiskState:
			r = static_cast<uint16_t>(this->state);
		break;

		default:
			mylib_throw_exception_msg("Disk read invalid port ", port);
	}

	return r;
}

void Disk::write (const uint16_t port, const uint16_t value)
{
	const IO_Port port_enum = static_cast<IO_Port>(port);

	switch (port_enum) {
		using enum IO_Port;

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
			FileDescriptor desc;
			const uint16_t id = this->next_id++;
			mylib_assert_exception(id < std::numeric_limits<uint16_t>::max())
			desc.fname = std::move(this->fname);
			desc.file.open(fname.data(), std::ios::binary | std::ios_base::in);
			
			auto pair = this->file_descriptors.insert(std::make_pair(id, std::move(desc)));
			
			if (!pair.second)
				mylib_throw_exception_msg("file descriptor already exists");

			this->current_file_descriptor = &pair.first->second;

			this->state = State::Idle;
		};
		break;

		case CloseFile: {
			auto it = this->file_descriptors.find(this->data_written);
			mylib_assert_exception_msg(it != this->file_descriptors.end(), "file descriptor not found")
			FileDescriptor& desc = it->second;
			desc.file.close();
			this->file_descriptors.erase(it);
			this->current_file_descriptor = nullptr;
		}
		break;

		case ReadFile: {
			this->state = State::WaitingReadSize;
			this->count = 0;
			auto it = this->file_descriptors.find(this->data_written);

			mylib_assert_exception_msg(it != this->file_descriptors.end(), "file descriptor not found")

			this->current_file_descriptor = &it->second;
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
			this->state = ReadingFile;
		break;

		case ReadingFile:
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

		case SettingFname: {
			char c = static_cast<char>(value);

			if (c == 0)
				this->state = State::Idle;
			else
				this->fname += c;
		}
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

} // end namespace