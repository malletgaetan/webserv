#include <iostream>
#include <signal.h>
#include "config/Config.hpp"
#include "server/Server.hpp"
#include "config/ConfigParsingException.hpp"
#include "http.hpp"

Server *g_server;

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

void sig_handler(int signum)
{
	(void)signum;
	g_server->stop();
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
	HTTP::init_maps();
	try {
		Config::parseFile(f.configpath);
		if (f.debug)
			Config::printConfiguration();
		g_server = new Server();
		signal(SIGINT, &sig_handler);
		g_server->serve();
	} catch(ConfigParsingException &e) {
		std::cerr << "failed to parse configuration: " << e.what() << std::endl;
	} catch(std::runtime_error &e) {
		std::cerr << "runtime_error: " << e.what() << std::endl;
	}
	delete g_server;
}
