
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
			if (sipper.has_session(pack.remote())) {
				pack.set_remote(sipper.get_partner(pack.remote()));
				rtpserv.async_send(pack);
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

		sipper.stop();
		rtpserv.stop();
	}

	~media_server() {
		this->stop();

		for (auto & thread : task_force) {
			if (thread.joinable()) {
				thread.join();
			}
		}
	}

};