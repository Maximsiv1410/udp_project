#pragma once
#include <boost/asio.hpp>
#include <iostream>
#include <optional>

#include "../../Network/sip/sip.hpp"
#include "session.hpp"

using namespace boost;
using namespace net;

class session_handler : public sip::server {
	asio::io_context & ios;
	asio::ip::udp::endpoint local;
	asio::ip::udp::socket sock;

	std::vector<session> sessions; // thread safe ?
	std::vector<session> rtpsess;

	std::map<std::string, asio::ip::udp::endpoint> dbase; // ;)

	std::map<std::string, void(session_handler::*)(sip::request &)> request_map;
	std::map<std::string, void(session_handler::*)(sip::response &)> response_map;

public:

	session_handler(asio::io_context & ioc, std::uint16_t port = 5060) 
	: sip::server(ioc, sock)
	, ios(ioc)
	, local(asio::ip::udp::v4(), port)
	, sock(ios, local)
	{
		request_map["REGISTER"] = &session_handler::do_register;
		request_map["INVITE"] = &session_handler::tracked_invite;
		request_map["ACK"] = &session_handler::tracked_ack;
		request_map["BYE"] = &session_handler::tracked_bye;

		response_map["Trying"] = &session_handler::tracked_trying;
		response_map["Ringing"] = &session_handler::tracked_ringing;
		response_map["OK"] = &session_handler::tracked_ok;

	}

	void stop() {
		if (sock.is_open()) {
			sock.close();
		}
	}

	void add_session_sip(const asio::ip::udp::endpoint & caller, const asio::ip::udp::endpoint & callee) {
		sessions.emplace_back(caller, callee);
	}

	void add_session_rtp(const asio::ip::udp::endpoint & caller, const asio::ip::udp::endpoint & callee) {
		rtpsess.emplace_back(caller, callee);
	}

	bool has_session_sip(const asio::ip::udp::endpoint & ep) {
		for (auto & s : sessions) {
			if (s.is_participant(ep)) return true;
		}
		return false;
	}

	bool has_session_rtp(const asio::ip::udp::endpoint & ep) {
		for (auto & s : rtpsess) {
			if (s.is_participant(ep)) return true;
		}
		return false;
	}

	std::optional<asio::ip::udp::endpoint> get_partner_sip(const asio::ip::udp::endpoint & ep) {
		for (auto & s : sessions) {
			if (s.is_participant(ep)) {
				return std::make_optional(s.partner(ep));
			}
		}
		return std::nullopt;
	}

	std::optional<asio::ip::udp::endpoint> get_partner_rtp(const asio::ip::udp::endpoint & ep) {
		for (auto & s : rtpsess) {
			if (s.is_participant(ep)) {
				return std::make_optional(s.partner(ep));
			}
		}
		return std::nullopt;
	}

	void process(sip::message & msg) {
		if (msg.type() == sip::sip_type::Request) {
			sip::request & request = (sip::request&)msg;
			if (request_map.count(request.method())) {
				(this->*request_map[request.method()])(request);
			}
			else {
				std::cerr << "No handler for method: " << request.method() << '\n';
			}
		}
		else if(msg.type() == sip::sip_type::Response) {
			sip::response & response = (sip::response&)msg;
			if (response_map.count(response.status())) {
				(this->*response_map[response.status()])(response);
			}
			else {
				std::cerr << "No handler for status: " << response.status() << '\n';
			}
		}
	}

private:
	//////////////// handlers ///////////////////

	void do_register(sip::request & request) {
		if (request.headers().count("To")) {
			dbase[request.headers()["To"]] = request.remote();

			auto addr = asio::ip::address::from_string("127.0.0.1");
			dbase["rtp:"+request.headers()["To"]] = asio::ip::udp::endpoint(addr, std::stol(request.headers()["MYRTP"]));

			std::cout << "registered user sip" << request.headers()["To"] << " (" 
			<< request.remote() << ")\n";

			std::cout << "registered user rtp" << request.headers()["To"] << " (" 
			<< dbase["rtp:"+request.headers()["To"]] << ")\n";
		}
	}

	void tracked_invite(sip::request & request) {
		if (dbase.count(request.headers()["To"]) && dbase.count("rtp:"+request.headers()["To"]) && dbase.count("rtp:"+request.headers()["From"])) {
			add_session_sip(request.remote(), dbase[request.headers()["To"]]);
			add_session_rtp(dbase["rtp:"+request.headers()["To"]], dbase["rtp:"+request.headers()["From"]]);

			std::cout << "tracked invitation => redirecting from " << request.headers()["From"]  << "("
			<< request.remote() << ") to " << request.headers()["To"] << "(" << dbase[request.headers()["To"]] << ")\n";

			request.set_remote(dbase[request.headers()["To"]]);
			request.add_header("SERVERRTPPORT", std::to_string(local.port()));

			auto wrapper = sip::message_wrapper{std::make_unique<sip::request>(std::move(request))};
			this->async_send(wrapper);

		}
	}

	void tracked_ack(sip::request & request) {
		if (dbase.count(request.headers()["From"])) {
			if (!has_session_sip(request.remote())) return;

			auto peer = get_partner_sip(request.remote());
			if (!peer.has_value()) return;

			for (auto &s : sessions) {
				if (s.is_participant(*peer)) {
					s.set_status(session_status::happening);
				}
			}

			request.set_remote(*peer);

			auto wrapper = sip::message_wrapper{std::make_unique<sip::request>(std::move(request))};
			this->async_send(wrapper);


		}
	}

	void tracked_bye(sip::request & request) {
		if (dbase.count(request.headers()["From"])) {
			if (!has_session_sip(request.remote())) return;

			auto peer = get_partner_sip(request.remote());
			if (!peer.has_value()) return;

			request.set_remote(*peer);

			auto wrapper = sip::message_wrapper{std::make_unique<sip::request>(std::move(request))};
			this->async_send(wrapper);
 		}
	}

	//////////////////////////////////////////////////
	void tracked_trying(sip::response & response) {
		if (dbase.count(response.headers()["From"])) {
			if (!has_session_sip(response.remote())) return;

			auto peer = get_partner_sip(response.remote());
			if (!peer.has_value()) return;

			response.set_remote(*peer);

			auto wrapper = sip::message_wrapper{std::make_unique<sip::response>(std::move(response))};
			this->async_send(wrapper);
 		}
	}

	void tracked_ringing(sip::response & response) {
		if (dbase.count(response.headers()["From"])) {
			if (!has_session_sip(response.remote())) return;

			auto peer = get_partner_sip(response.remote());
			if (!peer.has_value()) return;

			response.set_remote(*peer);

			auto wrapper = sip::message_wrapper{std::make_unique<sip::response>(std::move(response))};
			this->async_send(wrapper);
 		}
	}

	void tracked_ok(sip::response & response) {
		if (dbase.count(response.headers()["From"])) {
			if (!has_session_sip(response.remote())) return;

			if (response.headers()["CSeq"].find("BYE") != std::string::npos) {
				auto peer = get_partner_sip(response.remote());
				if (!peer.has_value()) return;

				response.set_remote(*peer);

				auto wrapper = sip::message_wrapper{std::make_unique<sip::response>(std::move(response))};
				this->async_send(wrapper);
			}

		}
	}
};
