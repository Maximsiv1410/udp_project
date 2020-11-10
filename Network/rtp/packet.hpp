#pragma once
#include "rtp_common.hpp"
namespace net {
	namespace realtime {

		class rtp_header {
			typedef unsigned char uchar;

			unsigned char version_ : 2;          // version (V): 2 bits
			unsigned char padding_ : 1;          // padding (P): 1 bit
			unsigned char extension_ : 1;        // extension (X): 1 bit
			unsigned char csrc_count_ : 4;       // CSRC count (CC): 4 bits
			unsigned char marker_ : 1;           // marker (M): 1 bit
			unsigned char pay_type_ : 7;         // payload type (PT): 7 bits

			std::uint16_t seq_num_;              // sequence number: 16 bits
			std::uint32_t timestamp_;            // timestamp: 32 bits
			std::uint32_t ssrc_;                 // SSRC: 32 bits

		public:
			void version(int vers) { version_ = (uchar)vers; }
			int version() { return version_; }

			void padding(bool padd) { padding_ = (uchar)padd; }
			bool padding() { return padding_; }

			void extension(bool ext) { extension_ = (uchar)ext; }
			bool extension() { return extension_; }

			void csrc_count(int csrc_c) { csrc_count_ = (uchar)csrc_c; }
			int csrc_count() { return csrc_count_; }


			void marker(bool marker) { marker_ = (uchar)marker; }
			bool marker() { return marker_; }

			void payload_type(int p_type) { pay_type_ = (uchar)p_type; }
			int payload_type() { return pay_type_; }


			void sequence_number(std::uint16_t seq_num) { seq_num_ = seq_num; }
			std::uint16_t sequence_number() { return seq_num_; }


			void timestamp(std::uint32_t stamp) { timestamp_ = stamp; }
			std::uint32_t timestamp() { return timestamp_; }


			void ssrc(std::uint32_t ssrc) { ssrc_ = ssrc; }
			std::uint32_t ssrc() { return ssrc_; }

		};

		// my personal payload_header only for video data
		struct video_payload_header {
    		std::uint32_t full_size;         // full size in bytes
    		std::uint16_t frag_count;        // number of fragments of full picture
    		std::uint16_t curr_num;          // number of current fragment
		};

		class rtp_packet {
			rtp_header header_;
			std::vector<uint32_t> csrc_list_;
			video_payload_header vph_;
			std::vector<char> payload_;

			asio::ip::udp::endpoint remote_;

		public:
			rtp_packet() = default;
			rtp_packet(rtp_packet&&) = default;
			rtp_packet & operator= (rtp_packet&&) = default;

			/////////////
			video_payload_header & vph() { return vph_; }

			rtp_header & header() { return header_; }

			std::vector<std::uint32_t> & csrc_list() { return csrc_list_; }

			std::vector<char> & payload() { return payload_; }

			asio::ip::udp::endpoint & remote() { return remote_;  }



			void set_remote(asio::ip::udp::endpoint && rem) {
				remote_ = std::move(rem);
			}

			void set_remote(const asio::ip::udp::endpoint & rem) {
				remote_ = rem;
			}
			
			// mb add vector support
			void set_payload(const char * source, std::size_t bytes) {
				payload_.resize(bytes);
				std::memcpy(payload_.data(), source, bytes);
			}

			// mb add vector support
			void set_csrc_list(std::uint32_t * source, std::size_t elems) {
				csrc_list_.resize(elems);
				std::memcpy(csrc_list_.data(), source, elems * sizeof(std::uint32_t));
			}

			std::size_t total() {
				std::size_t size = 0;
				size += sizeof(rtp_header);
				size += 4 * csrc_list_.size();
				size += payload_.size();
				return size;
			}
		};
	}
}