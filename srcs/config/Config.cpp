#include "config/Config.hpp"

std::vector<ServerBlock> Config::_servers;
std::map<int, std::map<const std::string, const ServerBlock *> > Config::_ordered_servers;
std::vector<int> Config::ports;
size_t Config::line = 0;
