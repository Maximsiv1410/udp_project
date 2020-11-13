
#include <boost/asio.hpp>
using namespace boost;

#include "../Network/sip/sip.hpp"
#include "../Network/rtp/rtp.hpp"
#include "session.hpp"

using namespace net;


class media_server {
	using work_ptr = std::unique_ptr<asio::io_context::work>;
	using work_entity = asio::io_context::work;

	asio::io_context ios;
	work_ptr work;

	asio::ip::udp::endpoint sip_ep;
	asio::ip::udp::endpoint rtp_ep;

	asio::ip::udp::socket sip_socket;
	asio::ip::udp::socket rtp_socket;

	sip::server sipper;
	realtime::server rtp_service;

	std::vector<std::thread> pool;
	std::vector<session> sessions;

public:

	media_server(std::size_t threads, std::size_t sipport, std::size_t rtpport)
	: 
	sip_ep(asio::ip::udp::v4(), sipport),
	rtp_ep(asio::ip::udp::v4(), rtpport),
	sipper(ios, sip_socket),
	rtp_service(ios, rtp_socket),
	work(std::make_unique<work_entity>(ios))
	{
		for (auto i = 0; i < threads; i++) {
			pool.push_back(std::thread([this]{ ios.run(); }));
		}
	}

	void start() {
		sipper.set_callback([this](sip::message_wrapper && wrapper)
		{
			this->sip_callback(*wrapper.storage());
		});

		rtp_service.set_callback([this](realtime::rtp_packet && packet)
		{
			this->rtp_callback(packet);
		});

		sipper.start(true);
		rtp_service.start(true);
	}

	void sip_callback(sip::message & message) {
		// process
	}

	void rtp_callback(realtime::rtp_packet & packet) {
		// process
	}


	void stop() {
		if (!ios.stopped()) {
			ios.stop();
		}

		if (work) {
			work.reset(nullptr);
		}
	}

	~media_server() {
		if (!ios.stopped()) {
			ios.stop();
		}

		if (work) {
			work.reset(nullptr);
		}

		for (auto& th : pool) {
			if (th.joinable()) {
				th.join();
			}
		}

		if (sip_socket.is_open()) {
			sip_socket.close();
		}

		if (rtp_socket.is_open()) {
			rtp_socket.close();
		}
	}

}