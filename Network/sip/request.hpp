#pragma once

#include "sip_dependency.hpp"
#include "message.hpp"
#include "sdp.hpp"

using namespace boost;


namespace net {
	namespace sip {
		class request : public message {
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

			std::string & method() { return method_; }

			std::string & uri() { return uri_; }

			std::string & version() { return version_; }

			sip_type type() override { return sip_type::Request; }
			
			std::size_t total() {
				std::size_t length = message::total();
				length += method_.size();
				length += 1; // SPACE

				length += uri_.size();
				length += 1; // SPACE

				length += version_.size();
				length += 2; // CRLF

				return length;
			}
		};
	}
}