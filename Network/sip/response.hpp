#pragma once


#include "sip_dependency.hpp"
#include "sdp.hpp"
#include "message.hpp"

using namespace boost;

namespace net {
	namespace sip {
		class response : public message {

			std::size_t code_;
			std::string version_;
			std::string status_;
		public:
			response() = default;
			response & operator=(response&& other) = default;
			response(response&& other) = default;

			void set_version(const std::string & version) { version_ = version; }
			void set_version(std::string && version) { version_ = std::move(version); }
			void set_version(const char * version) { version_.assign(version); }

			void set_code(std::size_t code) { code_ = code; }

			void set_status(const std::string & status) { status_ = status; }
			void set_status(std::string && status) { status_ = std::move(status); }
			void set_status(const char* status) { status_.assign(status); }

			std::string & version() { return version_; }


			std::size_t & code() { return code_; }

			std::string& status() { return status_; }

			sip_type type() override { return sip_type::Response; }

			std::size_t total() {				
				std::size_t length = message::total();

				length += version_.size();
				length += 1; // SPACE

				std::string code = std::to_string(code_);
				length += code.size();
				length += 1; // SPACE

				length += status_.size();
				length += 2; // CRLF

				return length;
			}

		};
	}
}