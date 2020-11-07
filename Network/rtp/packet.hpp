#pragma once
#include "rtp_common.hpp"
namespace net {
	namespace rtp {

		struct rtp_header {
			unsigned char version_ : 2;          // version (V): 2 bits
			unsigned char padding_ : 1;          // padding (P): 1 bit
			unsigned char extension_ : 1;        // extension (X): 1 bit
			unsigned char csrc_count_ : 4;       // CSRC count (CC): 4 bits
			unsigned char marker_ : 1;           // marker (M): 1 bit
			unsigned char pay_type_ : 7;         // payload type (PT): 7 bits

			std::uint16_t seq_num_;              // sequence number: 16 bits
			std::uint16_t timestamp_;            // timestamp: 32 bits
			std::uint32_t ssrc_;                 // SSRC: 32 bits

			int version() { return version_; }

			bool padding() { return padding_; }

			bool extension() { return extension_; }

			int csrc_count() { return csrc_count_; }

			bool marker() { return marker_; }

			int payload_type() { return pay_type_; }

			std::uint16_t sequence_number() { return seq_num_; }

			std::uint16_t timestamp() { return timestamp_; }

			std::uint16_t ssrc() { return ssrc_; }

		};


		struct rtp_packet {
			rtp_header header_;

			std::vector<char> csrc_list_;
			std::vector<char> payload_;

			//asio::ip::udp::endpoint remote_;

			rtp_header & header() { return header_; }

			std::vector<char> & csrc_list() { return csrc_list_; }

			std::vector<char> & payload() { return payload_; }

		};
	}
}