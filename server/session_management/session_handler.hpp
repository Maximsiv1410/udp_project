#pragma once
#include <boost/asio.hpp>
#include <iostream>

#include "../../Network/sip/sip.hpp"
#include "session.hpp"

using namespace boost;
using namespace net;

class session_handler {
	std::vector<session> sessions; // thread safe ?
	// std::map<std::string, void(session_handler::*)(sip::request &)>
	// std::map<std::string, void(session_handler::*)(sip::response &)>

public:

	bool has_session(int endpoint) {
		//
	}

	void process(sip::message & msg) {
		if (msg.type() == sip::sip_type::Request) {
			on_request((sip::request&)msg);
		}
		else if(msg.type() == sip::sip_type::Response) {
			on_response((sip::response&)msg);
		}
	}

	void on_request(sip::request & request) {
		// process
	}

	void on_response(sip::response & response) {
		// process
	}

	//////////////// handlers ///////////////////

	void tracked_invite(sip::request & request) {

	}

	void tracked_ack(sip::request & request) {

	}

	void tracked_bye(sip::request & request) {

	}

	//////////////////////////////////////////////////
	void tracked_trying(sip::response & response) {

	}

	void tracked_ringing(sip::response & response) {

	}

	void on_ok(sip::response & response) {
		
	}
};
