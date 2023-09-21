#include "decoder.h"
#include "instructions.h"

#include <bit>
#include <cassert>
#include <cstdio>
#include <format>
#include <optional>
#include <string>
#include <unordered_map>

namespace emu8086 {

// State
struct HumanAsm {
	std::string instr;
	std::optional<std::size_t> jmp_line;

	HumanAsm(std::string &&str) : instr(str) {}
	HumanAsm(std::string &&str, std::size_t jmp_line) : instr(str), jmp_line(jmp_line) {}
};

std::vector<HumanAsm> human_asm;
std::unordered_map<std::size_t, int> labels;

enum MemoryMode {
	NO_DISPLACEMENT = 0b00,
	SHORT = 0b01,
	WIDE = 0b10, 
	REGISTER = 0b11,
};

enum SegRegCode {
	ES = 0b00,
	CS = 0b01,
	SS = 0b10,
	DS = 0b11,
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

const char *segment_reg[4] = {
	"es", "cs", "ss", "ds"
};

//bool check_opcode(uint8_t instr, Instruction opcode) {
//	int offset = std::countl_zero(uint8_t(opcode));
//	return ((instr >> offset) & opcode) == opcode;
//}

int16_t bitwise_abs(int16_t num) {
	int16_t mask = num >> 15;
	num = num ^ mask;

	return num - mask;
}

void handle_regmem_reg(std::string_view name, const uint8_t *&source, uint8_t opcode) {
	const uint8_t mem = *source++;
	const bool dest = (opcode & D_MASK) != 0;
	const bool wide = (opcode & W_MASK);
	
	const uint8_t mode = (mem & MOD_MASK) >> 6;
	const uint8_t reg = (mem & SB_REG_MASK) >> 3;
	const uint8_t regmem = (mem & REGMEM_MASK);
	
	if (mode == MemoryMode::REGISTER) {
		auto src = dest ? regmem : reg;
		auto dst = dest ? reg : regmem;
	
		const char *fst = reg_table[wide][dst];
		const char *snd = reg_table[wide][src];
		human_asm.push_back(std::format("{} {}, {}", name.data(), fst, snd));
	} else {
		const char *r = reg_table[wide][reg];
		char eff_addr_calc[16] = { '\0' };
		const char *eff_addr = eff_addr_table[regmem];
	
		if (mode == MemoryMode::SHORT) {
			int8_t disp = *source++;
			if (disp) {
				sprintf(eff_addr_calc, "%s %s %d", eff_addr, disp > 0 ? "+" : "-", bitwise_abs(disp));
			} else {
				sprintf(eff_addr_calc, "%s", eff_addr);
			}
		} else if (mode == MemoryMode::WIDE || regmem == 0x6) {
			uint8_t disp_l = *source++;
			uint8_t disp_h = *source++;
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
			assert(mode == MemoryMode::NO_DISPLACEMENT);
			sprintf(eff_addr_calc, "%s", eff_addr);
		}
	
		if (dest) {
			human_asm.push_back(std::format("{} {}, [{}]", name.data(), r, eff_addr_calc));
		} else {
			human_asm.push_back(std::format("{} [{}], {}", name.data(), eff_addr_calc, r));
		}
	}
}

int16_t get_imm_data(const uint8_t *&source, bool wide, bool sign_extended = false) {
	int8_t data_l = *source++;
	uint8_t data_h = wide && !sign_extended ? *source++ : 0;
	int16_t data = *reinterpret_cast<uint8_t *>(&data_l) | (data_h << 8);

	return wide && !sign_extended ? data : data_l;
}

void handle_imm_regmem(std::string_view name, const uint8_t *&source, uint8_t opcode, bool special = false) {
	uint8_t operands = *source++;
				
	const bool wide = (opcode & W_MASK);
	const bool sign_extended = (opcode & S_MASK) && special;
	const uint8_t mode = (operands & MOD_MASK) >> 6;
	const uint8_t regmem = (operands & REGMEM_MASK);
	
	if (mode == MemoryMode::REGISTER) {
		const char *r = reg_table[wide][regmem];
	
		int16_t data = get_imm_data(source, wide, sign_extended);
	
		human_asm.push_back(std::format("{} {}, {}", name.data(), r, data));
	} else if (mode == MemoryMode::SHORT) {
		char eff_addr_calc[16] = { '\0' };
		int8_t disp = *source++;
					
		int16_t data = get_imm_data(source, wide, sign_extended);
	
		if (disp) {
			sprintf(eff_addr_calc, "%s %s %d", eff_addr_table[regmem], disp > 0 ? "+" : "-", bitwise_abs(disp));
		} else {
			sprintf(eff_addr_calc, "%s", eff_addr_table[regmem]);
		}
	
		human_asm.push_back(std::format("{} [{}], byte {}", name.data(), eff_addr_calc, data));
	} else if (mode == MemoryMode::WIDE) {
		char eff_addr_calc[16] = { '\0' };
		uint8_t disp_l = *source++;
		uint8_t disp_h = *source++;
		int16_t disp = disp_l | (int16_t(bool(disp_h)) * (disp_h << 8) + int16_t(!bool(disp_h)) * (disp_l & 0x8000));
	
		int16_t data = get_imm_data(source, wide, sign_extended);
	
		if (disp) {
			sprintf(eff_addr_calc, "%s %s %d", eff_addr_table[regmem], disp > 0 ? "+" : "-", bitwise_abs(disp));
		} else {
			sprintf(eff_addr_calc, "%s", eff_addr_table[regmem]);
		}
	
		human_asm.push_back(std::format("{} [{}], word {}", name.data(), eff_addr_calc, data));
	} else if (mode == MemoryMode::NO_DISPLACEMENT) {
		char eff_addr_calc[16] = { '\0' };
	
		if (regmem == 0x6) {
			uint8_t disp_l = *source++;
			uint8_t disp_h = *source++;
			uint16_t disp = disp_l | disp_h;
	
			sprintf(eff_addr_calc, "%d", disp);
		} else {
			sprintf(eff_addr_calc, "%s", eff_addr_table[regmem]);
		}
					
		int16_t data = get_imm_data(source, wide, sign_extended);
	
		human_asm.push_back(std::format("{} [{}], {} {}", name.data(), eff_addr_table[regmem], (wide ? "word" : "byte"), data));
	}
}

void handle_imm_reg(std::string_view name, const uint8_t *&source, uint8_t opcode) {
	const uint8_t reg = (opcode & FB_REG_MASK);

	const bool wide = (opcode & IMM_W_MASK);
	int16_t data = get_imm_data(source, wide);

	const char *r = reg_table[wide][reg];
	human_asm.push_back(std::format("{} {}, {}", name.data(), r, data));
}

void handle_mem_acc(std::string_view name, const uint8_t *&source, uint8_t opcode) {
	const bool wide = (opcode & 0x1);
	uint16_t addr = *source++;
	addr = addr | (*source++ << 8);
	
	human_asm.push_back(std::format("{} {}, [{}]", name.data(), wide ? "ax" : "al", addr));
}

void handle_acc_mem(std::string_view name, const uint8_t *&source, uint8_t opcode) {
	const bool wide = (opcode & W_MASK);
	uint16_t addr = *source++;
	addr = addr | (*source++ << 8);
	
	human_asm.push_back(std::format("{} [{}], {}", name.data(), addr, wide ? "ax" : "al"));
}

void handle_imm_acc(std::string_view name, const uint8_t *&source, uint8_t opcode) {
	const bool wide = (opcode & W_MASK);
	int16_t data = get_imm_data(source, wide);

	human_asm.push_back(std::format("{} {}, {}", name.data(), wide ? "ax" : "al", data));
}

void handle_jmp(std::string_view name, const uint8_t *&source) {
	int8_t offset = *source++;

	// Each jmp instruction is 2 bytes and offset is given in bytes
	// so if we want to count offset by instructions we divide by 2
	int instr_offset = (offset >> 1);
	std::size_t line = human_asm.size() + instr_offset;
	if (!labels.contains(line)) {
		labels.insert({ line, static_cast<int>(labels.size()) });
	}
	human_asm.push_back(HumanAsm{ std::format("{}", name.data(), offset), line });
}

void decode(const uint8_t *source, std::size_t source_size) {
	// Incdicate asm is  16 bits arch
	human_asm.push_back(std::format("bits 16"));

	auto og_src = source;
	while (source < og_src + source_size) {
		const uint8_t opcode = *source++;

		Instruction instr = get_instruction(opcode);
		bool special = false;
		if (instr.type == InstructionType::Special) {
			special = true;
			auto snd_byte = *source;
			instr = get_special_instruction(instr, snd_byte);
		}

		switch (instr.type) {
		case InstructionType::RegMem_Reg:
		{
			handle_regmem_reg(instr.name, source, opcode);
			break;
		}
		case InstructionType::Imm_RegMem:
		{
			handle_imm_regmem(instr.name, source, opcode, special);
			break;
		}
		case InstructionType::Imm_Reg:
		{
			handle_imm_reg(instr.name, source, opcode);
			break;
		}
		case InstructionType::Mem_Acc:
		{
			handle_mem_acc(instr.name, source, opcode);
			break;
		}
		case InstructionType::Acc_Mem:
		{
			handle_acc_mem(instr.name, source, opcode);
			break;
		}
		case InstructionType::Imm_Acc:
		{
			handle_imm_acc(instr.name, source, opcode);
			break;
		}
		case InstructionType::Jmp:
		{
			handle_jmp(instr.name, source);
			break;
		}
		default:
			human_asm.push_back(std::format("{}", instr.name));
			fprintf(stdout, "instruction not supported!");
			//exit(1);
			break; // just in case
		}
	}

	for (std::size_t i = 0; i < human_asm.size(); ++i) {
		auto &instr = human_asm[i];
		int label_idx = -1;
		bool is_jmp = instr.jmp_line.has_value();
		if (is_jmp) {
			label_idx = labels[instr.jmp_line.value()];
		}
		fprintf(stdout, "%s%s\n", instr.instr.c_str(), is_jmp ? (" label" + std::to_string(label_idx)).c_str() : "");
		if (auto it = labels.find(i); it != labels.end()) {
			fprintf(stdout, "label%s:\n", std::to_string(it->second).c_str());
		}
	}
}

}
