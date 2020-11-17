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
	mutable std::mutex mtx;

	asio::ip::udp::endpoint caller;
	asio::ip::udp::endpoint callee;

	session_status status; // or atomic?

public:
	session(asio::ip::udp::endpoint & caller, asio::ip::udp::endpoint & callee) 
	: caller(caller)
	, callee(callee)
	, status(session_status::pending)
	{

	}

	bool is_participant(const asio::ip::udp::endpoint & ep) {
		std::lock_guard<std::mutex> lock(mtx);
		if (ep == caller || ep == callee) {
			return true;
		}
		else return false;
	}

	// get info about person to send data
	asio::ip::udp::endpoint & partner(asio::ip::udp::endpoint & ep) {
		std::lock_guard<std::mutex> lock(mtx);
		if (caller != ep) return caller;
		if (callee != ep) return callee;
	}

	bool alive() {
		std::lock_guard<std::mutex> lock(mtx);
		return status == session_status::happening;
	}

};
