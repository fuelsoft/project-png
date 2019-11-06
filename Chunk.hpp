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

class Chunk{
private:
	// bool public_;

public:
	uint32_t length;
	uint32_t type;
	std::vector<uint8_t> data;
	uint32_t crc;

	Chunk() {}
	Chunk(uint32_t length, uint32_t type, std::vector<uint8_t> data, uint32_t crc);
	~Chunk();

	bool validate();
	std::string format();
	std::string name();
};

#endif