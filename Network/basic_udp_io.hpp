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

		io_context::strand write_strand;
		io_context::strand handler_strand;

		bool notify_mode {false};
	
		std::atomic<unsigned long long> bytes_out{0};
        std::atomic<unsigned long long> bytes_in{0};

        std::mutex w_mtx;
        bool writing = false;

	public:
		basic_udp_service(io_context& ios, ip::udp::socket& sock)
			: ios_(ios)
			, sock_(sock) 
			, write_strand(ios)
			, handler_strand(ios)
		{
		}

		virtual ~basic_udp_service() {}

		unsigned long long written_out() {
			return bytes_out.load(std::memory_order_relaxed);
		}

        unsigned long long read_in() {
            return bytes_in.load(std::memory_order_relaxed);
        }


		void start(bool notify = false) {
			notify_mode = notify;
			read();
		}

		// should it be here?
		void shutdown() {
			if (sock_.is_open()) {
				sock_.close();
			}
		}

		void set_callback(std::function<void(input_type&&)> callback) {
			std::lock_guard<std::mutex> guard(callback_mtx);
			cback = std::move(callback);
		}

		void poll(/*size_t count, */std::function<void(input_type&&)> task) {
			if (notify_mode) return;

			input_type input;
			while (!ios_.stopped()) {
				qin.wait_and_pop(input);		
				task(std::move(input)); // or call default callback?
			}
		} 


		// send message bypassing queue
		void async_send(output_type & output) {
			//auto out = std::make_shared<char[]>(output.total());
			std::shared_ptr<char[]> out(new char[output.total()]);

			output_builder builder(output);
			builder.extract(out.get());

            write_strand.post([this, out, remote{output.remote()}, total{output.total()}]
			{
				std::size_t bytes = sock_.send_to(asio::buffer(out.get(), total), remote);
				if (!bytes) {
					std::cout << "Can't write to socket -> exiting\n";
				}
				else {

				}
            });
		}


		void enqueue(output_type & output) {
			qout.push(std::move(output));
		}

		void init_send() {
			bool busy;
			{
				std::lock_guard<std::mutex> guard(w_mtx);
				busy = writing;
			}
			if (!busy && !qout.empty()) {
				write_strand.post([this]{ write_queue(); });
			}
		}



	private:

		void write_queue() {
			{
				std::lock_guard<std::mutex> guard(w_mtx);
				writing = true;
			}

			while(!qout.empty()) {
				output_type message;
				bool attempt = qout.try_pop(message);
				if (!attempt) return;
				
				char * buffer = new char[message.total()];

				output_builder builder(message);
				builder.extract(buffer);

				std::size_t bytes = sock_.send_to(asio::buffer(buffer, message.total()), message.remote());

				delete[] buffer;
				if (!bytes) {				
					std::cout << "Can't write to socket, exiting\n";
					break;
				}
				else {

				}
			}

			{
				std::lock_guard<std::mutex> guard(w_mtx);
				writing = false;
			}
		}


		void read() {
			sock_.async_receive_from(asio::buffer(in_buff.data(), in_buff.max_size()), remote_,
			[this](std::error_code ec, std::size_t bytes) 
			{
				if (!ec) {
					if (bytes) {
						in_buff.set_size(bytes);
                        bytes_in.store(bytes, std::memory_order_relaxed);
                        //std::cout << "read " << bytes << " bytes.\n";
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

			if (notify_mode) {
				// probably too memory-expensive :(
				auto input = std::make_shared<input_type>(parser.parse());

				// callback will be called only after completion of previous
				// because we are queueing them consistently after every 'read'
				handler_strand.post( [this, input /*, cback */]
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



