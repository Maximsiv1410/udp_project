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

	void start(bool notifying) {
		listener.start(notifying);
	}


	void async_send(sip::response & resp) {
		listener.async_send(resp);
	}

	/////////////////////////////////
	void poll(std::function<void(sip::request&&)> callback) {
		listener.poll(callback);
	}

	void set_callback(std::function<void(sip::request&&)> callback) {
		listener.set_callback(callback);
	}

	void stop() {
		// may be work should be protected somehow?
		//work.reset(nullptr);
		ios.stop();
	}


};


std::atomic<unsigned long long> counter{0};
void back(sip_server& caller, sip::request && req) {
	//std::cout << "got request from " << req.remote().address() << ":" << req.remote().port() << "\n";
	//std::cout << ",,\n";
	sip::response resp;
	resp.set_version("SIP/2.0")
		.set_code(200 + counter.load(std::memory_order_relaxed))
		//.set_code(req.remote().port())
		.set_status("OK")
		.set_body("response from server")
		.set_remote(req.remote());

		counter.fetch_add(1, std::memory_order_relaxed);
	caller.async_send(resp);
}


int main() {
	setlocale(LC_ALL, "ru");

	sip_server server(1, 6000);



	server.set_callback(
	[&server](sip::request && req)
	{
		back(server, std::move(req));
	});

	server.start(true);



/*
	server.start(false);

	std::thread worker([&server]()
	{
		server.poll([&server](sip::request && req)
		{
			back(server, std::move(req));
		});
	});
*/


	std::cin.get();

	server.stop();
	//worker.join();
	return 0;
}




