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

		std::atomic<bool> flag{false};
		
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


		/////////////////////////////////
		void stop() {
			flag.store(true, std::memory_order_relaxed);
		}

		void poll(std::function<void(input_type&&)> task) {
			if (notify_mode) return;

			while (!flag.load(std::memory_order_relaxed)) {
				if (qin.empty()) qin.wait();
				auto ptr = qin.try_pop();
				if (ptr) {					
					std::cout << "making task\n";
					task(std::move(*ptr));
					std::cout << "done task\n";
				}
			}

		} 
		////////////////////////////////


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
		//virtual void on_income() {}

	};

}



