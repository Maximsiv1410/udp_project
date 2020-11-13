#include <boost/asio.hpp>
#include "../Network/rtp/rtp.hpp"
#include <iostream>

using namespace boost;
using namespace net;

struct peer {
	std::string name;
	asio::ip::udp::endpoint remote;
};

struct session {
	peer caller;
	peer callee;

	std::atomic<bool> is_alive;
	std::atomic<bool> is_finished;

	bool is_participant(const asio::ip::udp::endpoint & ep) {
		if (ep == caller.remote || ep == callee.remote) {
			return true;
		}
		else return false;
	}

	// get info about person to send data
	asio::ip::udp::endpoint & partner(asio::ip::udp::endpoint & ep) {
		if (caller.remote != ep) return caller.remote;
		if (callee.remote != ep) return callee.remote;
	}


};
