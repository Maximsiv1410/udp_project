#pragma once


#include "sip_dependency.hpp"
#include "request.hpp"
using namespace boost;

namespace net {
	namespace sip {
		class request_builder {
		public:
			request & sipreq;

			request_builder(request & req)
				: sipreq(req) {
			}

			template <typename Buffer>
			void extract_to(Buffer && buffer) {
				std::size_t offset = 0;
				std::memcpy(buffer.data() + offset, sipreq.method().data(), sipreq.method().size());
				offset += sipreq.method().size();
				buffer[offset] = ' ';
				offset += 1;

				std::memcpy(buffer.data() + offset, sipreq.uri().data(), sipreq.uri().size());
				offset += sipreq.uri().size();
				buffer[offset] = ' ';
				offset += 1;

				std::memcpy(buffer.data() + offset, sipreq.version().data(), sipreq.version().size());
				offset += sipreq.version().size();
				std::memcpy(buffer.data() + offset, CRLF, 2);
				offset += 2;

				for (auto & header : sipreq.headers()) {
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

				if (sipreq.body().size()) {
					std::memcpy(buffer + offset, sipreq.body().data(), sipreq.body().size());
					offset += sipreq.body().size();
				}

			}


			void extract(char * buffer) {
				std::size_t length = sipreq.total();

				std::size_t offset = 0;
				std::memcpy(buffer + offset, sipreq.method().data(), sipreq.method().size());
				offset += sipreq.method().size();
				buffer[offset] = ' ';
				offset += 1;

				std::memcpy(buffer + offset, sipreq.uri().data(), sipreq.uri().size());
				offset += sipreq.uri().size();
				buffer[offset] = ' ';
				offset += 1;

				std::memcpy(buffer + offset, sipreq.version().data(), sipreq.version().size());
				offset += sipreq.version().size();
				std::memcpy(buffer + offset, CRLF, 2);
				offset += 2;

				for (auto & header : sipreq.headers()) {
					std::memcpy(buffer + offset, header.first.data(), header.first.size());
					offset += header.first.size();

					std::memcpy(buffer + offset, ": ", 2);
					offset += 2;

					std::memcpy(buffer + offset, header.second.data(), header.second.size());
					offset += header.second.size();

					std::memcpy(buffer + offset, CRLF, 2);
					offset += 2;
				}

				std::memcpy(buffer + offset, CRLF, 2);
				offset += 2;

				if (sipreq.body().size()) {
					std::memcpy(buffer + offset, sipreq.body().data(), sipreq.body().size());
					offset += sipreq.body().size();
				}
			}
		};
	}
}