#pragma once


#include "sip_dependency.hpp"
#include "sdp.hpp"

#include "response.hpp"
#include "../memstream.hpp"

using namespace boost;

namespace net {
	namespace sip {
		template<typename Buffer>
		class response_parser {
			Buffer & buffer;
			asio::ip::udp::endpoint endpoint;
		public:
			response_parser(Buffer & buff, asio::ip::udp::endpoint && ep)
				: buffer(buff),
				endpoint(std::move(ep)) 	
			{
			}

			std::unique_ptr<response> parse() {
				response resp;
				resp.set_remote(std::move(endpoint));
	
				// if bodyptr = nullptr => exception ?
				auto bodyptr = strstr(buffer.data(), "\r\n\r\n");
				auto diff = bodyptr - buffer.data();

				imemstream in(buffer.data(), diff);

				in >> resp.version();
				in >> resp.code();
				in >> resp.status();

				in.ignore(2);

				std::string header_name;
				std::string header_value;
				while (!in.eof()) {
					std::getline(in, header_name, ':');
					if (!in.eof()) {
						in.get();
						std::getline(in, header_value, '\r');
						in.get();

						resp.add_header(header_name, header_value);
					}
				}

				resp.body().resize(buffer.size() - diff - 4);
				std::memcpy(resp.body().data(), bodyptr + 4, resp.body().size());

				return std::make_unique<response>(std::move(resp));
			}

		};
	}
}