#include "instructions.h"

#include <bit>

#define OPCODE(v) 0b##v, (sizeof(#v) - 1)
#define O(v) OPCODE(v) // brevity

namespace emu8086 {

Instruction instructions[256];
Instruction special_instructions[7][8] = {
	// 0x80-0x81
	{
		{"add", InstructionType::Special_Imm_RegMem},
		{"or", InstructionType::Special_Imm_RegMem},
		{"adc", InstructionType::Special_Imm_RegMem},
		{"sbb", InstructionType::Special_Imm_RegMem},
		{"and", InstructionType::Special_Imm_RegMem},
		{"sub", InstructionType::Special_Imm_RegMem},
		{"xor", InstructionType::Special_Imm_RegMem},
		{"cmp", InstructionType::Special_Imm_RegMem},
	},
	// 0x82-0x83
	{
		{"add", InstructionType::Special_Imm_RegMem},
		{"", InstructionType::Unknown},
		{"adc", InstructionType::Special_Imm_RegMem},
		{"sbb", InstructionType::Special_Imm_RegMem},
		{"", InstructionType::Unknown},
		{"sub", InstructionType::Special_Imm_RegMem},
		{"", InstructionType::Unknown},
		{"cmp", InstructionType::Special_Imm_RegMem},
	},
	// 0xD0-0xD1
	{
		{"rol", InstructionType::Special_RegMem_1},
		{"ror", InstructionType::Special_RegMem_1},
		{"rcl", InstructionType::Special_RegMem_1},
		{"rcr", InstructionType::Special_RegMem_1},
		{"sal", InstructionType::Special_RegMem_1},
		{"shr", InstructionType::Special_RegMem_1},
		{"", InstructionType::Unknown},
		{"sar", InstructionType::Special_RegMem_1},
	},
	// 0xD2-0xD3
	{
		{"rol", InstructionType::Special_RegMem_CL},
		{"ror", InstructionType::Special_RegMem_CL},
		{"rcl", InstructionType::Special_RegMem_CL},
		{"rcr", InstructionType::Special_RegMem_CL},
		{"sal", InstructionType::Special_RegMem_CL},
		{"shr", InstructionType::Special_RegMem_CL},
		{"", InstructionType::Unknown},
		{"sar", InstructionType::Special_RegMem_CL},
	},
	// 0xF6-0xF7
	{
		{"test", InstructionType::Special_Imm_RegMem},
		{"", InstructionType::Special_RegMem},
		{"not", InstructionType::Special_RegMem},
		{"neg", InstructionType::Special_RegMem},
		{"mul", InstructionType::Special_RegMem},
		{"imul", InstructionType::Special_RegMem},
		{"div", InstructionType::Special_RegMem},
		{"idiv", InstructionType::Special_RegMem},
	},
	// 0xFE
	{
		{"inc", InstructionType::Special_RegMem},
		{"dec", InstructionType::Special_RegMem},
		{"", InstructionType::Unknown},
		{"", InstructionType::Unknown},
		{"", InstructionType::Unknown},
		{"", InstructionType::Unknown},
		{"", InstructionType::Unknown},
		{"", InstructionType::Unknown},
	},
	// 0xFF
	{
		{"inc", InstructionType::Special_Mem},
		{"dec", InstructionType::Special_Mem},
		{"call", InstructionType::Special_RegMem},
		{"call", InstructionType::Special_Mem},
		{"jmp", InstructionType::Special_RegMem},
		{"jmp", InstructionType::Special_Mem},
		{"push", InstructionType::Special_Mem},
		{"", InstructionType::Unknown},
	},
};

void RegisterInstruction(const std::string& name, uint8_t opcode, int num_bits, InstructionType it, int special_idx = -1) {
	auto trail = 8 - num_bits;
	Instruction instruction {
		.name = name,
		.type = it,
		.special_instr_idx = special_idx,
	};

	int num_combinations = (1 << trail);
	opcode = (opcode << trail);
	for (int i = 0; i < num_combinations; ++i) {
		instructions[opcode + i] = instruction;
	}
}

void register_instructions() {
	RegisterInstruction("mov", O(100010), InstructionType::RegMem_Reg);
	RegisterInstruction("mov", O(1100011), InstructionType::Imm_RegMem); // Actually special, but lone in category.
	RegisterInstruction("mov", O(1011), InstructionType::Imm_Reg);
	RegisterInstruction("mov", O(1010000), InstructionType::Mem_Acc);
	RegisterInstruction("mov", O(1010001), InstructionType::Acc_Mem);
	RegisterInstruction("mov", O(10001110), InstructionType::RegMem_SR);
	RegisterInstruction("mov", O(10001100), InstructionType::SR_RegMem);

	RegisterInstruction("add", O(000000), InstructionType::RegMem_Reg);
	// Ox80-0x81
	RegisterInstruction("", O(1000000), InstructionType::Special_Imm_RegMem, 0);
	// Ox82-0x83
	RegisterInstruction("", O(1000001), InstructionType::Special_Imm_RegMem, 1);
	RegisterInstruction("add", O(0000010), InstructionType::Imm_Acc);

	RegisterInstruction("sub", O(001010), InstructionType::RegMem_Reg);
	RegisterInstruction("sub", O(0010110), InstructionType::Imm_Acc);

	RegisterInstruction("cmp", O(001110), InstructionType::RegMem_Reg);
	RegisterInstruction("cmp", O(0011110), InstructionType::Imm_Acc);

	RegisterInstruction("je",     O(01110100), InstructionType::Jmp);
	RegisterInstruction("jl",     O(01111100), InstructionType::Jmp);
	RegisterInstruction("jle",    O(01111110), InstructionType::Jmp);
	RegisterInstruction("jb",     O(01110010), InstructionType::Jmp);
	RegisterInstruction("jbe",    O(01110110), InstructionType::Jmp);
	RegisterInstruction("jp",     O(01111010), InstructionType::Jmp);
	RegisterInstruction("jo",     O(01110000), InstructionType::Jmp);
	RegisterInstruction("js",     O(01111000), InstructionType::Jmp);
	RegisterInstruction("jnz",    O(01110101), InstructionType::Jmp);
	RegisterInstruction("jnl",    O(01111101), InstructionType::Jmp);
	RegisterInstruction("jg",     O(01111111), InstructionType::Jmp);
	RegisterInstruction("jnb",    O(01110011), InstructionType::Jmp);
	RegisterInstruction("ja",     O(01110111), InstructionType::Jmp);
	RegisterInstruction("jnp",    O(01111011), InstructionType::Jmp);
	RegisterInstruction("jno",    O(01110001), InstructionType::Jmp);
	RegisterInstruction("jns",    O(01111001), InstructionType::Jmp);
	RegisterInstruction("loop",   O(11100010), InstructionType::Jmp);
	RegisterInstruction("loopz",  O(11100001), InstructionType::Jmp);
	RegisterInstruction("loopnz", O(11100000), InstructionType::Jmp);
	RegisterInstruction("jcxz",   O(11100011), InstructionType::Jmp);

	// TODO:
	// file format:
	// <instruction-name> <bit-layout> <instruction-type> [<special-instr-idx>]
	// python to generate the c++ table

	//RegisterInstruction("push", O(01010), InstructionType::Reg);
	//for (int i = 0; i < 8; ++i) {
	//	RegisterInstruction("push", 0b000000110 + (i << 3), 8, InstructionType::SR);
	//}

	//// This 0x8F POP is actually special but it's the only one
	//// in its category so treating it as normal instruction
	//RegisterInstruction("pop", O(10001111), InstructionType::RegMem);
	//RegisterInstruction("pop", O(01011), InstructionType::RegMem);
	//for (int i = 0; i < 8; ++i) {
	//	RegisterInstruction("pop", 0b000000111 + (i << 3), 8, InstructionType::SR);
	//}

	//RegisterInstruction("xchg", O(1000011), InstructionType::RegMem_Reg);
	//RegisterInstruction("xchg", O(10010), InstructionType::Reg_Acc);

	//RegisterInstruction("in", O(1110010), InstructionType::FixedPort);
	//RegisterInstruction("in", O(1110110), InstructionType::VariablePort);

	//RegisterInstruction("out", O(1110011), InstructionType::FixedPort);
	//RegisterInstruction("out", O(1110111), InstructionType::VariablePort);

	//RegisterInstruction("xlat", O(11010111), InstructionType::SingleByte);
	//RegisterInstruction("lea", O(10001101), InstructionType::RegMem_Reg);
	//RegisterInstruction("lds", O(10001101), InstructionType::RegMem_Reg);
	//RegisterInstruction("les", O(10000100), InstructionType::RegMem_Reg);
	//RegisterInstruction("lahf", O(11010111), InstructionType::SingleByte);
	//RegisterInstruction("sahf", O(11010111), InstructionType::SingleByte);
	//RegisterInstruction("pushf", O(11010111), InstructionType::SingleByte);
	//RegisterInstruction("popf", O(11010111), InstructionType::SingleByte);
}

Instruction get_instruction(uint8_t opcode) {
	return instructions[opcode];
}

Instruction get_special_instruction(const Instruction& ins, uint8_t second_byte) {
	return special_instructions[ins.special_instr_idx][(second_byte & SB_REG_MASK)>>3];
}

}