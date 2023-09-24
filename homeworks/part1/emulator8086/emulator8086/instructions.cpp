#include "instructions.h"

#include <bit>

namespace emu8086 {

#include "scripts/instr_table.inl"

Instruction get_instruction(uint8_t opcode) {
	return instructions[opcode];
}

Instruction get_special_instruction(const Instruction& ins, uint8_t second_byte) {
	return special_instructions[ins.__special_instr_idx][(second_byte & SB_REG_MASK)>>3];
}

Register get_register(uint8_t idx, bool wide) {
	return static_cast<Register>(uint8_t(wide) * 8 + idx);
}

EffectiveAddress get_eff_addr(uint8_t idx) {
	return static_cast<EffectiveAddress>(idx);
}

SegmentRegister get_seg_reg(uint8_t idx) {
	return static_cast<SegmentRegister>(idx);
}

bool is_seg_prefix(uint8_t b) {
	return instructions[b].type == InstructionType::SegmentPrefix;
}

}