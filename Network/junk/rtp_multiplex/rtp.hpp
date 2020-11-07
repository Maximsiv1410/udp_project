#include <vector>
#include <iostream>


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
// Solution to inject RTP/RTCP Multiplexing 
// it means opportunity to listen RTP + RTCP at single port
// and this code provides simple interface
// to distinguish RTP from RTCP when parsing raw-byte input
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////


enum class realtime {
	RTP = 0,
	RTCP = 1
};




class realtime_packet {
public:
	virtual realtime type() = 0;

	virtual ~realtime_packet() {}
protected:
	// asio::ip::udp::endpoint remote_;
};

struct packet_wrapper {
	realtime_packet * pack = nullptr;

};




struct rtp_header {
	int rtp;
};
class rtp_packet : public realtime_packet {
public:
	rtp_header header;
	realtime type() override { return realtime::RTP; }
};




struct rtcp_header {
	int rtcp;
};
class rtcp_packet : public realtime_packet {
public:
	rtcp_header header;
	realtime type() override { return realtime::RTCP; }
};





template <typename Buffer>
class parser_base {
protected:
	Buffer & buffer;
	int remote;

	// asio::ip::udp::endpoint remote_;
public:
	parser_base(Buffer & buff /*, remote_ */) : buffer(buff) {}

	virtual packet_wrapper parse() = 0;

	virtual ~parser_base() {
	}
};

template <typename Buffer>
class rtp_parser : public parser_base<Buffer> {
public:
	rtp_parser(Buffer & buff/*, remote_ */) : parser_base<Buffer>(buff) {}

	packet_wrapper parse() {
		rtp_packet *pack = new rtp_packet();
		this->remote;

		return packet_wrapper{ pack };
	}

	~rtp_parser() {
	}
};

template <typename Buffer>
class rtcp_parser : public parser_base<Buffer> {
public:
	rtcp_parser(Buffer & buff/*, remote_ */) : parser_base<Buffer>(buff) {}

	packet_wrapper parse() {
		rtcp_packet *pack = new rtcp_packet();
		this->buffer;

		return packet_wrapper{ pack };
	}

	~rtcp_parser() {
	}

};







class parser_chooser {
public:
	template <typename Buffer>
	static parser_base<Buffer> * choose(Buffer & buff, int num) {
		if (num == 1) {
			return new rtp_parser<Buffer>(buff);
		}
		else if (num == 2) {
			return new rtcp_parser<Buffer>(buff);
		}
	}
};





template <typename Buffer>
class common_parser {
	Buffer & buffer;
	int number = -50;
public:
	common_parser(Buffer & buff, int num) : buffer(buff), number(num) {}

	packet_wrapper parse() {
		parser_base<Buffer> * parser = parser_chooser::choose(buffer, number);
		auto result = parser->parse();
		delete parser;
		return result;
	}
};


/*
// same idea as with parsers
// need common_builder
// and builder_chooser

class builder_base {
	realtime_packet & packet;
public:
	builder_base(realtime_packet & pack) : packet(pack) {}

	template <typename Buffer>
	void extract_to(Buffer & buffer) = 0;

	virtual ~builder_base() {}
};

class rtp_builder  : builder_base {
public:
	rtp_builder(rtp_packet & pack) : builder_base(pack) {}

	template <typename Buffer>
	void extract(Buffer & buffer) {

	}
};

class rtcp_builder  : builder_base {
public:
	rtcp_builder(rtcp_packet & pack) : builder_base(pack) {}

	template <typename Buffer>
	void extract(Buffer & buffer) {

	}
};

*/

int main() {
	using buffer_type = std::vector<char>;
	buffer_type buff;

	int num = 2;


	common_parser<buffer_type> parser(buff, num /*remote*/);
	packet_wrapper result = parser.parse();



	auto type = result.pack->type();

	if (type == realtime::RTP) {
		((rtp_packet*)result.pack)->header.rtp = 12;
		std::cout << "parsed RTP packet\n";
	}
	else if(type == realtime::RTCP) {
		((rtcp_packet*)result.pack)->header.rtcp = 567;
		std::cout << "parsed RTCP packet\n";
	}

	delete result.pack;

}

/*

// probably common for server and client
struct rtp_client_traits {
	using input_type = packet_wrapper;
	using output_type = packet_wrapper;

	template <typename T>
	using input_parser = common_parser<T>;

	using output_builder = common_builder;
};

*/