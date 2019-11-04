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
	uint8_t leading_bytes_8[8];
	uint64_t position;
	uint32_t length, type, crc;

	vector<Chunk> chunks;

	if (argc != 2) {
		cerr << "Usage: ./png <filename>" << endl;
		return 1;
	}

	ifstream input(argv[1], ios::binary | ios::in);

	if (!input.is_open()) {
		cerr << "Could not read file \"" << argv[1] << "\"" << endl;
		return 2;
	}

	input.read(reinterpret_cast<char *>(&leading_bytes_8), 8);

	/* File should lead with [89 50 4E 47 0D 0A 1A 0A] by RFC 2083 */
	for (int i = 0; i < 8; i++) {
		if (leading_bytes_8[i] != PNG_SIGNATURE[i]) {
			cerr << "Invalid PNG signature. Is the file corrupted or not a PNG?" << endl;
			return 3;
		}
	}

	cout << "Valid PNG" << endl;

	/* Add IHDR Chunk */
	/* TODO: Error checking here */
	length		= read32_i(input);
	type		= read32(input);
	position	= input.tellg();
	input.seekg(position + length);
	crc 		= read32_i(input);

	Chunk c(length, type, position, crc);
	cout << c.print() << endl;

	return 0;
}
