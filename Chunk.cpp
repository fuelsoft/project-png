/* 
CHUNK.CPP
NICK WILSON
2019
*/

#include "Chunk.hpp"

/* Constructor */
Chunk::Chunk(uint32_t length, uint32_t type, uint64_t offset, uint32_t crc) {
	this->length = length;
	this->type = type;
	this->offset = offset;
	this->crc = crc;
}

/* Validate CRC */
bool Chunk::validate() {
	return true;
}

/* Debug Chunk details prinout */
/* TODO: type printout not safe */
std::string Chunk::print() {
	return std::to_string(length) + " " + ((char *) &type) + " " + std::to_string(offset) + " " + std::to_string(crc);
}
