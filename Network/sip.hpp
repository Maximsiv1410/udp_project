#pragma once
#include <boost/asio.hpp>
using namespace boost;

#include "net_essential.hpp"

#include "memstream.hpp"
#include "buffer.hpp"
#include "basic_udp_io.hpp"

namespace net {
	namespace sip {

		class sdp {
			std::map<char, std::string> headers_;
		};


		///////////////////////////////////////////////////////////////////
		/////// Containers for holding
		/////// Request and Responses
		/////// in SIP manner
		//////////////////////////////////////////////////////////////////
		class response {
			sdp sdp_;

			asio::ip::udp::endpoint remote_;

			std::map<std::string, std::string> headers_;
			std::vector<char> body_;

			std::size_t code_;
			std::string version_;
			std::string status_;
		public:
			response() = default;
			response & operator=(response&& other) = default;
			response(response&& other) = default;

			response & set_version(const std::string & version) { version_ = version; return *this; }
			response & set_version(std::string && version) { version_ = std::move(version); return *this; }
			response & set_version(const char * version) { version_.assign(version); return *this; }

			response & set_remote(const asio::ip::udp::endpoint& remote) { remote_ = remote; return *this; }
			response & set_remote(asio::ip::udp::endpoint&& remote) { remote_ = std::move(remote); return *this; }

			response & set_code(std::size_t code) { code_ = code; return *this; }

			response & set_status(const std::string & status) { status_ = status; return *this; }
			response & set_status(std::string && status) { status_ = std::move(status); return *this; }
			response & set_status(const char* status) { status_.assign(status); return *this; }

			std::string & version() { return version_; }

			std::map<std::string, std::string> & headers() { return headers_; }

			std::vector<char> & body() { return body_; }

			asio::ip::udp::endpoint & remote() { return remote_; }

			std::size_t & code() { return code_; }

			std::string& status() { return status_; }

			response & set_body(const std::string & input) {
				body_.clear();
				body_.resize(input.size());
				std::memcpy(body_.data(), input.data(), input.size());

				return *this;
			}

			response &  append_body(std::string & input) {
				auto was = body_.size();
				body_.resize(was + input.size());
				std::memcpy(body_.data() + was, input.data(), input.size());

				return *this;
			}

			response & add_header(std::string & left, std::string & right) {
				headers_[left] = right;

				return *this;
			}

			response & add_header(const char* left, const char* right) {
				headers_[left] = right;

				return *this;
			}

			std::size_t total() {
				std::size_t length = 0;
				length += version_.size();
				length += 1; // SPACE

				std::string code = std::to_string(code_);
				length += code.size();
				length += 1; // SPACE

				length += status_.size();
				length += 2; // CRLF


				for (auto & header : headers_) {
					length += header.first.size();
					length += 2; // for ': '
					length += header.second.size();
					length += 2; // CRLF
				}

				length += 2; // additional CRLF

				length += body_.size();

				return length;
			}
		};

		class request {
			sdp sdp_;

			asio::ip::udp::endpoint remote_;

			std::map<std::string, std::string> headers_;
			std::vector<char> body_;

			std::string method_;
			std::string uri_;
			std::string version_;

		public:
			request() = default;
			request & operator=(request&& other) = default;
			request(request&& other) = default;

			request & set_method(const std::string & meth) { method_ = meth; return *this; }
			request & set_method(std::string && meth) { method_ = meth; return *this; }
			request & set_method(const char * meth) { method_ = meth; return *this; }

			request & set_uri(const std::string & uri) { uri_ = uri; return *this; }
			request & set_uri(std::string && uri) { uri_ = std::move(uri); return *this; }
			request & set_uri(const char * uri) { uri_.assign(uri); return *this; }

			request & set_version(const std::string & version) { version_ = version; return *this; }
			request & set_version(std::string && version) { version_ = std::move(version); return *this; }
			request & set_version(const char* version) { version_.assign(version); return *this; }

			request & set_remote(const asio::ip::udp::endpoint& remote) { remote_ = remote; return *this; }
			request & set_remote(asio::ip::udp::endpoint&& remote) { remote_ = std::move(remote); return *this; }

			std::string & method() { return method_; }

			std::string & uri() { return uri_; }

			std::string & version() { return version_; }

			asio::ip::udp::endpoint & remote() & { return remote_; }

			const std::map<std::string, std::string> & headers() { return headers_; }

			std::vector<char> & body() { return body_; }

			request & set_body(std::string & input) {
				body_.resize(input.size());
				std::memcpy(body_.data(), input.data(), input.size());

				return *this;
			}

			request & set_body(const char* input, std::size_t size) {
				body_.resize(size);
				std::memcpy(body_.data(), input, size);

				return *this;
			}

			request &  append_body(std::string & input) {
				auto was = body_.size();
				body_.resize(was + input.size());
				std::memcpy(body_.data() + was, input.data(), input.size());

				return *this;
			}

			request & add_header(std::string & left, std::string & right) {
				headers_[left] = right;

				return *this;
			}

			request & add_header(const char* left, const char* right) {
				headers_[left] = right;

				return *this;
			}

			std::size_t total() {
				std::size_t length = 0;
				length += method_.size();
				length += 1; // SPACE

				length += uri_.size();
				length += 1; // SPACE

				length += version_.size();
				length += 2; // CRLF

				for (auto & header : headers_) {
					length += header.first.size();
					length += 2; // for ': '
					length += header.second.size();
					length += 2; // CRLF
				}

				// additional CRLF
				length += 2;

				length += body_.size();

				return length;
			}
		};





		///////////////////////////////////////////////////////////////////
		/////// Message Parsers
		/////// Raw Buffers -> High Level Containers
		//////////////////////////////////////////////////////////////////
		template <typename Buffer>
		class request_parser {
			Buffer & buffer;
			asio::ip::udp::endpoint endpoint;
		public:
			request_parser(Buffer & buff, asio::ip::udp::endpoint && ep) 
				: buffer(buff), 
				endpoint(std::move(ep)) 
			{}

			request parse() {
				request request;
				request.remote() = std::move(endpoint);

				auto bodyptr = strstr(buffer.data(), "\r\n\r\n");
				auto diff = bodyptr - buffer.data();

				imemstream in(buffer.data(), diff);

				in >> request.method();
				in >> request.uri();
				in >> request.version();

				in.ignore(2);

				std::string header_name;
				std::string header_value;
				while (!in.eof()) {
					std::getline(in, header_name, ':');
					if (!in.eof()) {
						in.get();
						std::getline(in, header_value, '\r');
						in.get();

						request.add_header(header_name, header_value);
					}
				}

				// shoudl be SDP parsing here, but now skipped this

				request.body().resize(buffer.size() - diff - 4);
				std::memcpy(request.body().data(), bodyptr + 4, request.body().size());

				return request;
			}
		};

		template<typename Buffer>
		class response_parser {
			Buffer & buffer;/*
			asio::ip::udp::endpoint endpoint;*/
		public:
			response_parser(Buffer & buff, asio::ip::udp::endpoint && ep)
				: buffer(buff)/*,
				endpoint(std::move(ep)) */	
			{
			}

			response parse() {
				response resp;
				/*resp.set_remote(std::move(endpoint));*/

				auto bodyptr = strstr(buffer.data(), "\r\n\r\n");
				auto diff = bodyptr - buffer.data();

				imemstream in(buffer.data(), diff);

				in >> resp.version();
				in >> resp.code();
				in >> resp.status();

				in.ignore(2);

				std::string header_name;
				std::string header_value;
				while (!in.eof()) {
					std::getline(in, header_name, ':');
					if (!in.eof()) {
						in.get();
						std::getline(in, header_value, '\r');
						in.get();

						resp.add_header(header_name, header_value);
					}
				}

				resp.body().resize(buffer.size() - diff - 4);
				std::memcpy(resp.body().data(), bodyptr + 4, resp.body().size());

				return resp;
			}

		};




		///////////////////////////////////////////////////////////////////
		/////// Message Builders
		/////// High Level Containers -> Raw buffers
		//////////////////////////////////////////////////////////////////
		class request_builder {
		public:
			request & sipreq;

			request_builder(request & req)
				: sipreq(req) {
			}

			template <typename Buffer>
			void extract_to(Buffer && output) {
				std::size_t offset = 0;
				std::memcpy(output.data() + offset, sipreq.method().data(), sipreq.method().size());
				offset += sipreq.method().size();
				output[offset] = ' ';
				offset += 1;

				std::memcpy(output.data() + offset, sipreq.uri().data(), sipreq.uri().size());
				offset += sipreq.uri().size();
				output[offset] = ' ';
				offset += 1;

				std::memcpy(output.data() + offset, sipreq.version().data(), sipreq.version().size());
				offset += sipreq.version().size();
				std::memcpy(output.data() + offset, CRLF, 2);
				offset += 2;

				for (auto & header : sipreq.headers()) {
					std::memcpy(output.data() + offset, header.first.data(), header.first.size());
					offset += header.first.size();

					std::memcpy(output.data() + offset, ": ", 2);
					offset += 2;

					std::memcpy(output.data() + offset, header.second.data(), header.second.size());
					offset += header.second.size();

					std::memcpy(output.data() + offset, CRLF, 2);
					offset += 2;
				}
				std::memcpy(output.data() + offset, CRLF, 2);
				offset += 2;

				std::memcpy(output.data() + offset, sipreq.body().data(), sipreq.body().size());
				offset += sipreq.body().size();

				output.set_size(offset);
			}


			void extract(std::vector<char> & buffer) {
				std::size_t length = sipreq.total();
			}
		};




		class response_builder {
			response& sipres;
		public:

			response_builder(response & res) : sipres(res) {}

			template <typename Buffer>
			void extract_to(Buffer && output) {
				std::size_t offset = 0;
				std::memcpy(output.data() + offset, sipres.version().data(), sipres.version().size());
				offset += sipres.version().size();
				output[offset] = ' ';
				offset += 1;


				std::string code = std::to_string(sipres.code());
				std::memcpy(output.data() + offset, code.data(), code.size());
				offset += code.size();
				output[offset] = ' ';
				offset += 1;


				std::memcpy(output.data() + offset, sipres.status().data(), sipres.status().size());
				offset += sipres.status().size();
				std::memcpy(output.data() + offset, CRLF, 2);
				offset += 2;

				for (auto & header : sipres.headers()) {
					std::memcpy(output.data() + offset, header.first.data(), header.first.size());
					offset += header.first.size();

					std::memcpy(output.data() + offset, ": ", 2);
					offset += 2;

					std::memcpy(output.data() + offset, header.second.data(), header.second.size());
					offset += header.second.size();

					std::memcpy(output.data() + offset, CRLF, 2);
					offset += 2;
				}
				std::memcpy(output.data() + offset, CRLF, 2);
				offset += 2;

				std::memcpy(output.data() + offset, sipres.body().data(), sipres.body().size());
				offset += sipres.body().size();

				output.set_size(offset);
			}


			void extract(std::vector<char> & buffer) {
				std::size_t length = sipres.total();
				
			}
		};




		//////////////////////////////////////////////////////////////////
		///// Client and Server Traits 
		/////////////////////////////////////////////////////////////////
		class sip {
		};


		class server_traits {
		public:
			using input_type = request;
			using output_type = response;

			template<typename T>
			using input_parser = request_parser<T>;

			using output_builder = response_builder;
		};

		class client_traits {
		public:
			using input_type = response;
			using output_type = request;

			template<typename T>
			using input_parser = response_parser<T>;

			using output_builder = request_builder;

			//using send_method = asio::ip::udp::socket::async_send;
		};

		using client = basic_udp_service<sip, client_traits, buffer>;
		using server = basic_udp_service<sip, server_traits, buffer>;
	}
}