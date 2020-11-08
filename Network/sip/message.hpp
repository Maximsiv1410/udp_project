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

				message & set_remote(const asio::ip::udp::endpoint& remote) { remote_ = remote; return *this; }
				message & set_remote(asio::ip::udp::endpoint&& remote) { remote_ = std::move(remote); return *this; }

				asio::ip::udp::endpoint & remote() & { return remote_; }

				const std::map<std::string, std::string> & headers() { return headers_; }

				std::vector<char> & body() { return body_; }

				message & set_body(const std::string & input) {
					body_.resize(input.size());
					std::memcpy(body_.data(), input.data(), input.size());

					return *this;
				}

				message & set_body(const char* input, std::size_t size) {
					body_.resize(size);
					std::memcpy(body_.data(), input, size);

					return *this;
				}

				message &  append_body(std::string & input) {
					auto was = body_.size();
					body_.resize(was + input.size());
					std::memcpy(body_.data() + was, input.data(), input.size());

					return *this;
				}

				message & add_header(std::string & left, std::string & right) {
					headers_[left] = right;

					return *this;
				}

				message & add_header(const char* left, const char* right) {
					headers_[left] = right;

					return *this;
				}

				std::size_t total() { 
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