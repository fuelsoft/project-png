/* 
CHUNK.CPP
NICK WILSON
2019
*/

#include "Chunk.hpp"

/* Private */
/* CRC code adapted from reference CRC implementation */
/* See: https://tools.ietf.org/html/rfc2083#section-15 */
void Chunk::make_crc_table() {
	uint64_t c;

	for (uint64_t n = 0; n < 256; n++) {
		c = n;
		for (uint8_t k = 0; k < 8; k++) {
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}
		crc_table[n] = c;
	}
	crc_table_computed = true;
}

uint64_t Chunk::update_crc(uint64_t crc, uint8_t *buf, uint32_t len) {
	uint64_t c = crc;
	if (! crc_table_computed) make_crc_table();
	for (uint32_t n = 0; n < len; n++) c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
	return c;
}

uint64_t Chunk::calc_crc(uint8_t *buf, uint32_t len) {
	return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}

uint64_t Chunk::get_crc() {
	/* Problem: CRC runs over TYPE which is not present in my data vector */
	/* Solution: Append a copy of the vector data to a new vector starting with TYPE */
	std::vector<uint8_t> data_temp;
	data_temp.resize(sizeof(uint32_t) + data.size());
	/* Copy TYPE over */
	memcpy(data_temp.data(), &type, sizeof(uint32_t));
	/* Copy data over */
	memcpy(data_temp.data() + sizeof(uint32_t), data.data(), data.size());
	return calc_crc(data_temp.data(), data_temp.size());
}

/* Public */
/* Constructor */
Chunk::Chunk(uint32_t length, uint32_t type, std::vector<uint8_t> data, uint32_t crc) {
	this->length = length;
	this->type = type;
	this->data = data;
	this->crc = crc;
}

Chunk::Chunk(uint32_t length, uint32_t type, std::vector<uint8_t> data) {
	this->length = length;
	this->type = type;
	this->data = data;
	this->crc = get_crc();
}

Chunk::~Chunk() {
	//nothing to do here yet
}

/* Validate CRC */
bool Chunk::validate() {
	return crc == get_crc();
}

std::string Chunk::name() {
	std::string r;
	r.push_back(((char *) &type)[0]);
	r.push_back(((char *) &type)[1]);
	r.push_back(((char *) &type)[2]);
	r.push_back(((char *) &type)[3]);
	return r;
}
