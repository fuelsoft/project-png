/* 
PNG.CPP
NICK WILSON
2019
*/

#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

#include <arpa/inet.h>

#include "Chunk.hpp"

const uint8_t PNG_SIGNATURE[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
const uint64_t PNG_MIN_SIZE = 0x00; //TODO: Calculate this

using namespace std;

/* Read 4 bytes and swap ordering */
int read32_i(ifstream &input) {
	uint32_t x;
	input.read(reinterpret_cast<char *>(&x), 4);
	return ntohl(x);
}

/* Read 4 bytes */
int read32(ifstream &input) {
	uint32_t x;
	input.read(reinterpret_cast<char *>(&x), 4);
	return x;
}

int main(int argc, char const *argv[]) {
	uint8_t chunk_leading_bytes[8];
	uint32_t chunk_type, chunk_crc;
	uint64_t png_filesize, chunk_length;

	vector<uint8_t> chunk_data;
	vector<Chunk> chunks;

	// char temp_char;

	if (argc != 2) {
		cerr << "Usage: ./png <filename>" << endl;
		return 1;
	}

	ifstream input(argv[1], ios::binary | ios::in | ios::ate);

	if (!input.is_open()) {
		cerr << "Could not read file \"" << argv[1] << "\"" << endl;
		return 2;
	}

	png_filesize = input.tellg();
	input.clear();
	input.seekg(0);

	if (png_filesize < PNG_MIN_SIZE) {
		cerr << "PNG file is too small. Is it corrupted?" << endl;
		return 3;
	}

	input.read(reinterpret_cast<char *>(&chunk_leading_bytes), 8);

	/* File should lead with [89 50 4E 47 0D 0A 1A 0A] by RFC 2083 */
	for (int i = 0; i < 8; i++) {
		if (chunk_leading_bytes[i] != PNG_SIGNATURE[i]) {
			cerr << "Invalid PNG signature. Is the file corrupted or not a PNG?" << endl;
			return 4;
		}
	}

	cout << "Valid PNG" << endl;
	cout << "Filesize: " << png_filesize << " bytes\n" << endl;

	while (input.tellg() < png_filesize) {
		/* Read Chunk */
		chunk_length = read32_i(input);
		chunk_type = read32(input);
		chunk_data.resize(chunk_length); // Ensure vector has enough space up front for copy
		input.read((char *) chunk_data.data(), chunk_length); // Copy Chunk Data data (hmm)
		chunk_crc = read32_i(input);

		/* Push new Chunk to Chunk vector */
		chunks.emplace_back(chunk_length, chunk_type, move(chunk_data), chunk_crc);

		/* Ensure data is cleared */
		chunk_data.clear();

		/* Debug - Chunk data printout */
		cout << "Chunk Type: " << chunks[chunks.size() - 1].name() << endl;
		cout << "Data length: " << chunk_length << " bytes" << endl;
	}

	cout << "Chunk count: " << chunks.size() << endl;

	return 0;
}
