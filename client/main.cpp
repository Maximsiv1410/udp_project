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

	bool has_income() {
		return !listener.incoming().empty();
	}

	void send(sip::request & resp) {
		listener.send(resp);
	}

	void update(std::function<void(sip::response&&)> callback) {
		listener.update(callback);
	}

	void stop() {
		ios.stop();
	}

	bool stopped() {
		return ios.stopped();
	}

	void start(bool notifying) {
		listener.start(notifying);
	}

	void set_callback(std::function<void(sip::response&&)> callback) {
		listener.set_callback(callback);
	}

};

void back(sip_client& caller, sip::response&& resp) {
	//std::cout << "got response from " << caller.remote().address() << ":" << caller.remote().port() << "\n";
	std::cout << "..\n";
	sip::request req;

	req.set_method(meth)
		.set_uri(uri)
		.set_version(version)
		.add_header(left, right)
		.set_body(body, strlen(body))
		.set_remote(caller.remote());

	caller.send(req);

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


	

	client.set_callback([&client](sip::response && res)
	{
		back(client, std::move(res));
	});
	client.start(true);

	client.send(req);

	/*std::thread worker([&client] {
		while (!client.stopped()) {
			client.update([&client](sip::response && res) {
				back(client, std::move(res));
			});

			//std::this_thread::yield();
			//std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	}); */



	std::cin.get();
	//client.stop();

	//worker.join();

	return 0;
}