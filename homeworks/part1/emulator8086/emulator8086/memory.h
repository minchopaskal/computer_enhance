#pragma once

#include <cstdint>

namespace emu8086 {

enum class RegisterName {
	AL = 0, CL, DL, BL, AH, CH, DH, BH,
	AX, CX, DX, BX, SP, BP, SI, DI,
};

enum class SegmentRegisterName {
	ES = 0,
	CS,
	SS,
	DS,
};

enum class EffectiveAddress {
	BX_SI = 0,
	BX_DI,
	BP_SI,
	BP_DI,
	SI,
	DI,
	BP,
	BX,
};

static const char *reg_to_str[16] = {
	"al", "cl", "dl", "bl", "ah", "ch", "dh", "bh",
	"ax", "cx", "dx", "bx", "sp", "bp", "si", "di",
};

static const char *eff_addr_to_str[8] = {
	"bx + si",
	"bx + di",
	"bp + si",
	"bp + di",
	"si",
	"di",
	"bp",
	"bx",
};

static const char *sr_to_str[4] = {
	"es", "cs", "ss", "ds"
};

struct Register {
	union {
		uint16_t data = 0;

		struct {
			uint8_t low;
			uint8_t high;
		};
	};
};

using SegmentRegister = uint16_t;

Register *get_registers();
/**
 * short registers's data is returned in the LSB
 */
uint16_t get_register_data(RegisterName reg);
void set_register(RegisterName reg, uint16_t data);

SegmentRegister *get_srs();
SegmentRegister get_sr(SegmentRegisterName sr);
void set_sr(SegmentRegisterName sr, uint16_t data);

void print_registers();

} // namespace emu8086
