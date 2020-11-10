
#include <boost/asio.hpp>
using namespace boost;

#include <thread>
#include <memory>
#include <string>
#include <iostream>

#include "../Network/sip/sip.hpp"
#include "server.hpp"


struct video_session {
    std::vector<std::string> clients;

    void create(std::vector<std::string> & participants) {
    	clients = std::move(participants);
    }

    bool is_participant(std::string name) {
        for (auto & user : clients) {
            if (name == user) return true;
        }
        return false;
    }
};


std::map<std::string, asio::ip::udp::endpoint> dbase; // to hold info about 'registered' clients
std::vector<video_session> sessions; // currently alive sessions



std::atomic<unsigned long long> counter{0};
void back(sip_server& caller, sip::message && msg) {
	if (msg.type() == sip::sip_type::Request) {
		sip::request && req = (sip::request&&)std::move(msg);

		if (req.method() == "REGISTER") {

			sip::response resp;
			resp.set_version("SIP/2.0")
			.set_code(200)
			.set_status("OK")
			.set_remote(req.remote());

			std::cout << req.remote() << "\n";
			dbase[req.headers()["To"]] = req.remote();
			std::cout << "registered " << dbase[req.headers()["To"]] << '\n';

			sip::message_wrapper wrapper(std::make_unique<sip::response>(std::move(resp)));
			caller.async_send(wrapper);
		}
		else if(req.method() == "INVITE") {
			// readdress request to called user
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
		// process
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




