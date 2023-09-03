#include <cstdio>
#include <filesystem>

#include "decoder.h"

#include <filesystem>
#include <iostream>

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stdout, "Usage: %s <filename>\n", argv[0]);
		return 1;
	}

	const char *filename = argv[1];
	auto filesize = std::filesystem::file_size(filename);

	FILE *f = fopen(filename, "rb");
	if (f) {
		std::unique_ptr<uint8_t[]> source(new uint8_t[filesize]);
		auto bytes_read = fread(source.get(), sizeof(uint8_t), filesize, f);
		
		if (bytes_read != filesize) {
			fprintf(stderr, "Failed to read file %s!\n", filename);
			return 1;
		}

		emu8086::decode(source.get(), filesize);
	}

	return 0;
}