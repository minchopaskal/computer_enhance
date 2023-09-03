#pragma once

#include "emu8086.h"

namespace emu8086 {

/**
 * @breif Output disassembled binary instructions to std output as asm
 */
void decode(const std::uint8_t *source, std::size_t source_size);

}