#include <boost/asio.hpp>

#include <iostream>

#include "client_session.hpp"



using namespace boost;
using namespace net;






int main(int argc, char ** argv) {
	if (argc < 2) { std::cerr << "usage: ./client.out <your_uri> <remote_uri>"; return 1;}

	std::string user{argv[1]};

	client_session session(user);

	setlocale(LC_ALL, "ru");


	std::string command;
	std::string aux;
	while(true) {
		std::cin >> command;

		if (command == "invite") {
			std::cin >> aux;
			session.invite(aux);
		}
		else if (command == "register") {
			session.do_register();
		}
		else if(command == "bye") {
			session.bye();
		}
		else if (command == "exit") {
			break;
		}

	}
	

	std::cin.get();

	return 0;

}