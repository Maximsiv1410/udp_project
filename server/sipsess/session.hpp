#include <boost/asio.hpp>
#include "../../Network/rtp/rtp.hpp"
#include <iostream>

using namespace boost;
using namespace net;

enum class session_status {
	pending,
	happening,
	finished
};

struct peer {
	std::string name;
	asio::ip::udp::endpoint remote;
};

class session {

	asio::ip::udp::endpoint caller; // sip
	asio::ip::udp::endpoint callee; // sip



	session_status status; // or atomic?

public:
	session(const asio::ip::udp::endpoint & caller, const asio::ip::udp::endpoint & callee) 
	: caller(caller)
	, callee(callee)
	, status(session_status::pending)
	{

	}

	session(session&&) = default;

	bool is_participant(const asio::ip::udp::endpoint & ep) {
		if (ep == caller || ep == callee) {
			return true;
		}
		else return false;
	}

	// get info about person to send data
	asio::ip::udp::endpoint & partner(const asio::ip::udp::endpoint & ep) {
		if (caller != ep) return caller;
		if (callee != ep) return callee;
	}

	bool alive() {
		return status == session_status::happening;
	}

	void set_status(session_status stat) {
		status = stat;
	}

};
