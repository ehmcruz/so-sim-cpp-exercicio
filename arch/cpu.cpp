#include "cpu.h"

// ---------------------------------------

namespace Arch {

// ---------------------------------------

Cpu::Cpu (Computer& computer)
	: Device(computer)
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

} // end namespace