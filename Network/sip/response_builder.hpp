#pragma once



#include "sip_dependency.hpp"
#include "response.hpp"

using namespace boost;

namespace net {
	namespace sip {
		class response_builder {
			response& sipres;
		public:

			response_builder(response & res) : sipres(res) {}

			// for custom buffers
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

			// for std vector
			void extract(std::vector<char> & buffer) {
				std::size_t length = sipres.total();
				buffer.resize(length);
				
				std::size_t offset = 0;
				std::memcpy(buffer.data() + offset, sipres.version().data(), sipres.version().size());
				offset += sipres.version().size();
				buffer[offset] = ' ';
				offset += 1;

				std::string code = std::to_string(sipres.code());
				std::memcpy(buffer.data() + offset, code.data(), code.size());
				offset += code.size();
				buffer[offset] = ' ';
				offset += 1;

				std::memcpy(buffer.data() + offset, sipres.status().data(), sipres.status().size());
				offset += sipres.status().size();
				std::memcpy(buffer.data() + offset, CRLF, 2);
				offset += 2;

				for (auto & header : sipres.headers()) {
					std::memcpy(buffer.data() + offset, header.first.data(), header.first.size());
					offset += header.first.size();

					std::memcpy(buffer.data() + offset, ": ", 2);
					offset += 2;

					std::memcpy(buffer.data() + offset, header.second.data(), header.second.size());
					offset += header.second.size();

					std::memcpy(buffer.data() + offset, CRLF, 2);
					offset += 2;
				}
				std::memcpy(buffer.data() + offset, CRLF, 2);
				offset += 2;

				std::memcpy(buffer.data() + offset, sipres.body().data(), sipres.body().size());
				offset += sipres.body().size();
			}
		};
	}
}