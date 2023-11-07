#include <iostream>
#include "config/Config.hpp"
#include "RuntimeError.hpp"

int	main(int argc, char **argv)
{
	if (argc != 2) {
		std::cout << "Missing configuration file:\n" << "Usage: " << argv[0] << "config_file.conf" <<  std::endl;
	}
	try {
		Config::parseFile(argv[1]);
		Config::printConfiguration();
	} catch(RuntimeError &e) {
		std::cerr << "failed to parse configuration: " << e.what() << std::endl;
	}
}