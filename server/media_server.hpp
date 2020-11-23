
#include <boost/asio.hpp>
using namespace boost;

#include "../Network/sip/sip.hpp"
#include "../Network/rtp/rtp.hpp"
#include "sipsess/session_handler.hpp"
#include "rtp_engine.hpp"

using namespace net;


class media_server {
	using work_ptr = std::unique_ptr<asio::io_context::work>;
	using work_entity = asio::io_context::work;

	asio::io_context ios;
	work_ptr work;

	session_handler sipper;
	rtp_engine rtpserv;

	std::vector<std::thread> task_force;

public:

	media_server(std::size_t threads, std::uint16_t sip_port, std::uint16_t rtp_port)
	: work(new work_entity(ios))
	, sipper(ios, sip_port)
	, rtpserv(ios, rtp_port)
	{
		for (std::size_t i = 0; i < threads; i++) {
			task_force.emplace_back([this]{ ios.run(); });
		}

		sipper.set_callback([this](sip::message_wrapper && wrappie)
		{
			sipper.process(*wrappie.storage());
		});

		rtpserv.set_callback([this](realtime::rtp_packet && pack)
		{
			if (sipper.has_session_rtp(pack.remote())) {
				auto peer = sipper.get_partner_rtp(pack.remote());
				if (peer.has_value()) {
					//std::cout << "redirecting packet from " << pack.remote() << " to " << *peer << '\n';
					pack.set_remote(*peer);
					rtpserv.async_send(pack);
				}
			}
			else {
				std::cout << "No session with " << pack.remote() << '\n';
			}
		});
	}

	void start() {
		sipper.start(true);
		rtpserv.start(true);
	}

	void stop() {
		if (work) {
			work.reset(nullptr);
		}

		if (!ios.stopped()) {
			ios.stop();
		}
	}

	~media_server() {
		this->stop();

		for (auto & thread : task_force) {
			if (thread.joinable()) {
				thread.join();
			}
		}

		// probably here is the best place
		// because closing before
		// joining all threads causes data race
		sipper.stop();
		rtpserv.stop();
	}

};