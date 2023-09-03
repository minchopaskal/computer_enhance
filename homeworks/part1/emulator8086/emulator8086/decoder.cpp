#include "decoder.h"
#include "instructions.h"

#include <cstdio>

namespace emu8086 {

// Instruction Masks
enum InstructionMask {
	OPCODE = 0xFC,
	D = 0x02,
	W = 0x01,
};

enum DataMask {
	MOD = 0xE0,
	REG = 0x38,
	REGMEM = 0x07,
};

enum MemoryMode {
	MEMORY,
	SHORT,
	WIDE,
	REGISTER,
};

const char *reg_table[2][8] = {
	{ "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh" },
	{ "ax", "cx", "dx", "bx", "sp", "bp", "si", "di" }
};

void decode(const uint8_t *source, std::size_t source_size) {
	// Incdicate asm is  16 bits arch
	fprintf(stdout, "bits 16\n\n");
	
	std::size_t ptr = 0;
	while (ptr < source_size) {
		const uint8_t instr = source[ptr];
		const uint8_t mem = source[ptr + 1];

		const uint8_t opcode = (instr & InstructionMask::OPCODE) >> 2;
		const bool dest = (instr & InstructionMask::D) >> 1;
		const bool wide = (instr & InstructionMask::W);

		const uint8_t mode = (mem & DataMask::MOD) >> 6;
		const uint8_t reg = (mem & DataMask::REG) >> 3;
		const uint8_t regmem = (mem & DataMask::REGMEM);

		//printf("dest: %d Wide: %d mode: %d reg: %d regmem: %d\n", dest, wide, mode, reg, regmem);

		switch (opcode) {
		case Instruction::MOV: {
			switch (mode) {
			case MemoryMode::REGISTER: {
				auto src = dest ? regmem : reg;
				auto dst = dest ? reg : regmem;

				const char *fst = reg_table[wide][dst];
				const char *snd = reg_table[wide][src];
				fprintf(stdout, "mov %s, %s\n", fst, snd);
				break;
			}
			case MemoryMode::SHORT:
			case MemoryMode::WIDE:
			case MemoryMode::MEMORY:
			default:
				break;
			}
			ptr += 2;
			break;
		}
		default:
			ptr += 2;
			break;
		}
	}
}

}
