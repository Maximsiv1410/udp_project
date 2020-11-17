
#include <boost/asio.hpp>
using namespace boost;

#include "../Network/rtp/rtp.hpp"

using namespace net;


class rtp_engine : public realtime::server {
	asio::io_context & ios;
	asio::ip::udp::endpoint local;
	asio::ip::udp::socket sock;

public:
	rtp_engine(asio::io_context & ioc, std::uint16_t port) 
	: realtime::server(ioc, sock)
	, ios(ioc)
	, local(asio::ip::udp::v4(), port)
	, sock(ioc, local)
	{

	}

	void stop() {
		if (sock.is_open()) {
			sock.close();
		}
	}
};