#pragma once


#include "sip_dependency.hpp"
#include "request.hpp"

#include "../memstream.hpp"

using namespace boost;

namespace net {
	namespace sip {
		template <typename Buffer>
		class request_parser {
			Buffer & buffer;
			asio::ip::udp::endpoint endpoint;
		public:
			request_parser(Buffer & buff, asio::ip::udp::endpoint && ep) 
				: buffer(buff), 
				endpoint(std::move(ep)) 
			{}

			std::unique_ptr<request> parse() {
				request req;
				req.remote() = std::move(endpoint);

				auto bodyptr = strstr(buffer.data(), "\r\n\r\n");
				auto diff = bodyptr - buffer.data();

				imemstream in(buffer.data(), diff);

				in >> req.method();
				in >> req.uri();
				in >> req.version();

				in.ignore(2);

				std::string header_name;
				std::string header_value;
				while (!in.eof()) {
					std::getline(in, header_name, ':');
					if (!in.eof()) {
						in.get();
						std::getline(in, header_value, '\r');
						in.get();

						req.add_header(header_name, header_value);
					}
				}

				// shoudl be SDP parsing here, but now skipped this

				req.body().resize(buffer.size() - diff - 4);
				std::memcpy(req.body().data(), bodyptr + 4, req.body().size());

				return std::make_unique<request>(std::move(req));
			}
		};
	}
}