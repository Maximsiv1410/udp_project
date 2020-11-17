#ifndef DATA_SENDER_HPP
#define DATA_SENDER_HPP

#include <QtCore>
#include <QByteArray>

#include "Network/rtp/rtp.hpp"
using namespace net;

#define MAX_PART_SIZE 1300

struct frame_data {
    std::shared_ptr<QByteArray> image;

    std::uint16_t parts;
    std::uint16_t overage;
    std::uint32_t id;

    frame_data() = default;

    frame_data(std::shared_ptr<QByteArray> & ptrImg, std::uint32_t ID)
        : image{ptrImg},
          id{ID}
    {
        overage = image->size() % MAX_PART_SIZE;
        parts = (image->size() - overage) + (overage > 0);
    }
};



class rtp_io : public QObject, public realtime::client  {
    Q_OBJECT

public:
        rtp_io(asio::io_context & ios, std::string addr, std::uint16_t port)
        : realtime::client(ios, sock),
          ios(ios),
          remote(asio::ip::address::from_string(addr), port),
          sock(ios)

    {
        sock.open(asio::ip::udp::v4());
        sock.connect(remote);
        this->set_callback([this](realtime::rtp_packet && pack)
        {
            received_packet(pack);
        });
    }

    void cache_frame(std::shared_ptr<QByteArray> & img, std::uint32_t frame_id) {
        frames_out.push(frame_data{img, frame_id});
        send_frame();
    }


private:
    void received_packet(realtime::rtp_packet & pack) {
        realtime::video_header header;

        if (pack.payload().size() >= sizeof(header)) {
            std::memcpy(&header, pack.payload().data(), sizeof(header));
            if (header.frame_no == frame_in_counter) {
                std::memcpy(curr_frame->data(), pack.payload().data() + sizeof(header), header.part_size);
                frame_offset += header.part_size;
                if (frame_offset == header.full_size) {
                    frame_in_counter++;
                    // data race condition may exist
                    emit frame_gathered(std::move(curr_frame));
                    curr_frame.reset();
                }
            }
            else if (header.frame_no == frame_in_counter + 1) {
                // just next frame
                curr_frame->resize(header.full_size);
                frame_offset = 0;
            }
            else {
                // reordering happened
            }
        }
        else {
            // corrupted
        }
    }



    void send_frame() {
        if (frames_out.empty()) return;

        asio::post(ios, [this]
        {
            frame_data fd;
            bool attempt = frames_out.try_pop(fd);
            if (!attempt) return;

            std::size_t offset = 0;
            for (std::uint16_t i = 0; i < fd.parts; i++) {
                realtime::rtp_packet packet;

                realtime::video_header videoHeader;
                videoHeader.frame_no = fd.id;
                videoHeader.parts = fd.parts;
                videoHeader.part_no = i;
                videoHeader.full_size = fd.image->size();
                if (i == fd.parts - 1) {
                    if (fd.overage) {
                        videoHeader.part_size = fd.overage;
                    }
                    else {
                        videoHeader.part_size = MAX_PART_SIZE;
                    }
                }

                packet.payload().resize(sizeof(videoHeader) + videoHeader.part_size);

                std::memcpy(packet.payload().data(), fd.image->data() + offset, sizeof(videoHeader));
                offset += sizeof(videoHeader);

                std::memcpy(packet.payload().data(), fd.image->data() + offset, videoHeader.part_size);
                offset += videoHeader.part_size;

                enqueue(packet);
            }
            init_send();
        });

    }

signals:
    void frame_gathered(std::shared_ptr<std::vector<char>> frame);

private:
    /* temporary data for gathering frame */
    std::shared_ptr<std::vector<char>> curr_frame;
    std::atomic<std::uint16_t> frame_in_counter{0};
    std::size_t frame_offset{0};
    std::size_t frame_parts{0};

    asio::io_context & ios;
    asio::ip::udp::endpoint remote;
    asio::ip::udp::socket sock;

    tsqueue<frame_data> frames_out;
    tsqueue<frame_data> frames_in;
};

#endif // DATA_SENDER_HPP
