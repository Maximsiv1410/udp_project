#pragma once


#include "request.hpp"
#include "response.hpp"

#include "request_parser.hpp"
#include "response_parser.hpp"

#include "request_builder.hpp"
#include "response_builder.hpp"

#include "../basic_udp_io.hpp"
#include "../buffer.hpp"

namespace net {
	namespace sip {

		class sip {
		};


		class server_traits {
		public:
			using input_type = request;
			using output_type = response;

			template<typename T>
			using input_parser = request_parser<T>;

			using output_builder = response_builder;
		};

		class client_traits {
		public:
			using input_type = response;
			using output_type = request;

			template<typename T>
			using input_parser = response_parser<T>;

			using output_builder = request_builder;
		};

		using client = basic_udp_service<sip, client_traits, buffer>;
		using server = basic_udp_service<sip, server_traits, buffer>;

	}
}