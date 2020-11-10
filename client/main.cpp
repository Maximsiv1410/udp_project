#include <boost/asio.hpp>

#include <iostream>

#include "client.hpp"
#include "../Network/sip/sip.hpp"


using namespace boost;
using namespace net;

std::string user;
std::string remote;

const char* meth = "INVITE";
const char* uri = "sip:smbd@lol.com";
const char*version = "SIP/2.0";
const char* left = "Content-Length";
const char* right = "5";
const char*body = "hello";


void back(sip_client& caller, sip::message && msg) { 
	if (msg.type() == sip::sip_type::Response) {
		sip::response && resp = (sip::response&&)std::move(msg);
		if(resp.code() == 200) {
			std::cout << "GOT 200 OK !!!!!\n";

			/*
			sip::request req;
			req.set_method(meth)
			.set_uri(uri)
			.set_version(version)
			.add_header(left, right)
			.set_body(body, strlen(body))
			.set_remote(caller.remote());

			sip::message_wrapper wrapper{std::make_unique<sip::request>(std::move(req))};
			caller.async_send(wrapper);
			*/
		}
	}
	else if(msg.type() == sip::sip_type::Request) {
		sip::request && req = (sip::request&&)std::move(msg);
		
		if (req.method() == "INVITE") {
			std::cout << "I WAS INVITED\n";
		}
	}
} 


int main(int argc, char ** argv) {
	if (argc < 3) { std::cerr << "usage: ./client.out <your_uri> <remote_uri>"; return 1;}

	user = std::string(argv[1]);
	remote = std::string(argv[2]);

	setlocale(LC_ALL, "ru");

	sip_client client("127.0.0.1", 1, 5060);

	sip::request req;
	req.set_method("REGISTER")
		.set_uri(user)
		.set_version(version)
		.add_header("Request-URI", "sip:myserver.kek")
		.add_header("To", user.c_str())
		.add_header("From", user.c_str())
		//.set_body(body, strlen(body))
		.set_remote(client.remote());


	client.set_callback([&client](sip::message_wrapper && wrappie) {
		back(client, std::move(*wrappie.storage()));
	});

	client.start(true);


	sip::message_wrapper wrapper(std::make_unique<sip::request>(std::move(req)));
	client.async_send(wrapper);
	
	/*
	std::string command;
	while(true) {
		std::cin >> command;

		if (command == "invite") {

		}
	}
	*/

	std::cin.get();

	return 0;

}