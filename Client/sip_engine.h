#ifndef SIP_ENGINE_H
#define SIP_ENGINE_H

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
    sip_engine(asio::io_context & ioc, std::string address, std::uint16_t port, std::uint16_t rtpport);

    void do_register(std::string me);

    void invite(std::string who);

    void ack();

    void ok(ok_reason reason);

    void bye();

    void on_message(sip::message & message);


private:
    void on_trying(sip::response & response);

    void on_ringing(sip::response & response);

    void on_ok(sip::response & response);

    void on_invite(sip::request & request);

    void on_ack(sip::request & request);
    void on_bye(sip::request & request);
public slots:

    void incoming_call_accepted();

    void on_call_finished();

signals:
    // display incoming call on form
    void incoming_call();

    void finishing_call();

private:
    asio::io_context & ios;

    asio::ip::udp::endpoint remote;
    asio::ip::udp::socket sock;

    sip::status status;
    sip::role role;

    sip::client_session session;
    std::atomic<uint16_t> cseq{0};

    std::uint16_t rtpport;


    std::map<std::string, void((sip_engine::*)(sip::response&))> response_map;
    std::map<std::string, void((sip_engine::*)(sip::request&))> request_map;
};

#endif // SIP_ENGINE_H
