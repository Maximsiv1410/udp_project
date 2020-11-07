#pragma once

#include "sip_dependency.hpp"


using namespace boost;

namespace net {
	namespace sip {
		class sdp {
			std::map<char, std::string> headers_;
		};

	}
}