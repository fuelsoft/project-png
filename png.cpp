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
#include <time.h>
#include <utime.h>

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

/* By trial and error, this seems to be about the biggest size permitted by most programs */
/* This seems to contradict the spec which states it may be up to 2^31 - 1 */
/* Programs should just skip these chunks but they don't. Instead, they crash :D */
const uint32_t CHUNK_SIZE_DATA_MAX = 0x700000;

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

/* Debug toggle */
bool print_debug = false;

/* Program mode */
uint8_t mode = 0;

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

/* Take pre-defined string type and return four bytes */
uint32_t as_type(string str) {
	if (str.length() < 4) return 0;
	return (str[3] << 24) + (str[2] << 16) + (str[1] << 8) + str[0];
}

void print_usage() {
	cout << "Usage:" << endl;
	cout << "\tAnalyze:     ./png [-a] [-d] <input>" << endl;
	cout << "\tInsertion:   ./png  -i  [-d] <input> <target> <output>" << endl;
	cout << "\tExtraction:  ./png  -e  [-d] <input>" << endl;
	cout << "Flags:" << endl;
	cout << "\th: Show [H]elp" << endl;
	cout << "\td: Enable [D]ebug printouts" << endl;
	cout << "\ta: [A]nalyze mode" << endl;
	cout << "\ti: [I]nsertion mode" << endl;
	cout << "\te: [E]xtraction mode" << endl;
	return;
}

int main(int argc, char const *argv[]) {
	uint8_t chunk_leading_bytes[8];
	uint32_t chunk_type, chunk_crc, chunk_length;
	uint64_t png_filesize;
	string png_filename;

	vector<uint8_t> chunk_data;
	vector<Chunk> chunks;
	vector<string> filenames;

	/* Process flags */
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			switch (argv[i][1]) {
				case 'a':
					mode = 0;
				case 'd':
					print_debug = true;
					break;
				case 'i':
					mode = 1;
					break;
				case 'e':
					mode = 2;
					break;
				case 'h':
					/* help */
					print_usage();
					return 1;
				case '\0':
					/* '-' is not a flag */
					print_usage();
					return 1;
				default:
					/* Other invalid flag */
					cerr << "Invalid flag \'-" << argv[i][1] << "\'" << endl;
					print_usage();
					return 1;
			}
		}
		else {
			filenames.emplace_back(argv[i]);
		}
	}

	/* Analysis mode */
	if (mode == 0 && filenames.size() != 1) {
		cerr << "Invalid arguments!" << endl;
		print_usage();
		return 1;
	}

	/* Insertion mode */
	else if (mode == 1 && filenames.size() != 3) {
		cerr << "Invalid arguments!" << endl;
		print_usage();
		return 1;
	}

	/* Extraction mode */
	else if (mode == 2 && filenames.size() != 1) {
		cerr << "Invalid arguments!" << endl;
		print_usage();
		return 1;
	}

	/* Read file details from first file */
	struct stat file_A;
	png_filename = filenames[0];

	if (stat(png_filename.c_str(), &file_A)) {
		cerr << "Could not read file details for \"" << png_filename << "\"" << endl;
		return 1;
	}

	png_filesize = file_A.st_size;

	/* Open first file (PNG) */
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

	if (print_debug) cout << "PNG signature validated!" << endl;
	if (print_debug) cout << "Filesize: " << png_filesize << " bytes\n" << endl;

	/* Read all of PNG into memory and break it into chunks */
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
		if (print_debug) {
			cout << "Chunk Type: " << chunks[chunks.size() - 1].name() << " | Length: " << chunk_length << " bytes" << endl;
		}
	}

	input_A.close();
	if (print_debug) cout << "Chunk count: " << chunks.size() << "\n" << endl;

	if (print_debug) cout << "Validating chunks..." << endl; 
	for (uint32_t i = 0; i < chunks.size(); i++) {
		if (!chunks[i].validate()) {
			cerr << "Chunk " << i << " failed validation!" << endl;
			return 1;
		}
	}
	if (print_debug) cout << "Chunks all validated!" << endl;

	uint32_t idx_pos = 0;
	uint32_t dat_pos = 0;

	/* Look for index chunk, if present */
	for (int i = 0; i < chunks.size(); i++) {
		if (chunks[i].name() == CHUNK_TYPE_INDEX) {
			idx_pos = i;
			break;
		}
	}

	/* Search for data chunk, which must come after index chunk */
	for (int i = idx_pos; i < chunks.size(); i++) {
		if (chunks[i].name() == CHUNK_TYPE_FILE) {
			dat_pos = i;
			break;
		}
	}

	/* Extraction Mode */
	if (mode == 2) {

		if (print_debug) cout << endl;

		/* Index position still 0, which is impossible as IHDR must be first */
		if (!idx_pos) {
			cerr << "No index chunk present!" << endl;
			return 1;
		}

		if (print_debug) cout << "Index chunk located: " << idx_pos << endl;

		uint32_t file_time_cr, file_time_mod;
		string out_filename;

		/* Error checking */
		if (chunks[idx_pos].length <= 2 * sizeof(uint32_t) + sizeof(uint64_t)) {
			cerr << "Empty filename!" << endl;
			return 1;
		}

		/* Build filename */
		for (int i = (2 * sizeof(uint32_t) + sizeof(uint64_t)); i < chunks[idx_pos].length; i++) {
			out_filename += chunks[idx_pos].data.data()[i];
		}

		if (print_debug) cout << "Filename located: \"" << out_filename << "\"" << endl;

		/* Extract file creation and modification time */
		memcpy(&file_time_cr, chunks[idx_pos].data.data(), sizeof(uint32_t));
		memcpy(&file_time_mod, chunks[idx_pos].data.data() + sizeof(uint32_t), sizeof(uint32_t));

		/* Data chunk was not found */
		if (!dat_pos) {
			cerr << "No file data chunk present!" << endl;
			return 1;
		}

		if (print_debug) cout << "File data chunk located: " << dat_pos << endl;

		/* Count how many chunks the file is split across */
		uint32_t file_chunk_count = dat_pos;
		while (chunks[file_chunk_count].name() == CHUNK_TYPE_FILE) {
			file_chunk_count++;
			if (file_chunk_count + 1 == chunks.size()) break;
		}
		file_chunk_count -= dat_pos;

		if (print_debug) cout << "File split across " << file_chunk_count << " file chunks" << endl;

		/* Avoid overwriting an existing file */
		out_filename += "_EX";

		ofstream output_D(out_filename, ios::binary | ios::out);

		if (!output_D.is_open()) {
			cerr << "Could not extract file \"" << out_filename << "\"" << endl;
			return 1;
		}

		/* Dump data chunk data into file */
		for (uint32_t i = 0; i < file_chunk_count; i++) {
			output_D.write(reinterpret_cast<const char *>(chunks[dat_pos+i].data.data()), chunks[dat_pos+i].data.size());
		}

		output_D.close();

		/* Write creation and modification time to file */
		struct utimbuf out_time;
		out_time.actime = file_time_cr;
		out_time.modtime = file_time_mod;

		if (utime(out_filename.c_str(), &out_time)) {
			cout << "Operation completed, but could not write file creation/modification time to file." << endl;
		}

		return 0;
	}

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
	depth =         chunks[0].data[8];
	colour =        chunks[0].data[9];
	compression =   chunks[0].data[10];
	filter =        chunks[0].data[11];
	interlace =     chunks[0].data[12];

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
	if (print_debug) {
		cout << "\nImage data:" << endl;
		cout << "Height: " << height << "px" << endl;
		cout << "Width: " << width << "px" << endl;
		cout << "Bit depth: " << (int) depth << " bits/channel" << endl;
		cout << "Colour type: " << (int) colour << " [" << PNG_TYPES_COLOUR[colour] << "]" << endl;
		cout << "Compression method: " << (int) compression << " [" << PNG_TYPES_COMPRESSION[compression] << "]" << endl;
		cout << "Filter method: " << (int) filter << " [" << PNG_TYPES_FILTER[filter] << "]" << endl;
		cout << "Interlace method: " << (int) interlace << " [" << PNG_TYPES_INTERLACE[interlace] << "]" << endl;
	}

	/* Analysis mode */
	/* Since there is no writing to be done, terminate here */
	if (mode == 0) {
		if (print_debug) {
			cout << endl;
			cout << "Index chunk" << ((idx_pos) ? " DOES " : " DOES NOT ") << "exist!" << endl;
			cout << "File chunks" << ((dat_pos) ? " DO " : " DO NOT ") << "exist!" << endl;
			cout << "You WILL" << ((idx_pos && dat_pos) ? " NOT " : " ") << "be able to insert a file into this image!" << endl;
		}
		return 0;
	}

	if (idx_pos && dat_pos) {
		cerr << "Index and file data already exist in input file." << endl;
		return 1;
	}
	else if (dat_pos) {
		cerr << "File data already exists in input file." << endl;
		return 1;
	}
	else if (idx_pos) {
		cerr << "Index already exists in input file." << endl;
		return 1;
	}

	/* Process target file */
	string file_filename = filenames[1];

	/*  This should never be triggered. 
		No modern FS supports filenames this long.
		This is here to enforce a sane limit to the length of filenames 
		so that the index chunk doesn't overflow the 32 bit length value. */
	if (file_filename.length() > 0xFF) {
		cerr << "Somehow you've exceeed the maximum filename size. Congratulations." << endl;
		cerr << "Unfortunately, this filename is too long to encode." << endl;
		return 1;
	}

	if (print_debug) cout << "\nOpening target file \"" << file_filename << "\"" << endl;

	/* Extract file metadata */
	struct stat file_B;
	if (stat(file_filename.c_str(), &file_B)) {
		cerr << "Could not read file details for \"" << file_filename << "\"" << endl;
		return 1;
	}

	/* Grab filesize, file creation and modification times */
	uint64_t file_filesize = file_B.st_size;

	/* 	These may need to be a larger type, time_t is not consistant
		across operating systems and appears to range from 32 to 64
		bytes (less relevant, but it can also be int or float). */
	uint32_t file_time_cr  = file_B.st_ctime;
	uint32_t file_time_mod = file_B.st_mtime;

	if (print_debug) cout << "Filesize: " << file_filesize << " bytes" << endl;

	uint32_t required_chunks = ceil(file_filesize / (float) CHUNK_SIZE_DATA_MAX);

	/* Test if file exceeds size limit for a single chunk */
	if (file_filesize > CHUNK_SIZE_DATA_MAX && print_debug) {
		cout << "File \"" << file_filename << "\" will span multiple chunks due to size." << endl;
		cout << "Chunks required: " << required_chunks << endl;
	}

	/* Open file stream to read data */
	ifstream input_B(file_filename, ios::binary | ios::in);
	if (!input_B.is_open()) {
		cerr << "Could not read file \"" << file_filename << "\"" << endl;
		return 1;
	}

	//For now, assume only one data chunk required
	vector<uint8_t> file_data;

	/* Hard 4 GB cap.
		This can be extended without any real issue but keep in mind 
		every additional bit will require at least 1 more bit of memory. */
	if (file_filesize > 0xFFFFFFFF) {
		cerr << "File too large to be loaded into memory!" << endl;
		return 1;
	}

	uint64_t data_remaining = file_filesize;
	uint32_t chunks_created = 0;

	/* Read in as much as possible in the biggest chunk size */
	while (data_remaining > CHUNK_SIZE_DATA_MAX) {
		if (print_debug) cout << "Creating chunk " << chunks_created << "... ";
		data_remaining -= CHUNK_SIZE_DATA_MAX; //decrease remaining data by chunk size
		chunks_created++; //increment chunk count

		file_data.resize(CHUNK_SIZE_DATA_MAX); //this is a complete chunk so max size
		input_B.read(reinterpret_cast<char *>(file_data.data()), CHUNK_SIZE_DATA_MAX);    

		Chunk file(file_data.size(), as_type(CHUNK_TYPE_FILE), move(file_data), 0);
		file.force_crc_update(); // looking for why this loop is slow? it's probably this
		chunks.insert(chunks.begin() + chunks_created, file);

		file_data.clear();

		if (print_debug) cout << "done!" << endl;
	}

	/* Dump whatever is left into a smaller chunk */
	if (data_remaining) { //don't add an empty chunk though
		if (print_debug) cout << "Creating chunk " << chunks_created << "... ";
		chunks_created++;

		file_data.resize(data_remaining);
		input_B.read(reinterpret_cast<char *>(file_data.data()), data_remaining);    

		Chunk file(file_data.size(), as_type(CHUNK_TYPE_FILE), move(file_data), 0);
		file.force_crc_update();
		chunks.insert(chunks.begin() + chunks_created, file);
		if (print_debug) cout << "done!" << endl;
	}

	input_B.close();

	/* Create index chunk */
	vector<uint8_t> idx_data;

	/* Populate index chunk */
	idx_data.resize(sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint64_t) + file_filename.length());
	memcpy(idx_data.data(), &file_time_cr, sizeof(uint32_t));
	memcpy(idx_data.data() + sizeof(uint32_t), &file_time_mod, sizeof(uint32_t));
	memcpy(idx_data.data() + 2 * sizeof(uint32_t), &file_filesize, sizeof(uint64_t));
	memcpy(idx_data.data() + 4 * sizeof(uint32_t), file_filename.c_str(), file_filename.length());

	Chunk index(idx_data.size(), as_type(CHUNK_TYPE_INDEX), move(idx_data), 0);
	index.force_crc_update();

	ofstream output_C(filenames[2], ios::binary | ios::out);

	if (!output_C.is_open()) {
		cerr << "Could not open \"" << filenames[2] << "\" for writing!" << endl;
		return 1;
	}

	/* Figure out where to insert chunks */
	/* IHRD known to be leading chunk from above */
	chunks.insert(chunks.begin() + 1, index);

	vector<uint8_t> tmp;

	if (print_debug) cout << "Writing file to disk..." << endl;

	output_C.write(reinterpret_cast<const char *>(PNG_SIGNATURE), 8);

	for (uint32_t i = 0; i < chunks.size(); i++) {
		tmp = chunks[i].pack();
		output_C.write(reinterpret_cast<char *>(tmp.data()), tmp.size());
	}

	output_C.close();

	if (print_debug) cout << "Insertion completed successfully!" << endl;

	return 0;
}

/* TODO: 
	-Single instance of CRC table
	-Don't load large files, copy in chunks
	-Consider type for 'chunks' - would a linked list be more appropriate?
*/

/*
PROPOSED:

INDEX CHUNK:

4 byte unsigned length (per PNG spec)
4 byte type (per PNG spec)
N byte data:
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
