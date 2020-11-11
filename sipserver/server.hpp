#pragma once
#include <boost/asio.hpp>
using namespace boost;

#include "../Network/sip/sip.hpp"

using namespace net;

class sip_server : public sip::server {
	using work_ptr = std::unique_ptr<asio::io_context::work>;
	using work_entity = asio::io_context::work;

	asio::io_context ios;
	work_ptr work;
	asio::ip::udp::endpoint endpoint;
	asio::ip::udp::socket sock;

	std::vector<std::thread> pool;

public:
	sip_server(std::size_t threads, std::size_t port = 5060)
		: ios{},
		sip::server(ios, sock),
		endpoint(asio::ip::udp::v4(), port),
		sock(ios, endpoint),
		work(new work_entity(ios))
		
	{
		for (std::size_t i = 0; i < threads; i++) {
			pool.push_back(std::thread([this]() { ios.run(); }));
		}
	}

	~sip_server() {
		if (work) {
			work.reset(nullptr);
		}

		if (!ios.stopped()) {
			ios.stop();
		}

		for (auto & th : pool) {
			if (th.joinable()) {
				th.join();
			}
		}

		if (sock.is_open()) {
			sock.close();
		}
	}

	asio::ip::udp::endpoint & local() {
		return endpoint;
	}

	asio::io_context& get_io_context() { return ios; }

	void stop() {
		// may be work should be protected somehow?
		//work.reset(nullptr);
		ios.stop();
	}


};
