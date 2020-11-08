#pragma once


#include "sip_dependency.hpp"
#include "message.hpp"

#include "request_builder.hpp"
#include "response_builder.hpp"

using namespace boost;

namespace net {
	namespace sip {
		class builder {
			message_wrapper & sippack;
		public:
			builder(message_wrapper & wrapper) : sippack(wrapper) {}

			template <typename Buffer>
			void extract_to(Buffer & buff) {
					if (sippack.storage()->type() == sip_type::Response) {
						response_builder resbuilder((sip::response&)(*sippack.storage()));
						resbuilder.extract_to(buff);
					}
					else if(sippack.storage()->type() == sip_type::Request) {
						request_builder reqbuilder((sip::request&)(*sippack.storage()));
						reqbuilder.extract_to(buff);
					}
			}


			void extract(std::vector<char> & buff) {
				if (sippack.storage()->type() == sip_type::Response) {
						response_builder resbuilder((sip::response&)(*sippack.storage()));
						resbuilder.extract(buff);
					}
					else if(sippack.storage()->type() == sip_type::Request) {
						request_builder reqbuilder((sip::request&)(*sippack.storage()));
						reqbuilder.extract(buff);
					}
			}
		};		
	}
}