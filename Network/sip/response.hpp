#pragma once


#include "sip_dependency.hpp"
#include "sdp.hpp"

using namespace boost;

namespace net {
	namespace sip {
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
	}
}