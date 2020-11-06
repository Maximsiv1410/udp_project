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
			: 
			ios_(ios),
			sock_(sock) {
		}

		virtual ~basic_udp_service() {}


		// rename to 'init_read' ?
		void start(bool notify = false) {
			notify_mode = notify;
			read();
		}


		void set_callback(std::function<void(input_type&&)> & callback) {
			std::lock_guard<std::mutex> guard(callback_mtx);
			cback = std::move(callback);
		}


		void poll(std::function<void(input_type&&)> task) {
			if (notify_mode) return;

			input_type message;
			while (!ios_.stopped()) {
				qin.wait_and_pop(message);
				
				std::cout << "making task\n";
				task(std::move(message));
				std::cout << "done task\n";
				
			}

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
			output_type message;

			bool attempt = qout.try_pop(message);

			if (!attempt) {		
				exit_write();
				return;
			}		
	
			output_builder builder(message);
			builder.extract_to(out_buff);

			sock_.async_send_to(asio::buffer(out_buff.data(), out_buff.size()), message.remote(),
			[this](std::error_code ec, std::size_t bytes)
			{
				if (!ec) {
					if (bytes) {
						//std::cout << "written message\n";
						on_write();
					}
					else {
						std::cout << "bad write - bytes\n";
						write();
					}
				}
				else {
					std::cout << ec.message() + "\n";
				}
			});
		}

		// notify about end of writing
		void exit_write() {
			std::lock_guard<std::mutex> guard(write_mtx);
			writing = false;
		}

		void on_write() {
			out_buff.zero();

			// here 100% writing == true
			if (!qout.empty()) {
				write();
			}
			else {
				exit_write();
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


		// to think:
		// notify from service or poll?

		void on_read() {
			input_parser parser(in_buff, std::move(remote_));
			qin.push(parser.parse());

			if (notify_mode) {
				asio::post(ios_,
				[this] 
				{
					input_type message;
					bool attempt = qin.try_pop(message);
					if (attempt) {
						std::function<void(input_type&&)> task;
						{
							std::lock_guard<std::mutex> guard(callback_mtx);
							task = cback;
						}
						if (task) {
							task(std::move(message));
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



