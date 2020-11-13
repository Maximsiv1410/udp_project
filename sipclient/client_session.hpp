#pragma once
#include <boost/asio.hpp>

#include "client.hpp"

enum class role {
	idle,
	caller,
	callee
};

enum class stage {
	None,

	InviteSent,
	InviteOK,

	Talking,

	ByeSent,
	ByeOk,

	ByeGot
};


class client_session {
	role role_ { role::idle };
	stage stage_ { stage::None };

	sip_client sip_connection;

	std::string me;


public:

	client_session(std::string my_name) 
	: 
	sip_connection("127.0.0.1", 1, 5060),
	me(std::move(my_name))
	{
		sip_connection.start(true);
		sip_connection.set_callback(
		[this](sip::message_wrapper && wrapper)
		{
			this->on_message(wrapper.storage().get());
		});
	}
/////// only caller
	void do_register() {
		if (role_ == role::idle) {
			sip::request request;
			request.set_method("REGISTER");
			request.set_remote(sip_connection.remote());
			request.set_uri("sip:localhost.my");
			request.set_version("SIP/2.0");
			request.add_header("To", me.c_str());
			request.add_header("From", me.c_str());
			request.add_header("CSeq", "REGISTER");
			request.add_header("Content-Length", "0");

			sip::message_wrapper wrapper {std::make_unique<sip::request>(std::move(request))};
			std::cout << "sending message with size: " << wrapper.storage()->total() << "\n";
			sip_connection.async_send(wrapper);
		}
	}

	bool invite(std::string who) {
		if (role_ == role::idle) {
			role_ = role::caller;
			stage_ = stage::InviteSent;

			sip::request request;
			request.set_method("INVITE");
			request.set_remote(sip_connection.remote());
			request.set_uri(who);
			request.set_version("SIP/2.0");
			request.add_header("To", who.c_str());
			request.add_header("From", me.c_str());
			request.add_header("CSeq", "INVITE");
			request.add_header("Content-Length", "0");

			sip::message_wrapper wrapper {std::make_unique<sip::request>(std::move(request))};
			sip_connection.async_send(wrapper);

			return true;
		}
		return false;
	}
/////// only caller


	void bye() {
		
	}


	void incoming_ok(sip::response & response) {
		std::cout << "got ok\n";
		if (stage_ == stage::InviteSent) {
			if (response.headers().count("CSeq")) {
				if (response.headers()["CSeq"] == "INVITE") {
					stage_ == stage::InviteOK;

				}
			}
		}
	}

	void incoming_bye(sip::request & request) {
		if (stage_ == stage::Talking) {
			if (request.headers().count("CSeq")) {
				if (request.headers()["CSeq"] == "BYE") {
					stage_ == stage::None;

					sip::response response;
					response.set_code(200);
					response.set_status("OK");
					response.set_remote(sip_connection.remote());
					response.set_version("SIP/2.0");
					response.add_header("To", request.headers()["From"].c_str());
					response.add_header("From", me.c_str());
					response.add_header("CSeq", "BYE");
					response.add_header("Content-Length", "0");

					sip::message_wrapper wrapper {std::make_unique<sip::response>(std::move(response))};
					sip_connection.async_send(wrapper);

				}
			}
		}
	}

	void incoming_invite(sip::request & request) {
		std::cout << "got invitation from " << request.headers()["From"] << '\n';
		if (role_ == role::idle) {
			role_ = role::callee;

			sip::response response;
			response.set_code(200);
			response.set_status("OK");
			response.set_remote(sip_connection.remote());
			response.set_version("SIP/2.0");
			response.add_header("To", request.headers()["From"].c_str());
			response.add_header("From", me.c_str());
			response.add_header("CSeq", "INVITE");
			response.add_header("Content-Length", "0");

			sip::message_wrapper wrapper {std::make_unique<sip::response>(std::move(response))};
			sip_connection.async_send(wrapper);
		}
	}



	void on_message(sip::message * msg) {
		if (msg->type() == sip::sip_type::Response) {
			auto response = (sip::response*)msg;

			std::cout << response->code() << '\n';
			if (response->code() == 200) {
				incoming_ok(*response);
			}
			else {
				std::cout << "unrecognized response\n";
			}
		}
		else if(msg->type() == sip::sip_type::Request) {
			auto request = (sip::request*)msg;

			if (request->method() == "INVITE") {
				incoming_invite(*request);
			}
			else if(request->method() == "BYE") {
				incoming_bye(*request);
			}
		}
	}
};
