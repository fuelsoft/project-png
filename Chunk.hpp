/* 
CHUNK.HPP
NICK WILSON
2019
*/

#ifndef OBJ_CHUNK
#define OBJ_CHUNK

#include <string>
#include <cmath>

class Chunk{
private:
	bool public_;

public:
	uint32_t length;
	uint32_t type;
	uint64_t offset;
	uint32_t crc;

	Chunk() {}
	Chunk(uint32_t length, uint32_t type, uint64_t offset, uint32_t crc);

	bool validate();
	std::string print();
};

#endif