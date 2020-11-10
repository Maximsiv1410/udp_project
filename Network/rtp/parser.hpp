#pragma once

#include "rtp_common.hpp"

#include "packet.hpp"

namespace net {
	namespace realtime {

		template <typename Buffer>
		class rtp_parser {
			Buffer & buffer;
			asio::ip::udp::endpoint remote;
		public:
			rtp_parser(Buffer & buff, asio::ip::udp::endpoint && rem) 
			: 
			buffer(buff),
			 remote(std::move(rem)) {}

			rtp_packet parse() {
				std::size_t offset = 0;
				rtp_packet pack;

				pack.set_remote(std::move(remote));

				std::memcpy(&pack.header(), buffer.data() + offset, sizeof(rtp_header));
				offset += sizeof(rtp_header);

				if (pack.header().csrc_count() > 0) {
					std::size_t csrc_len = 4 * pack.header().csrc_count();
					pack.csrc_list().resize(pack.header().csrc_count());
					std::memcpy(pack.csrc_list().data(), buffer.data() + offset, csrc_len);
					offset += csrc_len;
				}

				////// video_payload_header section
				std::memcpy(&pack.vph(), buffer.data() + offset, sizeof(video_payload_header));
				offset += sizeof(video_payload_header);
				////// video_payload_header section

				// TODO :
				std::size_t payload_len = buffer.size() - offset;
				pack.payload().resize(payload_len);
				std::memcpy(pack.payload().data(), buffer.data() + offset, payload_len);
				offset += payload_len;

				return pack;
			}
		};

	}
}