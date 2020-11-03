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


class sip_client {
	using work_ptr = std::unique_ptr<asio::io_context::work>;
	using work_entity = asio::io_context::work;

	asio::io_context ios;
	work_ptr work;

	
	asio::ip::udp::endpoint remote_;
	asio::ip::udp::socket sock;

	sip::client listener;

	std::vector<std::thread> pool;
public:
	sip_client(std::string address, std::size_t threads, std::size_t port = 5060)
		: ios{},
		remote_(asio::ip::address::from_string(address), port),
		sock(ios),
		work(new work_entity(ios)),
		listener(ios, sock)
	{

		sock.open(asio::ip::udp::v4());
		sock.connect(remote_);
		for (std::size_t i = 0; i < threads; i++) {		
			pool.push_back(std::thread([this]() { ios.run(); }));
		}
		listener.start();
	}

	~sip_client() {
		boost::system::error_code ec;
		sock.shutdown(sock.shutdown_both, ec);
		sock.close(ec);
		
		work.reset(nullptr);
		if (!ios.stopped()) {
			ios.stop();
		}

		for (auto & th : pool) {
			if (th.joinable()) {
				th.join();
			}
		}

	}


	asio::ip::udp::endpoint & remote() {
		return remote_;
	}

	asio::io_context& get_io_context() { return ios; }

	bool has_income() {
		return !listener.incoming().empty();
	}

	void send(sip::request & resp) {
		listener.send(resp);
	}

	void update(std::function<void(sip::response&&)> callback) {
		listener.update(callback);
	}

};

void back(sip_client& caller, sip::response&& resp) {
	std::cout << "got response from " << caller.remote().address() << ":" << caller.remote().port() << "\n";

}


int main() {
	setlocale(LC_ALL, "ru");

	sip_client client("127.0.0.1", 2, 6000);

	sip::request req;

	req.set_method(meth)
		.set_uri(uri)
		.set_version(version)
		.add_header(left, right)
		.set_body(body, strlen(body))
		.set_remote(client.remote());


	client.send(req);


	std::atomic<bool> flag{ false };

	std::thread worker([&client, &flag] {
		while (!flag.load(std::memory_order_relaxed)) {
			client.update([&client](sip::response && res) 
			{
				back(client, std::move(res));
			});

			std::this_thread::yield();
		}
	});



	std::cin.get();

	flag.store(true, std::memory_order_relaxed);

	worker.join();

	return 0;
}