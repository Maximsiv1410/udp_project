#pragma once
#include "packet.hpp"

namespace net {
	namespace realtime {

		class rtp_builder {
			rtp_packet & pack;
		public:
			rtp_builder(rtp_packet & pack) : pack(pack) {}

			template <typename Buffer> 
			void extract_to(Buffer & buff){
				std::size_t offset = 0;
				std::memcpy(buff.data() + offset, &pack.header(), sizeof(rtp_header));
				offset += sizeof(rtp_header);

				if (!pack.csrc_list().empty()) {
					std::memcpy(buff.data() + offset, pack.csrc_list().data(), pack.header().csrc_count() * 4);
					offset += pack.header().csrc_count() * 4; // 4-byte width * n elements
				}
				
				std::memcpy(buff.data() + offset, pack.payload().data(), pack.payload().size());
				offset += pack.payload().size();
			}

			void extract(char * buff) {
				// check:
				// 1. buff.size >= 1400
				// 2. payload_size <= 1400

				std::size_t offset = 0;

				std::memcpy(buff + offset, &pack.header(), sizeof(rtp_header));
				offset += sizeof(rtp_header);

				if (!pack.csrc_list().empty()) {
					std::memcpy(buff + offset, pack.csrc_list().data(), pack.header().csrc_count() * 4);
					offset += pack.header().csrc_count() * 4; // 4-byte width * n elements
				}


				std::memcpy(buff + offset, pack.payload().data(), pack.payload().size());
				offset += pack.payload().size();

			}

		};
	}
}