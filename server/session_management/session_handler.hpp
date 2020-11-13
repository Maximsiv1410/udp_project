#pragma once
#include <boost/asio.hpp>
#include <iostream>


#include "../../Network/sip/sip.hpp"
#include "session.hpp"

using namespace boost;
using namespace net;

class session_handler {
	std::vector<session> sessions;

public:

	void process(sip::message & msg) {
		// may be parameter should be POINTER
		// to use dynamic_cast conversion to sip::request*
		// etc
		// we can use dynamic_cast with references
		// but it is overhead-way
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

	void on_invite() {

	}

	void on_ringing() {

	}

	void on_ok() {
		
	}
};
