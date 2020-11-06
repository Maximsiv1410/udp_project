#include <boost/asio.hpp>
#include "../Network/sip.hpp"
#include <iostream>

using namespace boost;
using namespace net;

const char* meth = "INVITE";
const char* uri = "sip:smbd@lol.com";
const char*version = "SIP/2.0";
const char* left = "Content-Length";
const char* right = "5";
const char*body = "hello";


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

std::atomic<unsigned long long> counter {0};
void back(sip_client& caller, sip::response&& resp) {
	counter.fetch_add(1, std::memory_order_relaxed);
	if (counter.load(std::memory_order_relaxed) >= 300) {
		std::cout << "got 300 responses\n";
		counter = 0;
	}
	//std::cout << "got response from " << caller.remote().address() << ":" << caller.remote().port() << " code: " + std::to_string(resp.code()) << "\n";
	//std::cout << "got response from server: my port is " + std::to_string(resp.code()) << '\n';



	sip::request req;
	req.set_method(meth)
		.set_uri(uri)
		.set_version(version)
		.add_header(left, right)
		.set_body(body, strlen(body))
		.set_remote(caller.remote());

	caller.async_send(req);
}


int main() {
	setlocale(LC_ALL, "ru");

		sip_client client("127.0.0.1", 1, 6000);

		sip::request req;
		req.set_method(meth)
			.set_uri(uri)
			.set_version(version)
			.add_header(left, right)
			.set_body(body, strlen(body))
			.set_remote(client.remote());


		client.set_callback([&client](sip::response && res) {
			back(client, std::move(res));
		});
		client.start(true);
		client.async_send(req);
		


		std::cin.get();

	return 0;

}