
#include <boost/asio.hpp>
using namespace boost;

#include <thread>
#include <memory>
#include <string>
#include <iostream>

#include "../Network/sip/sip.hpp"
#include "server.hpp"





std::map<std::string, asio::ip::udp::endpoint> dbase; // to hold info about 'registered' clients



std::atomic<unsigned long long> counter{0};
void back(sip_server& caller, sip::message && msg) {
	if (msg.type() == sip::sip_type::Request) {
		sip::request && req = (sip::request&&)std::move(msg);

		if (req.method() == "REGISTER") {

			// sad and bad, but it's temporarily
			sip::response resp;
			resp.set_version("SIP/2.0");
			resp.set_code(200);
			resp.set_status("OK");
			resp.set_remote(req.remote());;

			dbase[req.headers()["To"]] = req.remote();
			std::cout << "registered " << dbase[req.headers()["To"]] << '\n';

			sip::message_wrapper wrapper(std::make_unique<sip::response>(std::move(resp)));
			caller.async_send(wrapper);
		}
		else if(req.method() == "INVITE") {
			// redirect request to called user
			if (dbase.find(req.headers()["To"]) != dbase.end()) {
				req.set_remote(dbase[req.headers()["To"]]);
				std::cout << "Readdressing INVITE\n";
				sip::message_wrapper wrapper(std::make_unique<sip::request>(std::move(req)));
				caller.async_send(wrapper);
			}
		}
		else {
			// process logic of session
		}


	} 
	else if (msg.type() == sip::sip_type::Response) {
		sip::response && resp = (sip::response &&)std::move(msg);
		
		if (dbase.count(resp.headers()["To"])) {
			resp.set_remote(dbase[resp.headers()["To"]]);
			std::cout << "response to " << resp.headers()["To"] << '\n';
			sip::message_wrapper wrapper(std::make_unique<sip::response>(std::move(resp)));
			caller.async_send(wrapper);
		}
	}
}


int main() {
	setlocale(LC_ALL, "ru");

	sip_server server(4, 5060);


	server.set_callback(
	[&server](sip::message_wrapper && wrappie)
	{
		back(server, std::move(*wrappie.storage()));
	});

	server.start(true);


	std::cin.get();
	server.stop();
	return 0;
}




