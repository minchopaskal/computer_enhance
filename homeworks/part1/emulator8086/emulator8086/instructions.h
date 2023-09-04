#pragma once

#include "emu8086.h"

namespace emu8086 {

enum Instruction : uint8_t {
	REGMEM_TOFROM_REG = 0x22,
	IMM_TO_REGMEM = 0x63,
	IMM_TO_REG = 0xB,
	MEM_TO_ACC = 0x50,
	ACC_TO_MEM = 0x51,
};

}