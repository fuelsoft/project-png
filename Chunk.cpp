/* 
CHUNK.CPP
NICK WILSON
2019
*/

#include "Chunk.hpp"

/* Constructor */
Chunk::Chunk(uint32_t length, uint32_t type, std::vector<uint8_t> data, uint32_t crc) {
	this->length = length;
	this->type = type;
	this->data = data;
	this->crc = crc;
}

Chunk::~Chunk() {
	//nothing to do here yet
}

/* Validate CRC */
bool Chunk::validate() {
	return true;
}

/* Debug Chunk details prinout */
std::string Chunk::format() {
	std::string r = "SIZE: "
					+ std::to_string(length) 
					+ "\nTYPE: \"" 
					+ ((char *) &type)[0]
					+ ((char *) &type)[1]
					+ ((char *) &type)[2]
					+ ((char *) &type)[3] 
					+ "\"\nCRC: " 
					+ std::to_string(crc);
	return r;
}

std::string Chunk::name() {
	std::string r;
	r.push_back(((char *) &type)[0]);
	r.push_back(((char *) &type)[1]);
	r.push_back(((char *) &type)[2]);
	r.push_back(((char *) &type)[3]);
	return r;
}
