#include "media_server.hpp"


int main() {
	media_server server(4, 5060, 45777);
	server.start();

	std::cin.get();
	return 0;
}