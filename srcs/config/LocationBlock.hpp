#pragma once

#include "config/Block.hpp"

class LocationBlock: public Block {
	public:
		LocationBlock(const Server &s);
		LocationBlock(const Location &l);
		~LocationBlock();
};