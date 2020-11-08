#pragma once


#include "sip_dependency.hpp"
#include "packet.hpp"

#include "request_parser.hpp"
#include "response_parser.hpp"

#include "../memstream.hpp"

using namespace boost;

namespace net {
	namespace sip {
		
		template <typename Buffer>
		class parser {
			Buffer & buffer;
			asio::ip::udp::endpoint remote;
		public:
			parser(Buffer & buff, asio::ip::udp::endpoint&& rem) : buffer(buff), remote(std::move(rem)) {}

			packet_wrapper parse() {
				if (buffer[0] == 'S') // check for 'SIP/2.0'
				{
					response_parser<Buffer> parser(buffer, std::move(remote));
					return packet_wrapper{parser.parse()};
				}
				else {
					request_parser<Buffer> parser(buffer, std::move(remote));
					return packet_wrapper{parser.parse()};
				}
			}
		};
	}
}