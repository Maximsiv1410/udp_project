
#include <boost/asio.hpp>
using namespace boost;

#include <thread>
#include <memory>
#include <string>
#include <iostream>

#include <functional>
#include "../Network/rtp/rtp.hpp"

using namespace net;

class rtp_server : public realtime::server {
	using work_ptr = std::unique_ptr<asio::io_context::work>;
	using work_entity = asio::io_context::work;

	asio::io_context ios;
	work_ptr work;
	asio::ip::udp::endpoint endpoint;
	asio::ip::udp::socket sock;

	std::vector<std::thread> pool;
public:
	rtp_server(std::size_t threads, std::size_t port = 5060)
		: ios{},
		realtime::server(ios, sock),
		endpoint(asio::ip::udp::v4(), port),
		sock(ios, endpoint),
		work(new work_entity(ios))
		
	{
		for (std::size_t i = 0; i < threads; i++) {
			pool.push_back(std::thread([this]() { ios.run(); }));
		}
	}

	~rtp_server() {
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

std::uint32_t ids[15];
char payload[1350];


std::atomic<unsigned long long> bytes_in{0};
void back(rtp_server& caller, realtime::rtp_packet && msg) {
	realtime::rtp_packet mypack;
	//std::cout << "server received rtp packet from: " << msg.remote().address() << ":" << msg.remote().port() << '\n';  

	bytes_in.fetch_add(msg.total(), std::memory_order_relaxed);
	auto & header = mypack.header();


	// now just sending pseudo-data, no sense
	header.version(1);
	header.padding(true);
	header.extension(true);
	header.csrc_count(15);
	header.marker(true);
	header.payload_type(5);
	header.sequence_number(6);
	header.timestamp(msg.header().timestamp() * msg.header().timestamp());
	header.ssrc(677);

	mypack.set_csrc_list(ids, 15);
	mypack.set_payload(payload, 1350);
	mypack.set_remote(std::move(msg.remote()));

	caller.async_send(mypack);

}


int main() {
	std::memset(payload, 1, 1350);
	std::memset(ids, 1, 15 * sizeof(std::uint32_t));

	setlocale(LC_ALL, "ru");

	rtp_server server(2, 6000);
	server.set_callback(
	[&server](realtime::rtp_packet && package)
	{
		back(server, std::move(package));
	});
	server.start(true);

	std::cin.get();

	server.stop();

	std::cout << "read bytes: " << bytes_in << '\n';
	std::cout << "written bytes: " << server.written_out() << '\n';
	return 0;
}




