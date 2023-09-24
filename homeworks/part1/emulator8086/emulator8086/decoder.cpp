#include "decoder.h"
#include "instructions.h"

#include <bit>
#include <cassert>
#include <cstdio>
#include <format>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace emu8086 {

// State
std::vector<Instruction> decoded;
std::unordered_map<std::size_t, int> labels;

enum MemoryMode {
	NO_DISPLACEMENT = 0b00,
	SHORT = 0b01,
	WIDE = 0b10, 
	REGISTER = 0b11,
};

int16_t bitwise_abs(int16_t num) {
	int16_t mask = num >> 15;
	num = num ^ mask;

	return num - mask;
}

struct RegMemLike {
	uint8_t reg;
	uint8_t seg_reg;
	bool wide;
	bool sign_extended;
};

[[nodiscard]] RegMemLike handle_regmemlike(const uint8_t *&source, Instruction &instr) {
	const uint8_t opcode = *source++;
	const uint8_t mem = *source++;

	const bool dest = (opcode & D_MASK) != 0;
	const bool wide = (opcode & W_MASK);
	const bool sign_extended = (opcode & S_MASK) != 0;

	const uint8_t mode = (mem & MOD_MASK) >> 6;
	const uint8_t reg = (mem & SB_REG_MASK) >> 3;
	const uint8_t regmem = (mem & REGMEM_MASK);
	const uint8_t seg_reg = (mem & SR_MASK) >> 3;

	instr.flags.dest = dest;
	instr.flags.wide = wide;

	if (mode == MemoryMode::REGISTER) {
		instr.operands[0].type = OperandType::Register;
		instr.operands[0].reg = get_register(regmem, wide);
	} else {
		instr.operands[0].type = OperandType::EffectiveAddress;
		instr.operands[0].eff_addr = get_eff_addr(regmem);

		if (mode == MemoryMode::SHORT) {
			int8_t disp = *source++;
			if (is_seg_prefix(static_cast<uint8_t>(disp))) {
				instr.operands[0].seg_prefix = disp;
				disp = *source++;
			}
			instr.operands[0].displacement = disp;
		} else if (mode == MemoryMode::WIDE || regmem == 0x6) {
			uint8_t disp_l = *source++;
			if (is_seg_prefix(disp_l)) {
				instr.operands[0].seg_prefix = disp_l;
				disp_l = *source++;
			}
			uint8_t disp_h = *source++;
			int16_t disp = disp_l | (int16_t(bool(disp_h)) * (disp_h << 8) + int16_t(!bool(disp_h)) * (disp_l & 0x8000));
			instr.operands[0].displacement = disp;

			if (mode != MemoryMode::WIDE) { // Direct acess
				instr.operands[0].type = OperandType::DirectAccess;
			}
		}
	}

	return { reg, seg_reg, wide, sign_extended };
}

void handle_regmem_reg(const uint8_t *&source, Instruction &instr) {
	RegMemLike res = handle_regmemlike(source, instr);

	instr.operands[1].type = OperandType::Register;
	instr.operands[1].reg = get_register(res.reg, res.wide);
}

int16_t get_imm_data(const uint8_t *&source, bool wide, bool sign_extended = false) {
	int8_t data_l = *source++;
	uint8_t data_h = wide && !sign_extended ? *source++ : 0;
	int16_t data = *reinterpret_cast<uint8_t *>(&data_l) | (data_h << 8);

	return wide && !sign_extended ? data : data_l;
}

void handle_imm_regmem(const uint8_t *&source, Instruction &instr, bool sign_ext = false) {
	RegMemLike res = handle_regmemlike(source, instr);

	// TODO: this is actually a hack since we don't handle
	// the case when the instruction does not have a destination bit
	// this must be in the codegen.
	instr.flags.dest = false;
	res.sign_extended = sign_ext ? res.sign_extended : false;

	instr.operands[1].type = OperandType::Immediate;
	instr.operands[1].imm_value = get_imm_data(source, res.wide, res.sign_extended);
}

void handle_imm_reg(const uint8_t *&source, Instruction &instr) {
	
	const uint8_t opcode = *source++;
	const uint8_t reg = (opcode & FB_REG_MASK);

	const bool wide = (opcode & IMM_W_MASK);

	instr.operands[0].type = OperandType::Register;
	instr.operands[0].reg = get_register(reg, wide);

	instr.operands[1].type = OperandType::Immediate;
	instr.operands[1].imm_value = get_imm_data(source, wide);
}

void handle_mem_acc(const uint8_t *&source, Instruction &instr) {
	
	const uint8_t opcode = *source++;

	instr.flags.wide = (opcode & W_MASK);
	uint16_t addr = *source++;
	addr = addr | (*source++ << 8);

	instr.operands[1].type = OperandType::DirectAccess;
	instr.operands[1].direct_access = addr;

	instr.operands[0].type = OperandType::Accumulator;
}

void handle_acc_mem(const uint8_t *&source, Instruction &instr) {
	
	const uint8_t opcode = *source++;

	instr.flags.wide = (opcode & W_MASK);
	uint16_t addr = *source++;
	addr = addr | (*source++ << 8);
	
	instr.operands[0].type = OperandType::DirectAccess;
	instr.operands[0].direct_access = addr;

	instr.operands[1].type = OperandType::Accumulator;
}

void handle_imm_acc(const uint8_t *&source, Instruction &instr) {
	
	const uint8_t opcode = *source++;

	instr.flags.wide = (opcode & W_MASK);
	int16_t data = get_imm_data(source, instr.flags.wide);
	
	instr.operands[0].type = OperandType::Accumulator;

	instr.operands[1].type = OperandType::Immediate;
	instr.operands[1].imm_value = data;
}

void handle_jmp(const uint8_t *&source, Instruction &instr) {
	
	const uint8_t opcode = *source++;

	int8_t offset = *source++;

	// Each jmp instruction is 2 bytes and offset is given in bytes
	// so if we want to count offset by instructions we divide by 2
	int instr_offset = (offset >> 1);
	std::size_t line = decoded.size() + instr_offset;
	if (!labels.contains(line)) {
		labels.insert({ line, static_cast<int>(labels.size()) });
	}
	
	instr.operands[0].type = OperandType::Label;
	instr.operands[0].jmp_offset = offset;
}

void handle_regmem(const uint8_t *&source, Instruction &instr) {
	std::ignore = handle_regmemlike(source, instr);
}

void handle_regmem_1(const uint8_t *&source, Instruction &instr) {
	std::ignore = handle_regmemlike(source, instr);
	instr.operands[1].type = OperandType::Immediate;
	instr.operands[1].imm_value = 1;
}

void handle_regmem_CL(const uint8_t *&source, Instruction &instr) {
	std::ignore = handle_regmemlike(source, instr);
	instr.operands[1].type = OperandType::Register;
	instr.operands[1].reg = Register::CL;
}

void handle_mem_reg(const uint8_t *&source, Instruction &instr) {
	auto res = handle_regmemlike(source, instr);
	instr.operands[1] = instr.operands[0];

	instr.operands[0].type = OperandType::Register;
	instr.operands[0].reg = get_register(res.reg, res.wide);
}

void handle_esc(const uint8_t *&source, Instruction &instr) {
	auto fb = *source;
	auto sb = *(source + 1);

	int16_t data = 0;
	data = data | (fb & 0x07);
	data = data | ((sb & 0x38));

	auto res = handle_regmemlike(source, instr);
	instr.operands[1] = instr.operands[0];

	instr.operands[0].type = OperandType::Immediate;
	instr.operands[0].imm_value = data;
}

void handle_reg(const uint8_t *&source, Instruction &instr) {
	const uint8_t opcode = *source++;
	const uint8_t reg = (opcode & FB_REG_MASK);
	instr.operands[0].type = OperandType::Register;
	instr.operands[0].reg = get_register(reg, true);
}

void handle_seg_reg(const uint8_t *&source, Instruction &instr) {
	const uint8_t opcode = *source++;
	const uint8_t seg_reg = (opcode & SR_MASK) >> 3;

	instr.operands[0].type = OperandType::SegmentRegister;
	instr.operands[0].seg_reg = get_seg_reg(seg_reg);
}

void handle_sr_regmem(const uint8_t *&source, Instruction &instr) {
	auto res = handle_regmemlike(source, instr);
	
	instr.operands[1].type = OperandType::SegmentRegister;
	instr.operands[1].seg_reg = get_seg_reg(res.seg_reg);
}

void handle_reg_acc(const uint8_t *&source, Instruction &instr) {
	const uint8_t opcode = *source++;
	const uint8_t reg = (opcode & FB_REG_MASK);

	instr.operands[0].type = OperandType::Accumulator;

	instr.operands[1].type = OperandType::Register;
	instr.operands[1].reg = get_register(reg, true);

	instr.flags.wide = true;
}

void handle_fixed_port(const uint8_t *&source, Instruction &instr) {
	auto opcode = *source++;

	instr.flags.wide = (opcode & W_MASK);
	instr.flags.dest = (opcode & D_MASK) >> 1;

	uint8_t data = *source++;

	instr.operands[0].type = OperandType::Accumulator;

	instr.operands[1].type = OperandType::Immediate;
	instr.operands[1].imm_value = data;
}

void handle_var_port(const uint8_t *&source, Instruction &instr) {
	auto opcode = *source++;

	instr.flags.wide = (opcode & W_MASK);
	instr.flags.dest = (opcode & D_MASK) >> 1;

	instr.operands[0].type = OperandType::Accumulator;

	instr.operands[1].type = OperandType::Register;
	instr.operands[1].reg = Register::DX;
}

void print_instr(Instruction &instr, int idx);

void decode(const uint8_t *source, std::size_t source_size) {
	auto og_src = source;
	while (source < og_src + source_size) {
		const uint8_t opcode = *source;

		Instruction instr = get_instruction(opcode);
		bool special = false;
		if (instr.type == InstructionType::Special) {
			special = true;
			auto snd_byte = *(source+1);
			instr = get_special_instruction(instr, snd_byte);
		}

		switch (instr.type) {
		case InstructionType::Esc:
			handle_esc(source, instr);
			break;
		case InstructionType::Imm16:
			//handle_imm(source, instr, true);
			break;
		case InstructionType::FixedPort:
			handle_fixed_port(source, instr);
			break;
		case InstructionType::VariablePort:
			handle_var_port(source, instr);
			break;
		case InstructionType::Reg:
			handle_reg(source, instr);
			break;
		case InstructionType::RegMem:
			handle_regmem(source, instr);
			break;
		case InstructionType::Mem_Reg:
			handle_mem_reg(source, instr);
			break;
		case InstructionType::RegMem_1:
			handle_regmem(source, instr);
			break;
		case InstructionType::RegMem_CL:
			handle_regmem(source, instr);
			break;
		case InstructionType::Reg_Acc:
			handle_reg_acc(source, instr);
			break;
		case InstructionType::SR:
			handle_seg_reg(source, instr);
			break;
		case InstructionType::SR_RegMem:
			handle_sr_regmem(source, instr);
			break;
		case InstructionType::SingleByte:
			++source;
			break;
		case InstructionType::SkipSecond:
			source += 2;
			break;
		case InstructionType::RegMem_Reg:
			handle_regmem_reg(source, instr);
			break;
		case InstructionType::Imm_RegMem:
			handle_imm_regmem(source, instr);
			break;
		case InstructionType::Imm_RegMem_SE:
			handle_imm_regmem(source, instr, true);
			break;
		case InstructionType::Imm_Reg:
			handle_imm_reg(source, instr);
			break;
		case InstructionType::Mem_Acc:
			handle_mem_acc(source, instr);
			break;
		case InstructionType::Acc_Mem:
			handle_acc_mem(source, instr);
			break;
		case InstructionType::Imm_Acc:
			handle_imm_acc(source, instr);
			break;
		case InstructionType::Jmp:
			handle_jmp(source, instr);
			break;
		default:
			fprintf(stdout, "instruction not supported! Type: %d Idx: %llu\n", static_cast<int>(instr.type), decoded.size());
			exit(1);
			break; // just in case
		}

		decoded.push_back(instr);
		print_instr(instr, static_cast<int>(decoded.size() - 1));
	}
}

std::vector<Instruction> &get_decoded_instructions() {
	return decoded;
}

void print_operand(Operand &op, bool wide, std::size_t idx, bool other_imm, bool snd = false) {
	if (op.type == OperandType::None) {
		return;
	}

	fprintf(stdout, "%s ", snd ? "," : "");

	char specifier[5] = { '\0' };
	int len = sprintf(specifier, "%s", wide ? "word" : "byte");
	specifier[4] = '\0';

	switch (op.type) {
	case OperandType::Immediate:
		fprintf(stdout, "%d", op.imm_value);
		break;
	case OperandType::EffectiveAddress:
		if (other_imm) {
			fprintf(stdout, "%s ", specifier);
		}
		if (op.seg_prefix != 0xff) {
			fprintf(stdout, "%s:", segment_reg[op.seg_prefix]);
		}
		fprintf(stdout, "[%s", eff_addr_table[static_cast<int>(op.eff_addr)]);
		if (op.displacement > 0) {
			fprintf(stdout, " + %d", op.displacement);
		}
		if (op.displacement < 0) {
			fprintf(stdout, " - %d", bitwise_abs(op.displacement));
		}
		fprintf(stdout, "]");
		break;
	case OperandType::DirectAccess:
		if (other_imm) {
			fprintf(stdout, "%s ", specifier);
		}
		if (op.seg_prefix != 0xff) {
			fprintf(stdout, "%s:", segment_reg[op.seg_prefix]);
		}
		fprintf(stdout, "[%d]", op.direct_access);
		break;
	case OperandType::Register:
		fprintf(stdout, "%s", reg_table[static_cast<int>(op.reg)]);
		break;
	case OperandType::SegmentRegister:
		fprintf(stdout, "%s", segment_reg[static_cast<int>(op.seg_reg)]);
		break;
	case OperandType::Accumulator:
		fprintf(stdout, "%s", wide ? "ax" : "al");
		break;
	case OperandType::Label:
	{
		int instr_offset = (op.jmp_offset >> 1);
		std::size_t line = idx + instr_offset;
		if (auto it = labels.find(line); it != labels.end()) {
			fprintf(stdout, "label%d", it->second);
		} else {
			fprintf(stdout, "LABEL_NOT_FOUND");
		}

		break;
	}
	case OperandType::None:
		break;
	}
}

void print_instr(Instruction &instr, int idx) {
	if (instr.operands[1].type != OperandType::None && instr.flags.dest) {
		std::swap(instr.operands[0], instr.operands[1]);
	}

	auto &op0 = instr.operands[0];
	auto &op1 = instr.operands[1];

	fprintf(stdout, "%s", instr.name.c_str());
	print_operand(op0, instr.flags.wide, idx, op1.type == OperandType::Immediate || op1.type == OperandType::None);
	print_operand(op1, instr.flags.wide, idx, op0.type == OperandType::Immediate, true);

	// print label if necessary
	if (auto it = labels.find(idx); it != labels.end()) {
		fprintf(stdout, "\nlabel%s:", std::to_string(it->second).c_str());
	}

	fprintf(stdout, "\n");
}

void print_asm() {
	fprintf(stdout, "bits 16\n");

	for (std::size_t i = 0; i < decoded.size(); ++i) {
		auto &instr = decoded[i];

		print_instr(instr, static_cast<int>(i));
	}
}

}
