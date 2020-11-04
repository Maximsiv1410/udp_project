#include <boost/asio.hpp>
using namespace boost;

#include <thread>
#include <memory>
#include <string>
#include <iostream>

#include <functional>
#include "../Network/sip.hpp"

using namespace net;

class sip_server {
	using work_ptr = std::unique_ptr<asio::io_context::work>;
	using work_entity = asio::io_context::work;

	asio::io_context ios;
	work_ptr work;
	asio::ip::udp::endpoint endpoint;
	asio::ip::udp::socket sock;

	std::thread ios_thread;
	sip::server listener;

	std::vector<std::thread> pool;

public:
	sip_server(std::size_t threads, std::size_t port = 5060)
		: ios{},
		endpoint(asio::ip::udp::v4(), port),
		sock(ios, endpoint),
		work(new work_entity(ios)),
		listener(ios, sock)
	{
		for (std::size_t i = 0; i < threads; i++) {
			pool.push_back(std::thread([this]() { ios.run(); }));
		}
	}

	~sip_server() {
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

	asio::ip::udp::endpoint & local() {
		return endpoint;
	}

	asio::io_context& get_io_context() { return ios; }

	void start(bool notifying) {
		listener.start(notifying);
	}



	bool has_income() {
		return !listener.incoming().empty();
	}

	void send(sip::response & resp) {
		listener.send(resp);
	}

	void update(std::function<void(sip::request&&)> callback) {
		listener.update(callback);
	}

	void set_callback(std::function<void(sip::request&&)> callback) {
		listener.set_callback(callback);
	}


};


void back(sip_server& caller, sip::request && req) {
	//std::cout << "got request from " << req.remote().address() << ":" << req.remote().port() << "\n";
	//std::cout << ",,\n";
	sip::response resp;
	resp.set_version("SIP/2.0")
		.set_code(200)
		.set_status("OK")
		.set_body("response from server")
		.set_remote(req.remote());

	caller.send(resp);
}


int main() {
	setlocale(LC_ALL, "ru");

	sip_server server(4, 6000);
	server.set_callback([&server](sip::request && req) {
		back(server, std::move(req));
	});

	server.start(true);

	//std::atomic<bool> flag{ false };

	//std::this_thread::sleep_for(std::chrono::seconds(5));

	//std::thread worker([&server, &flag] {
	//	while (!flag.load(std::memory_order_relaxed)) {
	//		server.update([&server](sip::request && req) {
	//			back(server, std::move(req));
	//		});

	//		//std::this_thread::yield();
	//		//std::this_thread::sleep_for(std::chrono::seconds(2));
	//	}
	//});


	std::cin.get();
	//flag.store(true, std::memory_order_relaxed);

	//worker.join();
	return 0;
}




