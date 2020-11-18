#ifndef DATA_SENDER_HPP
#define DATA_SENDER_HPP

#include <QtCore>
#include <QByteArray>
#include <QImage>
#include <QImageReader>
#include <QBuffer>

#include "Network/rtp/rtp.hpp"
#include "opencv2/highgui.hpp"

using namespace net;

#define MAX_PART_SIZE 1300

struct frame_data {
   cv::Mat frame;
   std::size_t id = 0;
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

    void cache_frame(cv::Mat & img, std::size_t frame_id) {
        frames_out.push(frame_data{img, frame_id});
        send_frame();
    }

    asio::ip::udp::endpoint local() {
        return sock.local_endpoint();
    }

private:
    void received_packet(realtime::rtp_packet & pack) {
        realtime::video_header header;

        if (pack.payload().size() >= sizeof(header)) {
            std::memcpy(&header, pack.payload().data(), sizeof(header));
            if (header.frame_no == frame_in_counter) {
                std::memcpy(curr_frame.data(), pack.payload().data() + sizeof(header), header.part_size);
                frame_offset += header.part_size;

                if (frame_offset == header.full_size) {
                    frame_in_counter++;
                    // data race condition may exist
                    QByteArray bytes = QByteArray::fromRawData((const char*)(curr_frame.data()), curr_frame.size());
                    QBuffer buffer(&bytes);
                    QImageReader reader(&buffer);
                    QImage img = reader.read();

                    emit frame_gathered(img);
                    curr_frame.clear();
                }
            }
            else if (header.frame_no == frame_in_counter + 1) {
                // just next frame
                curr_frame.resize(header.full_size);
                frame_offset = 0;
                frame_in_counter++;
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

            std::vector<uchar> image;

            std::vector<int> quality_params(2);
            quality_params[0] = CV_IMWRITE_JPEG_QUALITY; // Кодек JPEG
            quality_params[1] = 20;
            cv::imencode(".jpg", fd.frame, image, quality_params);

            std::size_t img_overage = image.size() % MAX_PART_SIZE;
            std::size_t img_parts = ((image.size() - img_overage) / MAX_PART_SIZE) + (img_overage > 0);

            std::size_t img_offset = 0;
            for (std::uint16_t i = 0; i < img_parts; i++) {


                realtime::rtp_packet packet;

                realtime::video_header videoHeader;
                videoHeader.frame_no = fd.id;
                videoHeader.parts = img_parts;
                videoHeader.part_no = i;
                videoHeader.full_size = image.size();

                if (i == img_parts - 1 && img_overage) {
                    videoHeader.part_size = img_overage;
                } else videoHeader.part_size = MAX_PART_SIZE;

                packet.payload().resize(sizeof(videoHeader) + videoHeader.part_size);

                std::memcpy(packet.payload().data(), (char*)&videoHeader, sizeof(videoHeader));
                std::memcpy(packet.payload().data() + sizeof(videoHeader), image.data(), videoHeader.part_size);
                img_offset += videoHeader.part_size;

                packet.set_remote(this->sock.remote_endpoint());
                enqueue(packet);
            }
            init_send();
        });

    }



signals:
    void frame_gathered(QImage frame);

private:
    /* temporary data for gathering frame */
    std::vector<uchar> curr_frame;
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
