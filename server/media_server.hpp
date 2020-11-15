
#include <boost/asio.hpp>
using namespace boost;

#include "../Network/sip/sip.hpp"
#include "../Network/rtp/rtp.hpp"
#include "session_management/session_handler.hpp"

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


	//std::vector<session> sessions;
	// some session logic-related stuff here
	// e.g session_service
	// which will be responsible
	// for containing alive session
	// and registered users

	// also we will need some
	// rtp logic
	// but for a first time 
	// it will be enough to just find
	// session-partner of rtp_packet sender
	// and redirect it to session-partner

public:

	media_server(std::size_t threads, std::size_t sipport, std::size_t rtpport)
	: 
	sip_ep(asio::ip::udp::v4(), sipport),
	rtp_ep(asio::ip::udp::v4(), rtpport),
	sip_socket(ios, sip_ep),
	rtp_socket(ios, rtp_ep),
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
		std::cout << "received some SIP-related stuff with size: " << message.total() << '\n';
		if (message.type() == sip::sip_type::Request) {
			std::cout << ((sip::request&)message).method() << '\n';
			std::cout << ((sip::request&)message).headers()["From"] << '\n';
			std::cout << ((sip::request&)message).headers()["To"] << '\n';
		}
	}

	void rtp_callback(realtime::rtp_packet & packet) {
		std::cout << "received some RTP-related stuff with size: " << packet.total() << '\n';
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

};