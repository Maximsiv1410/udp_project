#pragma once

#include "sip_dependency.hpp"
#include "sdp.hpp"

using namespace boost;


namespace net {
	namespace sip {
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
	}
}