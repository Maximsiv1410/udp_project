#pragma once

#include <boost/asio.hpp>
#include "net_essential.hpp"
#include "tsqueue.hpp"

namespace net {
	using namespace boost;
	using namespace asio;

	// Buffer should be a stack allocated buffer at least 1400 byte-length
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

		std::unique_ptr<io_context::strand> strandie {nullptr};

		bool notify_mode {false};
	
		std::atomic<unsigned long long> bytes_out{0};

	public:
		basic_udp_service(io_context& ios, ip::udp::socket& sock)
			: 
			ios_(ios),
			sock_(sock) 
		{
		}

		virtual ~basic_udp_service() {}

		unsigned long long written_out() {
			return bytes_out.load(std::memory_order_relaxed);
		}


		void start(bool notify = false) {
			notify_mode = notify;
			if (notify) { 
				strandie = std::make_unique<io_context::strand>(ios_); 
			} // bad?
			read();
		}

		void shutdown() {
			sock_.close();
		}

		void set_callback(std::function<void(input_type&&)> callback) {
			std::lock_guard<std::mutex> guard(callback_mtx);
			cback = std::move(callback);
		}

		void poll(std::function<void(input_type&&)> task) {
			if (notify_mode) return;

			input_type input;
			while (!ios_.stopped()) {
				qin.wait_and_pop(input);		
				task(std::move(input));
			}
		} 


		// send message bypassing queue
		void async_send(output_type & output /* std::function<void()> completion_handler */) {
			auto out = std::make_shared<std::vector<char>>();
			out->resize(output.total());

			output_builder builder(output);
			builder.extract(out->data());

			
			std::cout << "writing to " << output.remote() << '\n';
			sock_.async_send_to(asio::buffer(out->data(), out->size()), output.remote(),
			[this, out](std::error_code ec, std::size_t bytes)
			{
				if (!ec) {
					if (bytes) {
						bytes_out.fetch_add(bytes, std::memory_order_relaxed);
						std::cout << "written " << bytes << " bytes.\n";
					}
					else {
						std::cout << "bad write - no bytes\n";
					}
				}
				else {
					std::cout << ec.message() + "\n";
				}
			});
		}


		void enqueue(output_type & output) {
			qout.push(std::move(output));
		}

		void init_send() {
			asio::post(ios_, [this]{ try_write();});
		}



	private:
		void try_write() {	
			if (qout.empty()) return;

			output_type message;
			bool attempt = qout.try_pop(message);
			if (!attempt) {		
				return;
			}		

			auto out = std::make_shared<std::vector<char>>();
			out->resize(message.total());

			output_builder builder(message);
			builder.extract(out->data());

			sock_.async_send_to(asio::buffer(out->data(), out->size()), message.remote(),
			[this, out](std::error_code ec, std::size_t bytes)
			{
				if (!ec) {
					if (bytes) {
						bytes_out.fetch_add(bytes, std::memory_order_relaxed);
					}
					else {
						std::cout << "bad write - no bytes\n";
					}
					try_write();
				}
				else {
					std::cout << ec.message() + "\n";
				}
			});
		}


		void read() {
			sock_.async_receive_from(asio::buffer(in_buff.data(), in_buff.max_size()), remote_,
			[this](std::error_code ec, std::size_t bytes) 
			{
				if (!ec) {
					if (bytes) {
						in_buff.set_size(bytes);
						std::cout << "read " << bytes << " bytes.\n";
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

			//should callback be called sequentially or simultaneously?????
			// strand reduces speed of processing callbacks
			// but if writer-threads are busy too long
			// this tactics are beneficial
			// because this service allows multiple 'send' operations at a time

			if (notify_mode && strandie) {
				// probably too memory-expensive :(
				auto input = std::make_shared<input_type>(parser.parse());

				// callback will be called only after completion of previous
				strandie->post( [this, input]
				{				
					std::function<void(input_type&&)> task;
					{
						std::lock_guard<std::mutex> guard(callback_mtx);
						task = cback;
					}
					if (task) {
						// should it be moved or just passed by reference?
						// even if unique_ptr comes here
						// it can be passed to callback by reference
						// because we process callbacks from this thread
						task(std::move(*input));
					}
					
				});

			} 
			else {	
				qin.push(parser.parse());
				in_buff.zero();
			}
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



