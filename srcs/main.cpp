#include <iostream>
#include "config/Config.hpp"
#include "server/Server.hpp"
#include "RuntimeError.hpp"
#include "http.hpp"

struct args {
	bool debug;
	char *configpath;
};

void	set_flags(int argc, char **argv, args *f)
{
	f->debug = false;
	f->configpath = NULL;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (std::string(argv[i]) == std::string("-d"))
				f->debug = true;
		} else {
			f->configpath = argv[i];
		}
	}
}

int	main(int argc, char **argv)
{
	args f;

	set_flags(argc, argv, &f);
	if (f.configpath == NULL) {
		std::cout << "Missing configuration file:" << std::endl;
		std::cout << "Usage: " << argv[0] << "config_file.conf" <<  std::endl;
		std::cout << "Flags:" << std::endl;
		std::cout << "\t-d debug mode" << std::endl;
		return (0);
	}
	HTTP::init_errors();
	try {
		Config::parseFile(f.configpath);
		if (f.debug)
			Config::printConfiguration();
		const LocationBlock *c = Config::matchConfig(std::string("localhost"), "/boumbou/tatatititutu/pif.html", 8000);
		c->printConfiguration(0);
		std::cout << c->getFilepath(std::string("/boumbou/tatatititutu/pif.html")) << std::endl;
		/* Server s = Server(); */
		/* s.serve(); */
	} catch(RuntimeError &e) {
		std::cerr << "failed to parse configuration: " << e.what() << std::endl;
	} catch(std::runtime_error &e) {
		std::cerr << "runtime_error: " << e.what() << std::endl;
	}
}
