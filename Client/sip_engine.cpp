#include "sip_engine.h"

    sip_engine::sip_engine(asio::io_context & ioc, std::string address, std::uint16_t port)
        : sip::client(ioc, sock)
        , ios(ioc)
        , remote(asio::ip::address::from_string(address), port)
        , sock(ioc)
    {
        sock.open(asio::ip::udp::v4());
        sock.connect(remote);

        session.through = sock.remote_endpoint();

        role = sip::role::empty;
        status = sip::status::Idle;

        request_map["INVITE"] = &sip_engine::on_invite;
        request_map["ACK"] = &sip_engine::on_ack;
        request_map["BYE"] = &sip_engine::on_bye;

        response_map["OK"] = &sip_engine::on_ok;
        response_map["Ringing"] = &sip_engine::on_ringing;
        response_map["Trying"] = &sip_engine::on_trying;

        this->set_callback([this](sip::message_wrapper && wrapper)
        {
            this->on_message(*wrapper.storage());
        });
    }



    void sip_engine::do_register(std::string me) {
        if (role == sip::role::empty && status == sip::status::Idle) {
            session.me = me;

            sip::request request;
            request.set_method("REGISTER");
            request.set_uri("sip:server");
            request.set_version("SIP/2.0");
            request.add_header("To", me);
            request.add_header("From", me);
            request.set_remote(sock.remote_endpoint());

            auto wrapper = sip::message_wrapper{std::make_unique<sip::request>(std::move(request))};
            this->async_send(wrapper);

            qDebug() << "registered\n";
        }
        else {

        }
    }

    void sip_engine::invite(std::string who) {
        if (role == sip::role::empty && status == sip::status::Idle) {
            session.other = who;
            sip::request request;
            request.set_method("INVITE");
            request.set_uri(who);
            request.set_version("SIP/2.0");
            request.add_header("CSeq", std::to_string(cseq) + " INVITE");
            request.add_header("To", who);
            request.add_header("From", session.me);
            request.set_remote(sock.remote_endpoint());

            auto wrapper = sip::message_wrapper{std::make_unique<sip::request>(std::move(request))};
            this->async_send(wrapper);

            role = sip::role::caller;
            status = sip::status::InviteSent;
        }
        else {
           qDebug() << "Wrong role/status to send INVITE\n";
        }
    }

    void sip_engine::ack() {
       if (role == sip::role::caller && status == sip::status::RingingOKReceived) {
            sip::request request;
            request.set_method("ACK");
            request.set_uri(session.other);
            request.set_version("SIP/2.0");
            request.add_header("To", session.other);
            request.add_header("From", session.me);
            request.set_remote(session.through);

            auto wrapper = sip::message_wrapper{std::make_unique<sip::request>(std::move(request))};
            this->async_send(wrapper);

            status = sip::status::InAction;

        }
        else {
            qDebug() << "Wrong role/status to send ACK\n";
        }
    }

    void sip_engine::ok(ok_reason reason) {
        if (role != sip::role::empty && status != sip::status::Idle) {
            sip::response response;
            response.set_status("OK");
            response.set_code(200);
            response.set_version("SIP/2.0");
            response.add_header("To", session.other);
            response.add_header("From", session.me);
            response.set_remote(session.through);

            if (reason == ok_reason::onInvite) {
                response.add_header("CSeq", std::to_string(cseq++) + " INVITE");
            }
            else if (reason == ok_reason::onBye){
                response.add_header("CSeq", std::to_string(cseq++) + " BYE");
            }
            auto wrapper = sip::message_wrapper{std::make_unique<sip::response>(std::move(response))};
            this->async_send(wrapper);

            role = sip::role::empty;
            status = sip::status::Idle;
        }
        else {
            qDebug() << "Wrong role/status to send OK\n";
        }
    }

    void sip_engine::bye() {
        if (role != sip::role::empty && status == sip::status::InAction) {
            sip::request request;
            request.set_method("BYE");
            request.set_uri(session.other);
            request.set_version("SIP/2.0");
            request.add_header("To", session.other);
            request.add_header("From", session.me);
            request.set_remote(session.through);

            auto wrapper = sip::message_wrapper{std::make_unique<sip::request>(std::move(request))};
            this->async_send(wrapper);

            status = sip::status::ByeSent;
        }
    }



    void sip_engine::on_message(sip::message & message) {
        if (message.type() == sip::sip_type::Request) {
            sip::request & request = (sip::request&)message;
            if (request_map.count(request.method())) {
                (this->*request_map[request.method()])(request);
            }
            else {
                qDebug() << "No handler for method: " << request.method().c_str() << "\n";
            }
        }
        else if(message.type() == sip::sip_type::Response) {
            sip::response & response = (sip::response&)message;
            if (response_map.count(response.status())) {
                (this->*response_map[response.status()])(response);
            }
            else {
                 qDebug() << "No handler for status: " << response.status().c_str() << "\n";
            }
        }
    }


     /* ////////////////////////////////////////////////////// */

    void sip_engine::on_trying(sip::response & response) {
        if(response.code() != 100) return;

        if (role == sip::role::caller && status == sip::status::InviteSent) {
            if (response.headers()["CSeq"] == std::to_string(cseq) + " INVITE") {
                status = sip::status::TryingReceived;
            }
            else {
                qDebug() << "CSeq corrupted\n";
            }
        }
        else {
            qDebug() << "Wrong role/status to receive Trying\n";
        }
    }

    void sip_engine::on_ringing(sip::response & response) {
        if (response.code() != 180) return;

        if (role == sip::role::caller && status == sip::status::TryingReceived) {
            if (response.headers()["CSeq"] == std::to_string(cseq) + " INVITE") {
                status = sip::status::RingingReceived;
            }
            else {
                qDebug() << "CSeq corrupted\n";
            }
        }
        else {
            qDebug() << "Wrong role/status to receive Ringing\n";
        }
    }

    void sip_engine::on_ok(sip::response & response) {
        if (response.code() != 200) return;

        if (role == sip::role::caller && status == sip::status::RingingReceived) {
            if (response.headers()["CSeq"] == std::to_string(cseq) + " INVITE") {
                status = sip::status::RingingOKReceived;
                cseq++;

                this->ack();
            }
            else {
                qDebug() << "CSeq corrupted\n";
            }
        }
       else if(role != sip::role::empty && status == sip::status::ByeSent) {
            if (response.headers()["CSeq"] == std::to_string(cseq) + " BYE") {
                status = sip::status::ByeReceived;
                // end of conversation
            }
            else {
                qDebug() << "CSeq corrupted\n";
            }
       }
        else {
            qDebug() << "Wrong role/status to receive OK\n";
        }
    }

    void sip_engine::on_invite(sip::request & request) {
        if (role == sip::role::empty && status == sip::status::Idle) {
            role = sip::role::callee;
            status = sip::status::InviteReceived;

            sip::response try_response;
            try_response.set_code(100);
            try_response.set_version("SIP/2.0");
            try_response.set_status("Trying");
            try_response.add_header("To", session.other);
            try_response.add_header("From", session.me);
            try_response.add_header("CSeq", request.headers()["CSeq"]); // or to_string(cseq) + "INVITE"?
            try_response.set_remote(session.through);

            auto try_wrapper = sip::message_wrapper{std::make_unique<sip::response>(std::move(try_response))};
            this->async_send(try_wrapper);

            status = sip::status::InviteTryingSent;

            qDebug() << "received invite from " << request.headers()["From"].c_str() << "\n";

            sip::response ring_response;
            ring_response.set_code(180);
            ring_response.set_version("SIP/2.0");
            ring_response.set_status("Ringing");
            ring_response.add_header("To", session.other);
            ring_response.add_header("From", session.me);
            ring_response.add_header("CSeq", request.headers()["CSeq"]); // or to_string(cseq) + "INVITE"?
            ring_response.set_remote(session.through);

            auto ring_wrapper = sip::message_wrapper{std::make_unique<sip::response>(std::move(ring_response))};
            this->async_send(ring_wrapper);

            status = sip::status::InviteRingingSent;

            emit incoming_call();

        }
        else {
            qDebug() << "Wrong role/status to receive INVITE\n";
        }
    }

    void sip_engine::on_ack(sip::request & request) {
        if (role == sip::role::callee && status == sip::status::InviteOKSent) {
            // emit on_media_start() => where we will make_unique<rtp_io>(request.headers()[RTPPORT])
        }
        else {
            qDebug() << "Wrong role/status to receive ACK\n";
        }
    }

    void sip_engine::on_bye(sip::request & request) {
        if (role != sip::role::empty && status == sip::status::InAction) {
            this->ok(ok_reason::onBye);
            emit finishing_call();
        }
        else {
            qDebug() << "Wrong role/status to receive BYE\n";
        }
    }


    // just send OK to caller
    // next we wait for ACK
    void sip_engine::incoming_call_accepted() {
        this->ok(ok_reason::onInvite);
    }

    // send BYE to partner
    void sip_engine::on_call_finished() {
        this->bye();
    }


