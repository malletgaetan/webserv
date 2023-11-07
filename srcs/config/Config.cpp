#include "config/Config.hpp"

std::vector<ServerBlock> Config::_server_blocks;
int Config::_nb_workers = 0;
size_t Config::line = 0;

int get_num_cpu_cores(void)
{
    std::ifstream cpuinfo("/proc/cpuinfo");
    int numCores = 0;
    std::string line;

    while (std::getline(cpuinfo, line)) {
        if (line.substr(0, 9) == "processor") {
            numCores++;
        }
    }

    return numCores;
}
