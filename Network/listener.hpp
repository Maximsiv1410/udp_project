#pragma once

#include "net_essential.hpp"
#include <boost/asio.hpp>

#include "input_buffer.hpp"
#include "output_buffer.hpp"
#include "sip_message.hpp"
#include "tsqueue.hpp"

using namespace boost;


namespace net {
	using namespace buffers;
	using namespace asio;

	namespace sip {


		class listener {
			input_buffer in_buff;
			output_buffer out_buff;

			tsqueue<request> qin;
			tsqueue<response> qout;

			io_context& ios_;
			ip::udp::socket& sock_;
			ip::udp::endpoint remote_;
			io_context::strand strand_;

		public:
			listener(io_context& ios, ip::udp::socket& sock)
				: ios_(ios),
				sock_(sock),
				strand_(ios)
			{
			}

			virtual ~listener() {}

			void start() {
				read();
			}

			void add_response(response & res) {
				bool idle = qout.empty();
				qout.push(std::move(res));

				asio::post(
					[this, idle]() {
					if (idle) {
						write();
					}
				});
			}

			void update(std::function<void(request)>  callback) {
				asio::post(ios_, [this, callback] 
				{
					while (qin.empty());
					callback(qin.pop());
				});
			}

			void update() {
				while (qin.empty());
				on_request(qin.pop());
			}

			tsqueue<request> & incoming() { return qin; }

		private:

			void read() {
				sock_.async_receive_from(asio::buffer(in_buff.data(), in_buff.max_size()), remote_,
				 [this](std::error_code ec, std::size_t bytes) {
					if (!ec) {
						if (bytes) {
							std::cout << "read message [" << bytes << "] bytes\n";
							in_buff.set_size(bytes);
							on_read();
						}
						else {
							std::cout << "bad read - bytes\n";
						}
					}
					else {
						std::cout << "bad read - error code\n";
						std::cout << ec.message() << "\n";
					}
				});
			}

			void on_read() {
				request_parser<input_buffer> parser(in_buff, std::move(remote_));
				qin.push(parser.parse());

				in_buff.zero();
				
				read();
			}


			void write() {
				auto res = qout.pop();
				response_builder builder(res);
				builder.extract_to(out_buff);

				sock_.async_send_to(asio::buffer(out_buff.data(), out_buff.size()), res.remote(),
				[this](std::error_code ec, std::size_t bytes) {
					if (!ec) {
						if (bytes) {
							std::cout << "written message [" << bytes << "] bytes\n";
							on_write();
						}
						else {
							std::cout << "bad write - bytes\n";
						}
					}
					else {
						std::cout << "bad write - error code\n";
						std::cout << ec.message() << "\n";
					}
				});
			}

			void on_write() {
				out_buff.zero();

				if (!qout.empty()) {
					write();
				}
			}



		protected:

			virtual void on_request(request&& req) {
				
			}
			

		};






	}
}