#pragma once

#include <boost/asio.hpp>
#include "net_essential.hpp"

#include "tsqueue.hpp"

namespace net {
	using namespace boost;
	using namespace asio;

	template<typename Proto, typename Traits, typename Buffer>
	class basic_udp_io {
		using input_type = typename Traits::input_type;
		using output_type = typename Traits::output_type;
		using input_parser = typename Traits::template input_parser<Buffer>;
		using output_builder = typename Traits::output_builder;
		using buffer = Buffer;

		buffer in_buff;
		buffer out_buff;

		tsqueue<input_type> qin;
		tsqueue<output_type> qout;

		io_context& ios_;
		ip::udp::socket& sock_;
		ip::udp::endpoint remote_;

		std::atomic<bool> writing{ false };

		std::function<void(input_type&&)> cback;

		bool notify_mode;

		std::mutex mtx;

	public:

		basic_udp_io(io_context& ios, ip::udp::socket& sock)
			: ios_(ios),
			sock_(sock) {
		}

		virtual ~basic_udp_io() {
		}

		void start(bool notify = false) {
			notify_mode = notify;
			read();
		}

		void send(output_type & res) {
			qout.push(std::move(res));
			asio::post(ios_,
			[this] {
				if (!writing.load(std::memory_order_relaxed))
					write();
			});
		}

		void set_callback(std::function<void(input_type&&)> & callback) {
			cback = std::move(callback);
		}

		void update(std::function<void(input_type&&)> callback) {
			asio::post(ios_, [this, callback] {
				if (!qin.empty()) {
					auto item = qin.try_pop();
					if (item) {
						callback(std::move(*item));
					}

				}
			});
		}


		tsqueue<input_type> & incoming() {
			return qin;
		}

	private:

		void read() {
			sock_.async_receive_from(asio::buffer(in_buff.data(), in_buff.max_size()), remote_,
			[this](std::error_code ec, std::size_t bytes) {
				if (!ec) {
					if (bytes) {
						//std::cout << "read message \n";
						std::cout << ",\n";
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
				asio::post(ios_, [this] 
				{ 
					auto ptr = qin.try_pop();
					if (ptr) {
						cback(std::move(*ptr));
					}
				});
			}
			

			in_buff.zero();
			read();
		}


		void write() {
			if (writing.load(std::memory_order_relaxed)) return;

			writing.store(true, std::memory_order_relaxed);
			auto ptr = qout.try_pop();
			if (!ptr) return; 
			auto res = std::move(*ptr);
			output_builder builder(res);
			builder.extract_to(out_buff);

			sock_.async_send_to(asio::buffer(out_buff.data(), out_buff.size()), res.remote(),
			[this](std::error_code ec, std::size_t bytes) {
				if (!ec) {
					if (bytes) {
						//std::cout << "written message\n";
						std::cout << ".\n";
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
			writing.store(false, std::memory_order_relaxed);


			if (!qout.empty()) {
				write();
			}
		}

	};

}



