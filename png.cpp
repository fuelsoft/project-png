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
#include <sys/stat.h>

#include "Chunk.hpp"

/* PNGs MUST have this as their leading bytes by RFC 2083 */
const uint8_t PNG_SIGNATURE[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

/* PNGs MUST report these types in HDR by RFC 2083 */
const std::string PNG_TYPES_COLOUR[] = {"GREYSCALE", "INVALID", "COLOUR", "PALLET", "GREYSCALE+ALPHA", "INVALID", "COLOUR+ALPHA"};
const std::string PNG_TYPES_COMPRESSION[] = {"DEFLATE W/ 32K WINDOW"};
const std::string PNG_TYPES_FILTER[] = {"NONE"};
const std::string PNG_TYPES_INTERLACE[] = {"NONE", "ADAM7"};

/* PNG writing constants */
const std::string CHUNK_TYPE_INDEX = "fiDX";
const std::string CHUNK_TYPE_FILE = "fiLE";

/* General error checking */
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
	uint32_t chunk_type, chunk_crc, chunk_length;
	uint64_t png_filesize;

	vector<uint8_t> chunk_data;
	vector<Chunk> chunks;

	if (argc != 3) {
		cerr << "Usage: ./png <png filename> <target filename>" << endl;
		return 1;
	}

	ifstream input_A(argv[1], ios::binary | ios::in | ios::ate);

	if (!input_A.is_open()) {
		cerr << "Could not read file \"" << argv[1] << "\"" << endl;
		return 1;
	}

	/* TODO: Get this from stat call instead */
	png_filesize = input_A.tellg();
	input_A.clear();
	input_A.seekg(0);

	if (png_filesize < PNG_MIN_SIZE) {
		cerr << "PNG file is too small. Is it corrupted?" << endl;
		return 1;
	}

	input_A.read(reinterpret_cast<char *>(&chunk_leading_bytes), 8);

	/* File should lead with [89 50 4E 47 0D 0A 1A 0A] by RFC 2083 */
	for (int i = 0; i < 8; i++) {
		if (chunk_leading_bytes[i] != PNG_SIGNATURE[i]) {
			cerr << "Invalid PNG signature. Is the file corrupted or not a PNG?" << endl;
			return 1;
		}
	}

	cout << "Valid PNG" << endl;
	cout << "Filesize: " << png_filesize << " bytes\n" << endl;

	while (input_A.tellg() < png_filesize) {
		/* Read Chunk */
		chunk_length = read32_i(input_A);

		/* Confirm it doesn't run past the end of the file */
		if ((int) input_A.tellg() + chunk_length + sizeof(chunk_crc) >= png_filesize) {
			cerr << "Reached EOF before all chunks were loaded. Is the PNG corrupted?" << endl;
			return 1;
		}

		/* Ensure vector has enough space up front for copy */
		chunk_data.resize(chunk_length);
		chunk_type = read32(input_A);
		input_A.read(reinterpret_cast<char *>(chunk_data.data()), chunk_length); // Copy Chunk Data data (hmm)
		chunk_crc = read32_i(input_A);

		/* Push new Chunk to Chunk vector */
		chunks.emplace_back(chunk_length, chunk_type, move(chunk_data), chunk_crc);

		/* Ensure data is cleared */
		chunk_data.clear();

		/* Debug - Chunk data printout */
		cout << "Chunk Type: " << chunks[chunks.size() - 1].name() << endl;
		cout << "Data length: " << chunk_length << " bytes" << endl;
	}

	input_A.close();
	cout << "Chunk count: " << chunks.size() << endl;

	/* First chunk MUST be of type IHDR by RFC 2083 */
	/* IHDR chunk MUST have 13 bytes of data */
	if (chunks[0].name() != "IHDR" || chunks[0].data.size() != 13) {
		cerr << "Invalid leading chunk!" << endl;
		return 1;
	}

	uint32_t width, height;
	uint8_t depth, colour, compression, filter, interlace;

	width =  (chunks[0].data[0] << 24) + (chunks[0].data[1] << 16) + (chunks[0].data[2] << 8) + (chunks[0].data[3]);
	height = (chunks[0].data[4] << 24) + (chunks[0].data[5] << 16) + (chunks[0].data[6] << 8) + (chunks[0].data[7]);
	depth = 		chunks[0].data[8];
	colour = 		chunks[0].data[9];
	compression = 	chunks[0].data[10];
	filter = 		chunks[0].data[11];
	interlace = 	chunks[0].data[12];

	/* error checking */
	if (!width) { //width is 0, not valid
		cerr << "Invalid image width!" << endl;
		return 1;
	}
	if (!height) { //height is 0, not valid
		cerr << "Invalid image height!" << endl;
		return 1;
	}
	if (!(depth == 1 || depth == 2 || depth == 4 || depth == 8 || depth == 16)) {
		cerr << "Invalid bit depth!" << endl;
		return 1;
	}
	if (!(colour == 0 || colour == 2 || colour == 3 || colour == 4 || colour == 6)) {
		cerr << "Invalid colour type!" << endl;
		return 1;
	}
	if (compression) { //zero is the only valid option
		cerr << "Invalid compression method!" << endl;
		return 1;
	}
	if (filter) { //zero is the only valid option
		cerr << "Invalid filter method!" << endl;
		return 1;
	}
	if (interlace > 1) { //zero and one are the only valid options
		cerr << "Invalid interlace method!" << endl;
		return 1;
	}

	/* debug */
	cout << "\nImage data:" << endl;
	cout << "Height: " << height << "px" << endl;
	cout << "Width: " << width << "px" << endl;
	cout << "Bit depth: " << (int) depth << " bits/channel" << endl;
	cout << "Colour type: " << (int) colour << " [" << PNG_TYPES_COLOUR[colour] << "]" << endl;
	cout << "Compression method: " << (int) compression << " [" << PNG_TYPES_COMPRESSION[compression] << "]" << endl;
	cout << "Filter method: " << (int) filter << " [" << PNG_TYPES_FILTER[filter] << "]" << endl;
	cout << "Interlace method: " << (int) interlace << " [" << PNG_TYPES_INTERLACE[interlace] << "]" << endl;

	/* process file 2 */
	cout << "\nOpening target file \"" << argv[2] << "\"" << endl;

	ifstream input_B(argv[2], ios::binary | ios::in | ios::ate);

	if (!input_B.is_open()) {
		cerr << "Could not read file \"" << argv[2] << "\"" << endl;
		return 1;
	}

	struct stat result;

	if (stat(argv[2], &result)) {
		cerr << "Could not read file details for \"" << argv[2] << "\"" << endl;
		return 1;
	}

	input_B.close();
	return 0;
}
