#include "emulator.h"

#include "memory.h"

namespace emu8086 {

void emulate(const std::vector<Instruction> &instructions) {
	Register *regs = get_registers();
	SegmentRegister *srs = get_srs();

	for (auto &instr : instructions) {
		switch (instr.opcode) {
		case InstructionOpcode::mov:
		{
			uint16_t src_data = 0;

			switch (instr.operands[1].type) {
			case OperandType::Immediate:
				src_data = instr.operands[1].imm_value;
				break;
			case OperandType::Register:
				src_data = get_register_data(instr.operands[1].reg);
				break;
			case OperandType::SegmentRegister:
				src_data = get_sr(instr.operands[1].seg_reg);
				break;
			default:
				break;
			}

			switch (instr.operands[0].type) {
			case OperandType::Register:
				set_register(instr.operands[0].reg, src_data);
				break;
			case OperandType::SegmentRegister:
				set_sr(instr.operands[0].seg_reg, src_data);
				break;
			default:
				break;
			}
			break;
		}
		default:
			fprintf(stdout, "Ignoring instruction %s\n", instr.name.c_str());
			break;
		}
	}
}

} // namespace emu8086
