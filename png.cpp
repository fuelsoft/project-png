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
#include <string.h>

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
const uint32_t CHUNK_SIZE_DATA_MAX = 0xFFFFFFFF;

/* General error checking */
const uint64_t PNG_MIN_SIZE = 0x39;
/*
SIGNATURE: 8
IHDR: 4 + 4 + N + 4
	N: 13
IDAT: 4 + 4 + N + 4
	N: 0
IEND: 4 + 4 + N + 4
	N: 0
TOTAL: 57 (0x39)
*/

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

uint32_t as_type(string str) {
	if (str.length() < 4) return 0;
	return (str[3] << 24) + (str[2] << 16) + (str[1] << 8) + str[0];
}

int main(int argc, char const *argv[]) {
	uint8_t chunk_leading_bytes[8];
	uint32_t chunk_type, chunk_crc, chunk_length;
	uint64_t png_filesize;
	string png_filename;

	vector<uint8_t> chunk_data;
	vector<Chunk> chunks;

	if (argc != 3) {
		cerr << "Usage: ./png <png filename> <target filename>" << endl;
		return 1;
	}

	struct stat file_A;
	png_filename = argv[1];

	if (stat(png_filename.c_str(), &file_A)) {
		cerr << "Could not read file details for \"" << png_filename << "\"" << endl;
		return 1;
	}

	png_filesize = file_A.st_size;

	ifstream input_A(png_filename, ios::binary | ios::in);

	if (!input_A.is_open()) {
		cerr << "Could not read file \"" << png_filename << "\"" << endl;
		return 1;
	}

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
	cout << "Chunk count: " << chunks.size() << "\n" << endl;

	cout << "Validating chunks..." << endl; 
	for (int i = 0; i < chunks.size(); i++) {
		if (!chunks[i].validate()) {
			cerr << "Chunk " << i << " failed validation!" << endl;
			return 1;
		}
	}
	cout << "Chunks all validated!" << endl;

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

	/* Process target file */
	string file_filename = argv[2];

	/* 	This should never be triggered. 
	No modern FS supports filenames this long.
	This is here to enforce a sane limit to the length of filenames 
	so that the index chunk doesn't overflow the 32 bit length value.
	*/
	if (file_filename.length() > 0xFF) {
		cerr << "Somehow you've exceeed the maximum filename size. Congratulations." << endl;
		cerr << "Unfortunately, this filename is too long to encode." << endl;
		return 1;
	}

	cout << "\nOpening target file \"" << file_filename << "\"" << endl;

	/* Extract file metadata */
	struct stat file_B;
	if (stat(file_filename.c_str(), &file_B)) {
		cerr << "Could not read file details for \"" << file_filename << "\"" << endl;
		return 1;
	}

	uint64_t file_filesize = file_B.st_size;
	uint32_t file_time_cr  = file_B.st_ctime;
	uint32_t file_time_mod = file_B.st_mtime;

	cout << "Filesize: " << file_filesize << " bytes" << endl;

	/* Test if file exceeds size limit for a single chunk */
	/* TODO: Span multiple chunks */
	if (file_filesize > CHUNK_SIZE_DATA_MAX) {
		cout << "NOTE: \"" << file_filename << "\" will span multiple chunks due to size." << endl;
		cout << "NOTE: Chunks required: " << ceil(file_filesize / CHUNK_SIZE_DATA_MAX) << endl;
		return 1;
	}

	/* Open file stream to read data */
	ifstream input_B(file_filename, ios::binary | ios::in);
	if (!input_B.is_open()) {
		cerr << "Could not read file \"" << file_filename << "\"" << endl;
		return 1;
	}

	//For now, assume only one data chunk required
	vector<uint8_t> file_data;

	/* Hard cap at 1 GB */
	/* TODO: Deal with this limitation */
	if (file_filesize > 0x40000000) {
		cerr << "File too large to be loaded into memory!" << endl;
		return 1;
	}

	file_data.resize(file_filesize);
	input_B.read(reinterpret_cast<char *>(file_data.data()), file_filesize);	
	input_B.close();

	Chunk file(file_data.size(), as_type(CHUNK_TYPE_FILE), move(file_data), 0);
	file.force_crc_update();

	//Create index chunk
	vector<uint8_t> idx_data;

	/* Create index chunk */
	idx_data.resize(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t) + file_filename.length());
	memcpy(idx_data.data(), &file_time_cr, sizeof(uint32_t));
	memcpy(idx_data.data() + sizeof(uint32_t), &file_time_mod, sizeof(uint32_t));
	memcpy(idx_data.data() + 2 * sizeof(uint32_t), &file_filesize, sizeof(uint64_t));
	memcpy(idx_data.data() + 4 * sizeof(uint32_t), file_filename.c_str(), file_filename.length());

	Chunk index(idx_data.size(), as_type(CHUNK_TYPE_INDEX), move(idx_data), 0);
	index.force_crc_update();

	return 0;
}

/* TODO: 
	-Single instance of CRC table
	-Don't load large files, copy in chunks
	-Multiple file chunk support
*/

/*
PROPOSED:

INDEX CHUNK:

4 byte unsigned length (per PNG spec)
4 byte type (per PNG spec)
N byte data:
	8 byte filesize
	4 byte creation time
	4 byte modification time
	N byte filename
4 byte CRC (per PNG spec)

Limitations:
	Only one per image
	Before all file data chunks
	Only in legal positions - after IHDR, before IEND, etc.

FILE CHUNK:

4 byte unsigned length (per PNG spec)
4 byte type (per PNG spec)
N byte data
4 byte CRC (per PNG spec)

Limitations:
	Only in legal positions - after IHDR, before IEND, etc.
*/
