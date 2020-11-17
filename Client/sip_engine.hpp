#ifndef SIP_ENGINE_HPP
#define SIP_ENGINE_HPP

#include <QDebug>
#include <QObject>
#include <QByteArray>

#include "Network/sip/sip.hpp"
using namespace net;
namespace net {
    namespace sip {
        enum class role {
            empty,
            caller,
            callee
        };

        enum class status {
            /* Caller zone */
            InviteSent,
            TryingReceived,
            RingingReceived,
            RingingOKReceived,



            /* Common zone */
            Idle,
            ByeSent,
            ByeReceived,
            ByeOKSent,
            ByeOKReceived,
            InAction, // == AckSent == AckReceived

            /* Callee zone */

            InviteReceived,
            InviteTryingSent,
            InviteRingingSent,
            InviteOKSent,


        };

        struct client_session {
            std::string me;
            std::string other;

            asio::ip::udp::endpoint through;
            // sdp ?
        };
    }
}

enum class ok_reason {
    onBye,
    onInvite
};

class sip_engine : public  QObject, public sip::client {
    Q_OBJECT
public:
    sip_engine(asio::io_context & ioc, std::string address, std::uint16_t port)
        : sip::client(ioc, sock),
          ios(ioc),
          remote(asio::ip::address::from_string(address), port),
          sock(ioc)

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

    void set_begin_handler(std::function<void(sip::client_session)> handler) {
        // call 'handler' when sip-session is established

    }

    void set_end_handler(std::function<void(sip::client_session)> handler) {
        // call 'handler' when sip-session was finished
    }

    void do_register(std::string me) {
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

    void invite(std::string who) {
        if (role == sip::role::empty && status == sip::status::Idle) {
            qDebug() << sock.local_endpoint().address().to_string().c_str() << " " <<sock.local_endpoint().port() << '\n';
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

    void ack() {
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

            begin_handler(session);
        }
        else {
            qDebug() << "Wrong role/status to send ACK\n";
        }
    }

    void ok(ok_reason reason) {
        if (role != sip::role::empty && status != sip::status::Idle) {
            if (reason == ok_reason::onInvite) {
                sip::request request;

            }
            else if (reason == ok_reason::onBye){

            }

            // async_send here
        }
        else {
            qDebug() << "Wrong role/status to send OK\n";
        }
    }

    void bye() {
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



    void on_message(sip::message & message) {
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
private:
    void on_trying(sip::response & response) {
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

    void on_ringing(sip::response & response) {
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

    void on_ok(sip::response & response) {
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

    void on_invite(sip::request & request) {
        if (role == sip::role::empty && status == sip::status::Idle) {
            role = sip::role::callee;
            status = sip::status::InviteReceived;

            // send 100 Trying

            qDebug() << "received invite from " << request.headers()["From"].c_str() << "\n";
            emit incoming_call(request.headers()["From"]);
            // send 180 Ringing and emit signal to notify MainWindow

        }
        else {
            qDebug() << "Wrong role/status to receive INVITE\n";
        }
    }

    void on_ack(sip::request & request) {
        // emit on_media_start() => where we will make_unique<rtp_io>(request.headers()[RTPPORT])
        begin_handler(session);
    }

    void on_bye(sip::request & request) {
        if (role != sip::role::empty && status == sip::status::InAction) {
            this->ok(ok_reason::onBye);
            end_handler(session);
            emit finishing_call();
        }
        else {
            qDebug() << "Wrong role/status to receive BYE\n";
        }
    }

public slots:
    // just send OK to caller
    // next we wait for ACK
    void incoming_call_accepted() {
        this->ok(ok_reason::onInvite);
    }

    // send BYE to partner
    void on_call_finished() {
        this->bye();
    }



signals:
    // display incoming call on form
    void incoming_call(std::string who);

    void finishing_call();

private:
    asio::io_context & ios;

    asio::ip::udp::endpoint remote;
    asio::ip::udp::socket sock;

    sip::status status;
    sip::role role;

    sip::client_session session;
    std::atomic<uint16_t> cseq{0};

    std::function<void(sip::client_session)> begin_handler;
    std::function<void(sip::client_session)> end_handler;

    std::map<std::string, void((sip_engine::*)(sip::response&))> response_map;
    std::map<std::string, void((sip_engine::*)(sip::request&))> request_map;
};


#endif // SIP_ENGINE_HPP
