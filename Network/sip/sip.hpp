#pragma once


#include "message.hpp"

#include "parser.hpp"
#include "builder.hpp"

#include "../basic_udp_io.hpp"
#include "../buffer.hpp"

namespace net {
	namespace sip {

		class sip {
		};


		class server_traits {
		public:
			using input_type = message_wrapper;
			using output_type = message_wrapper;

			template<typename T>
			using input_parser = parser<T>;

			using output_builder = builder;
		};

		class client_traits {
		public:
			using input_type = message_wrapper;
			using output_type = message_wrapper;

			template<typename T>
			using input_parser = parser<T>;

			using output_builder = builder;
		};

		using client = basic_udp_service<sip, client_traits, buffer>;
		using server = basic_udp_service<sip, server_traits, buffer>;

	}
}