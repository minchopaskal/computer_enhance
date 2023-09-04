#include "decoder.h"
#include "instructions.h"

#include <bit>
#include <cassert>
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
	NO_DISPLACEMENT,
	SHORT,
	WIDE,
	REGISTER,
};

const char *reg_table[2][8] = {
	{ "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh", },
	{ "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", },
};

const char *eff_addr_table[8] = {
	"bx + si",
	"bx + di",
	"bp + si",
	"bp + di",
	"si",
	"di",
	"bp",
	"bx",
};

bool check_opcode(uint8_t instr, Instruction opcode) {
	int offset = std::countl_zero(uint8_t(opcode));
	return ((instr >> offset) & opcode) == opcode;
}

int16_t bitwise_abs(int16_t num) {
	int16_t mask = num >> 15;
	num = num ^ mask;

	return num - mask;
}

void decode(const uint8_t *source, std::size_t source_size) {
	// Incdicate asm is  16 bits arch
	fprintf(stdout, "bits 16\n\n");
	
	std::size_t ptr = 0;
	while (ptr < source_size) {
		const uint8_t instr = source[ptr++];
		//fprintf(stdout, "INSTRUCTION: %x ", instr);

		if (check_opcode(instr, Instruction::IMM_TO_REG)) {
			const bool wide = (instr & 0x8);
			const bool reg = (instr & 0x7);
			
			uint8_t data_l = source[ptr++];
			uint8_t data_h = wide ? source[ptr++] : 0;
			uint16_t data = data_l | (data_h << 8);

			const char *r = reg_table[wide][reg];
			fprintf(stdout, "mov %s, %d\n", r, data);

			continue;
		}

		if (check_opcode(instr, Instruction::REGMEM_TOFROM_REG)) {
			const uint8_t mem = source[ptr++];

			const bool dest = (instr & InstructionMask::D) >> 1;
			const bool wide = (instr & InstructionMask::W);

			const uint8_t mode = (mem & DataMask::MOD) >> 6;
			const uint8_t reg = (mem & DataMask::REG) >> 3;
			const uint8_t regmem = (mem & DataMask::REGMEM);

			if (mode == MemoryMode::REGISTER) {
				auto src = dest ? regmem : reg;
				auto dst = dest ? reg : regmem;

				const char *fst = reg_table[wide][dst];
				const char *snd = reg_table[wide][src];
				fprintf(stdout, "mov %s, %s\n", fst, snd);
			} else {
				const char *r = reg_table[wide][reg];
				char eff_addr_calc[16] = { '\0' };
				const char *eff_addr = eff_addr_table[regmem];

				if (mode == MemoryMode::SHORT) {
					int8_t disp = source[ptr++];
					if (disp) {
						sprintf(eff_addr_calc, "%s %s %d", eff_addr, disp > 0 ? "+" : "-", bitwise_abs(disp));
					} else {
						sprintf(eff_addr_calc, "%s", eff_addr);
					}
				} else if (mode == MemoryMode::WIDE || regmem == 0x6) {
					uint8_t disp_l = source[ptr++];
					uint8_t disp_h = source[ptr++];
					int16_t disp = disp_l | (int16_t(bool(disp_h)) * (disp_h << 8) + int16_t(!bool(disp_h)) * (disp_l & 0x8000));

					if (mode == MemoryMode::WIDE) {
						if (disp) {
							sprintf(eff_addr_calc, "%s %s %d", eff_addr, disp > 0 ? "+" : "-", bitwise_abs(disp));
						} else {
							sprintf(eff_addr_calc, "%s", eff_addr);
						}
					} else {
						sprintf(eff_addr_calc, "%d", disp);
					}
				} else { // MemoryMode::NO_DISPLACEMENT
					sprintf(eff_addr_calc, "%s", eff_addr);
				}

				if (dest) {
					fprintf(stdout, "mov %s, [%s]\n", r, eff_addr_calc);
				} else {
					fprintf(stdout, "mov [%s], %s\n", eff_addr_calc, r);
				}
			}
			
			continue;
		}

		if (check_opcode(instr, Instruction::IMM_TO_REGMEM)) {
			uint8_t operands = source[ptr++];
			
			const bool wide = (instr & 0x1);
			const uint8_t mode = (operands & DataMask::MOD) >> 6;
			const uint8_t regmem = (operands & DataMask::REGMEM);

			if (mode == MemoryMode::REGISTER) {
				const char *r = reg_table[wide][regmem];

				uint8_t data_l = source[ptr++];
				uint8_t data_h = wide ? source[ptr++] : 0;
				uint16_t data = data_l | (data_h << 8);

				fprintf(stdout, "mov %s, %d\n", r, data);
			} else if (mode == MemoryMode::SHORT) {
				char eff_addr_calc[16] = { '\0' };
				int8_t disp = source[ptr++];
				
				uint16_t data = source[ptr++];
				data = wide ? (data | (source[ptr++] << 8)) : data;

				if (disp) {
					sprintf(eff_addr_calc, "%s %s %d", eff_addr_calc, disp > 0 ? "+" : "-", bitwise_abs(disp));
				} else {
					sprintf(eff_addr_calc, "%s", eff_addr_table[regmem]);
				}

				fprintf(stdout, "mov [%s], byte %d\n", eff_addr_calc, data);
			} else if (mode == MemoryMode::WIDE) {
				char eff_addr_calc[16] = { '\0' };
				uint8_t disp_l = source[ptr++];
				uint8_t disp_h = source[ptr++];
				int16_t disp = disp_l | (int16_t(bool(disp_h)) * (disp_h << 8) + int16_t(!bool(disp_h)) * (disp_l & 0x8000));

				uint16_t data = source[ptr++];
				data = wide ? (data | (source[ptr++] << 8)) : data;

				if (disp) {
					sprintf(eff_addr_calc, "%s %s %d", eff_addr_table[regmem], disp > 0 ? "+" : "-", bitwise_abs(disp));
				} else {
					sprintf(eff_addr_calc, "%s", eff_addr_table[regmem]);
				}

				fprintf(stdout, "mov [%s], word %d\n", eff_addr_calc, data);
			} else if (mode == MemoryMode::NO_DISPLACEMENT) {
				char eff_addr_calc[16] = { '\0' };

				if (regmem == 0x6) {
					uint8_t disp_l = source[ptr++];
					uint8_t disp_h = source[ptr++];
					uint16_t disp = disp_l | disp_h;

					sprintf(eff_addr_calc, "%d", disp);
				} else {
					sprintf(eff_addr_calc, "%s", eff_addr_table[regmem]);
				}
				
				uint16_t data = source[ptr++];
				data = wide ? (data | (source[ptr++] << 8)) : data;

				fprintf(stdout, "mov [%s], %s %d\n", eff_addr_table[regmem], (wide ? "word" : "byte"), data);
			}

			continue;
		}

		if (check_opcode(instr, Instruction::ACC_TO_MEM)) {
			const bool wide = (instr & 0x1);
			uint16_t addr = source[ptr++];
			addr = addr | (source[ptr++] << 8);

			fprintf(stdout, "mov [%d], %s\n", addr, wide ? "ax" : "al");

			continue;
		}

		if (check_opcode(instr, Instruction::MEM_TO_ACC)) {
			const bool wide = (instr & 0x1);
			uint16_t addr = source[ptr++];
			addr = addr | (source[ptr++] << 8);

			fprintf(stdout, "mov %s, [%d]\n", wide ? "ax" : "al", addr);

			continue;
		}
	}
}

}
