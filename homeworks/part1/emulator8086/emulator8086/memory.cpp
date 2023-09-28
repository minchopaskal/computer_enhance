#include "memory.h"

#include <cstdio>

namespace emu8086 {

Register registers[8];
SegmentRegister seg_regs[4];

Register *get_registers() {
	return registers;
}

uint16_t get_register_data(RegisterName reg) {
	int r = static_cast<int>(reg);
	if (r >= 8) {
		return registers[r - 8].data;
	} else if (r >= 4) {
		return registers[r - 4].high;
	} else {
		return registers[r].low;
	}
}

void set_register(RegisterName reg, uint16_t data) {
	auto r = static_cast<int>(reg);
	if (r >= 8) {
		registers[r - 8].data = data;
	} else if (r >= 4) {
		registers[r - 4].high = (data & 0xFF);
	} else {
		registers[r].low = (data & 0xFF);
	}
}

void print_registers() {
	static constexpr int idxs[8] = { 0, 3, 1, 2, 4, 5, 6, 7 };
	fprintf(stdout, "\n==========================================\n");
	for (int i = 0; i < 8; ++i) {
		fprintf(stdout, "%s -> %04x\n", reg_to_str[idxs[i] + 8], registers[idxs[i]].data);
	}

	for (int i = 0; i < 4; ++i) {
		fprintf(stdout, "%s -> %04x\n", sr_to_str[i], seg_regs[i]);
	}
}

SegmentRegister* get_srs() {
	return seg_regs;
}

SegmentRegister get_sr(SegmentRegisterName sr) {
	return seg_regs[static_cast<int>(sr)];
}

void set_sr(SegmentRegisterName sr, uint16_t data) {
	seg_regs[static_cast<int>(sr)] = data;
}

} // namespace emu8086
