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

		class packet {
			protected:
				sdp sdp_;
				asio::ip::udp::endpoint remote_;

				std::map<std::string, std::string> headers_;
				std::vector<char> body_; // later it will be SDP

			public:
				packet() = default;
				packet(packet&&) = default;
				packet & operator=(packet&&) = default;
				virtual ~packet() {}

				packet & set_remote(const asio::ip::udp::endpoint& remote) { remote_ = remote; return *this; }
				packet & set_remote(asio::ip::udp::endpoint&& remote) { remote_ = std::move(remote); return *this; }

				asio::ip::udp::endpoint & remote() & { return remote_; }

				const std::map<std::string, std::string> & headers() { return headers_; }

				std::vector<char> & body() { return body_; }

				packet & set_body(const std::string & input) {
					body_.resize(input.size());
					std::memcpy(body_.data(), input.data(), input.size());

					return *this;
				}

				packet & set_body(const char* input, std::size_t size) {
					body_.resize(size);
					std::memcpy(body_.data(), input, size);

					return *this;
				}

				packet &  append_body(std::string & input) {
					auto was = body_.size();
					body_.resize(was + input.size());
					std::memcpy(body_.data() + was, input.data(), input.size());

					return *this;
				}

				packet & add_header(std::string & left, std::string & right) {
					headers_[left] = right;

					return *this;
				}

				packet & add_header(const char* left, const char* right) {
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


		class packet_wrapper {
			std::unique_ptr<packet> storage_{nullptr};
		public:

			packet_wrapper(std::unique_ptr<packet> in) : storage_(std::move(in)) {}
			packet_wrapper() = default;
			packet_wrapper(packet_wrapper&&) = default;
			packet_wrapper & operator=(packet_wrapper&&) = default;

			asio::ip::udp::endpoint & remote() { return storage_->remote(); }
			std::size_t total() { return storage_->total(); }

			std::unique_ptr<packet> & storage() { return storage_; }
		};
	}
}