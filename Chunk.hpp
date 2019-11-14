/* 
CHUNK.HPP
NICK WILSON
2019
*/

#ifndef OBJ_CHUNK
#define OBJ_CHUNK

#include <string>
#include <cmath>
#include <vector>

#include <string.h>
#include <arpa/inet.h>

class Chunk{
private:
	uint64_t crc_table[256];
	bool crc_table_computed = false;

	uint32_t length;
	uint32_t type;
	uint32_t crc;

	void make_crc_table();
	uint64_t update_crc(uint64_t crc, uint8_t *buf, uint32_t len);
	uint64_t calc_crc(uint8_t *buf, uint32_t len);
	uint64_t get_crc();

public:

	std::vector<uint8_t> data;

	Chunk() {}
	Chunk(uint32_t length, uint32_t type, std::vector<uint8_t> data, uint32_t crc);
	Chunk(uint32_t length, uint32_t type, std::vector<uint8_t> data);
	~Chunk();

	std::vector<uint8_t> pack();

	void force_crc_update();
	bool validate();
	std::string name();
};

#endif