#include <boost/asio.hpp>
#include "../Network/rtp/rtp.hpp"
#include <iostream>

using namespace boost;
using namespace net;

class rtp_client : public realtime::client {
	using work_ptr = std::unique_ptr<asio::io_context::work>;
	using work_entity = asio::io_context::work;

	asio::io_context ios;
	work_ptr work;

	asio::ip::udp::endpoint remote_;
	asio::ip::udp::socket sock;

	std::vector<std::thread> pool;

public:
	rtp_client(std::string address, std::size_t threads, std::size_t port = 5060)
		: 
		realtime::client(ios, sock),
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

	~rtp_client() {
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

std::uint32_t ids[15];
char payload[1350];

std::atomic<unsigned int> my_num{0};

void back(rtp_client & caller, realtime::rtp_packet && pack) {
	realtime::rtp_packet mypack;
	//std::cout << "my hint is: " << pack.header().timestamp() << '\n';
	auto & header = mypack.header();

	header.version(1);
	header.padding(true);
	header.extension(true);
	header.csrc_count(15);
	header.marker(true);
	header.payload_type(5);
	header.sequence_number(6);
	header.timestamp(my_num.load(std::memory_order_relaxed));
	header.ssrc(677);

	mypack.set_csrc_list(ids, 15);
	mypack.set_payload(payload, 1350);

	auto copy = caller.remote();
	mypack.set_remote(std::move(copy));

	caller.async_send(mypack);
}


int main(int argc, char ** argv) {
	setlocale(LC_ALL, "ru");
	my_num.store(std::stol(argv[1]));

	std::string address{argv[2]};
	

	std::memset(payload, 1, 1350);
	std::memset(ids, 1, 15 * sizeof(std::uint32_t));


	rtp_client client(address, 1, 6000);
	client.set_callback([&client](realtime::rtp_packet && package) {
		back(client, std::move(package));
	});
	client.start(true);

	realtime::rtp_packet pack;
	auto & header = pack.header();

	header.version(1);
	header.padding(true);
	header.extension(true);
	header.csrc_count(15);
	header.marker(true);
	header.payload_type(5);
	header.sequence_number(6);
	header.timestamp(my_num.load());
	header.ssrc(677);

	pack.set_csrc_list(ids, 15);
	pack.set_payload(payload, 1350);

	auto copy = client.remote();
	std::cout << copy.address() << ':' << copy.port() << '\n';
	pack.set_remote(std::move(copy));

	client.async_send(pack);

	std::cin.get();

	return 0;

}