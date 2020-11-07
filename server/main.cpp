#include <boost/asio.hpp>
using namespace boost;

#include <thread>
#include <memory>
#include <string>
#include <iostream>

#include <functional>
#include "../Network/sip/sip.hpp"

using namespace net;

class sip_server : public sip::server {
	using work_ptr = std::unique_ptr<asio::io_context::work>;
	using work_entity = asio::io_context::work;

	asio::io_context ios;
	work_ptr work;
	asio::ip::udp::endpoint endpoint;
	asio::ip::udp::socket sock;

	std::vector<std::thread> pool;

public:
	sip_server(std::size_t threads, std::size_t port = 5060)
		: sip::server(ios, sock),
		ios{},
		endpoint(asio::ip::udp::v4(), port),
		sock(ios, endpoint),
		work(new work_entity(ios))
		
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

	void stop() {
		// may be work should be protected somehow?
		//work.reset(nullptr);
		ios.stop();
	}


};


std::atomic<unsigned long long> counter{0};
void back(sip_server& caller, sip::request && req) {

	sip::response resp;
	resp.set_version("SIP/2.0")
		.set_code(counter.load(std::memory_order_relaxed))
		//.set_code(req.remote().port())
		.set_status("OK")
		.set_body("response from server")
		.set_remote(req.remote());

	counter.fetch_add(1, std::memory_order_relaxed);

	//if (counter.load(std::memory_order_relaxed) >= 300) {
		//counter.store(0, std::memory_order_relaxed);
		//std::cout << "processed 300 requests\n";
	//}

	//std::this_thread::sleep_for(std::chrono::milliseconds(100));
	caller.async_send(resp);

}


int main() {
	setlocale(LC_ALL, "ru");

	sip_server server(4, 6000);



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

	std::cout << server.total_bytes() << '\n';
	server.stop();
	//worker.join();
	return 0;
}




