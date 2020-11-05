#pragma once

#include <boost/asio.hpp>
#include "net_essential.hpp"
#include "tsqueue.hpp"

namespace net {
	using namespace boost;
	using namespace asio;

	template <typename Proto, typename Traits, typename Buffer>
	class basic_udp_service {
		//// gathering traits of service ////
		using input_type = typename Traits::input_type;
		using output_type = typename Traits::output_type;
		using input_parser = typename Traits::template input_parser<Buffer>;
		using output_builder = typename Traits::output_builder;
		using buffer = Buffer;

		buffer in_buff;

		std::function<void(input_type&&)> cback;
		std::mutex callback_mtx;
	
		buffer out_buff;

		tsqueue<input_type> qin;
		tsqueue<output_type> qout;

		io_context& ios_;
		ip::udp::socket& sock_;
		ip::udp::endpoint remote_;

		bool notify_mode;
		bool writing{false};
		std::/*recursive_*/mutex write_mtx;
	public:

		basic_udp_service(io_context& ios, ip::udp::socket& sock)
			: ios_(ios),
			sock_(sock) {
		}

		virtual ~basic_udp_service() {}

		void start(bool notify = false) {
			notify_mode = notify;
			read();
		}


		void set_callback(std::function<void(input_type&&)> & callback) {
			std::lock_guard<std::mutex> guard(callback_mtx);
			cback = std::move(callback);
		}


		// !deprecate!
		void update(/*bool wait = false, */std::function<void(input_type&&)> callback) {
			if (notify_mode) return; // we should only use either update or default callback
			//if (wait) qin.wait();

			// !note!
			// callback can fail or drop exception
			// also it can block execution
			// that's why probably
			// we can use just calling thread to
			// process incoming messages
			// but also user may want to
			// process messages simultaneously
			// so we may need asio::post here
			//asio::post(ios_, [this, callback] {
			if (!qin.empty()) {
				auto item = qin.try_pop();
				if (item) {
					callback(std::move(*item));
				}
			}
			//});
		}


		// !deprecate!
		void async_update(std::function<void(input_type&&)> callback) {
			if (notify_mode) return; // ??

			asio::post(ios_,
			[this, callback] 
			{
				if (!qin.empty()) {
					auto item = qin.try_pop();
					if (item) {
						callback(std::move(*item));
					}
				}
			});
		}

		// !deprecate!
		tsqueue<input_type> & incoming() {
			return qin;
		}


		void async_send(output_type & res) {
			qout.push(std::move(res));
			asio::post(ios_,
			[this] 
			{
				bool busy = false;
				{
					std::lock_guard<std::mutex> guard(write_mtx);
					busy = writing;
					if (!writing) {
						writing = true;
					}
				}
				if (!busy) {
					write();
				}

			});
		}

	private:

		void write() {

			auto ptr = qout.try_pop();
			if (!ptr) {		
				std::lock_guard<std::mutex> guard(write_mtx);
				writing = false;
				return;
			}		

			auto res = std::move(*ptr);	
			output_builder builder(res);
			builder.extract_to(out_buff);

			sock_.async_send_to(asio::buffer(out_buff.data(), out_buff.size()), res.remote(),
			[this](std::error_code ec, std::size_t bytes)
			{
				if (!ec) {
					if (bytes) {
						//std::cout << "written message\n";
						//std::cout << ".\n";
						on_write();
					}
					else {
						std::cout << "bad write - bytes\n";
					}
				}
				else {
					std::cout << ec.message() + "\n";
				}
			});
		}

		void on_write() {
			out_buff.zero();

			if (!qout.empty()) {
				// no need in mutex
				// because here we are
				//guaranteed to be with writing == true
				write();
			}
			else {
				std::lock_guard<std::/*recursive_*/mutex> guard(write_mtx);
				writing = false;
			}
		}




		void read() {
			sock_.async_receive_from(asio::buffer(in_buff.data(), in_buff.max_size()), remote_,
			[this](std::error_code ec, std::size_t bytes) 
			{
				if (!ec) {
					if (bytes) {
						//std::cout << "read message \n";
						//std::cout << ",\n";
						in_buff.set_size(bytes);
						on_read();
					}
					else {
						read();
					}
				}
				else {
					std::cout << ec.message() << " " << ec.category().name() << " " << ec.value() << "\n";
				}
			});
		}

		void on_read() {
			input_parser parser(in_buff, std::move(remote_));
			qin.push(parser.parse());

			// !note!
			// maybe it should be default 
			// and 'update' should be removed
			if (notify_mode) {
				asio::post(ios_,
				[this] 
				{
					auto ptr = qin.try_pop();
					if (ptr) {
						std::function<void(input_type&&)> task;
						{
							std::lock_guard<std::mutex> guard(callback_mtx);
							task = cback;
						}
						if (task) {
							task(std::move(*ptr));
						}
					}
				});
			}

			in_buff.zero();
			read();
		}


	protected:
		// !note!
		virtual void on_income() {}

	};

}



