#pragma once
#include <boost/asio.hpp>
#include "../Network/sip/sip.hpp"
using namespace boost;
using namespace net;


class sip_client : public sip::client {
	using work_ptr = std::unique_ptr<asio::io_context::work>;
	using work_entity = asio::io_context::work;

	asio::io_context ios;
	work_ptr work;


	asio::ip::udp::endpoint remote_;
	asio::ip::udp::socket sock;

	std::vector<std::thread> pool;

public:
	sip_client(std::string address, std::size_t threads, std::size_t port = 5060)
		: 
		sip::client(ios, sock),
		ios{},
		remote_(asio::ip::address::from_string(address), port),
		sock(ios),
		work(new work_entity(ios))	
	{
		sock.open(asio::ip::udp::v4());
		sock.connect(remote_);

		for (std::size_t i = 0; i < threads; i++) {
			pool.push_back(std::thread([this]() { ios.run(); }));
		}
	}

	~sip_client() {
		work.reset(nullptr);

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


	asio::ip::udp::endpoint & remote() {
		return remote_;
	}

	asio::io_context& get_io_context() { return ios; }

	void stop() {
		work.reset(nullptr);
		ios.stop();
	}

	bool stopped() {
		return ios.stopped();
	}


};