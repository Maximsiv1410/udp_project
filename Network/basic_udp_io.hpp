#pragma once

#include <boost/asio.hpp>
#include "net_essential.hpp"
#include "tsqueue.hpp"

namespace net {
	using namespace boost;
	using namespace asio;

	// Buffer should be a stack allocated buffer at least 63535 byte-length
	template <typename Proto, typename Traits, typename Buffer>
	class basic_udp_service {
		//// gathering traits of service ////
		using input_type = typename Traits::input_type;
		using output_type = typename Traits::output_type;
		using input_parser = typename Traits::template input_parser<Buffer>;
		using output_builder = typename Traits::output_builder;
		using buffer = Buffer;

		buffer in_buff;

		// provide some align to deny false sharing and improve cache coherency
		std::function<void(input_type&&)> cback;
		std::mutex callback_mtx;
	
		tsqueue<input_type> qin;
		tsqueue<output_type> qout;

		io_context& ios_;
		ip::udp::socket& sock_;
		ip::udp::endpoint remote_;

		std::unique_ptr<io_context::strand> strandie;

		bool notify_mode;
	
		std::atomic<unsigned long long> bytes_out{0};
	public:

		unsigned long long written_out() {
			return bytes_out.load(std::memory_order_relaxed);
		}


		basic_udp_service(io_context& ios, ip::udp::socket& sock)
			: 
			ios_(ios),
			sock_(sock) 
			{}

		virtual ~basic_udp_service() {}

		// rename to 'init_read' ?
		void start(bool notify = false) {
			notify_mode = notify;
			if (notify) { 
				strandie = std::make_unique<io_context::strand>(ios_); 
			} // bad?
			read();
		}

		void set_callback(std::function<void(input_type&&)> callback) {
			std::lock_guard<std::mutex> guard(callback_mtx);
			cback = std::move(callback);
		}

		void poll(std::function<void(input_type&&)> task) {
			if (notify_mode) return;

			input_type message;
			while (!ios_.stopped()) {
				qin.wait_and_pop(message);
			
				task(std::move(message));
			}
		} 

		void async_send(output_type & res) {
			qout.push(std::move(res));
			asio::post(ios_, [this]{write();});
		}

	private:

		void write() {
			output_type message;			

			bool attempt = qout.try_pop(message);
			if (!attempt) {		
				return;
			}		

			// std::vector<char> should come here as output_buffer
			std::shared_ptr<std::vector<char>> out(std::make_shared<std::vector<char>>());
			out->resize(message.total());
			output_builder builder(message);
			builder.extract(*out);

			sock_.async_send_to(asio::buffer(out->data(), out->size()), message.remote(),
			[this, out](std::error_code ec, std::size_t bytes)
			{
				if (!ec) {
					if (bytes) {
						//std::cout << "written message\n";
						bytes_out.fetch_add(bytes, std::memory_order_relaxed);
						on_write();
					}
					else {
						std::cout << "bad write - bytes\n";
						write();
					}
				}
				else {
					std::cout << "write error\n";
					std::cout << ec.message() + "\n";
				}
			});
		}


		void on_write() {			
			if (!qout.empty()) {
				write();
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
					std::cout << "write error\n";
					std::cout << ec.message() << " " << ec.category().name() << " " << ec.value() << "\n";
				}
			});
		}


		// to think:
		// notify from service or poll?

		void on_read() {
			input_parser parser(in_buff, std::move(remote_));
			qin.push(parser.parse());

			//should callback be called sequentially or simultaneously?????
			// strand reduces speed of processing callbacks
			// but if writer-threads are busy too long
			// this tactics are beneficial
			// because this service allows multiple 'send' operations at a time

			if (notify_mode && strandie) {
				// callback will be called only after previous call end
				strandie->post(
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

				/*
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
				*/
			}

			in_buff.zero();
			read();
		}

	protected:
		// here is possible
		// analogue for std function<input_Type> callback's

		// idea is to call overwritten 'on_income'
		// on every 'on_read' event
		// where must be some logic 

		//virtual void on_income() {}

	};

}



