#pragma once


#include "sip_dependency.hpp"
#include "sdp.hpp"

using namespace boost;

namespace net {
	namespace sip {

		enum class sip_type {
			Response,
			Request
		};

		class message {
			protected:
				sdp sdp_;
				asio::ip::udp::endpoint remote_;

				std::map<std::string, std::string> headers_;
				std::vector<char> body_; // later it will be SDP

			public:
				message() = default;
				message(message&&) = default;
				message & operator=(message&&) = default;
				virtual ~message() {}

				void set_remote(const asio::ip::udp::endpoint& remote) { remote_ = remote; }
				void set_remote(asio::ip::udp::endpoint&& remote) { remote_ = std::move(remote); }

				asio::ip::udp::endpoint & remote() & { return remote_; }

				std::map<std::string, std::string> & headers() { return headers_; }

				std::vector<char> & body() { return body_; }

				void set_body(const std::string & input) {
					body_.resize(input.size());
					std::memcpy(body_.data(), input.data(), input.size());

				}

				void set_body(const char* input, std::size_t size) {
					body_.resize(size);
					std::memcpy(body_.data(), input, size);
				}

				void append_body(std::string & input) {
					auto was = body_.size();
					body_.resize(was + input.size());
					std::memcpy(body_.data() + was, input.data(), input.size());
				}

				void add_header(std::string & left, std::string & right) {
					headers_[left] = right;				
				}

				void add_header(const char* left, const char* right) {
					headers_[std::string(left)] = std::string(right);
				}

				std::size_t total_headers_body() {
					std::size_t length = 0;
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

				virtual std::size_t total() = 0;			


			virtual sip_type type() = 0;
		};


		class message_wrapper {
			std::unique_ptr<message> storage_{nullptr};
		public:

			message_wrapper(std::unique_ptr<message> in) : storage_(std::move(in)) {}
			message_wrapper() = default;
			message_wrapper(message_wrapper&&) = default;
			message_wrapper & operator=(message_wrapper&&) = default;

			asio::ip::udp::endpoint & remote() { return storage_->remote(); }
			std::size_t total() { return storage_->total(); }

			std::unique_ptr<message> & storage() { return storage_; }
		};
	}
}