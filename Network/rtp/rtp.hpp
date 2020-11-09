#pragma once

#include "../basic_udp_io.hpp"
#include "../buffer.hpp"

#include "builder.hpp"
#include "parser.hpp"
#include "packet.hpp"


namespace net {
	namespace realtime {
		struct rtp {};

		struct client_traits {
			using input_type = rtp_packet;
			using output_type = rtp_packet;

			template<typename T>
			using input_parser = rtp_parser<T>;

			using output_builder = rtp_builder;
		};

		struct server_traits {
			using input_type = rtp_packet;
			using output_type = rtp_packet;

			template<typename T>
			using input_parser = rtp_parser<T>;

			using output_builder = rtp_builder;
		};


		using server = basic_udp_service<rtp, server_traits, buffer>;
		using client = basic_udp_service<rtp, client_traits, buffer>;
	}
}