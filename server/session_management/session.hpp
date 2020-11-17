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
	peer caller;
	peer callee;

	std::atomic<session_status> is_alive;

public:
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

	bool alive() {
		return is_alive.load(std::memory_order_relaxed);
	}

};
