#ifndef DATA_SENDER_HPP
#define DATA_SENDER_HPP

#include <QtCore>
#include <QByteArray>
#include <QImage>
#include <QImageReader>
#include <QBuffer>
#include <QPixmap>
#include <fstream>

#include "Network/rtp/rtp.hpp"
#include "opencv2/highgui.hpp"

using namespace net;

#define MAX_PART_SIZE 1300

struct frame_data {
   cv::Mat frame;
   std::uint32_t id = 0;

   frame_data(const cv::Mat & mat, std::uint32_t id)
       : frame(mat)
       , id(id)
   {}
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

public slots:
    void cache_frame(cv::Mat img, std::size_t frame_id) {
        auto frame = std::make_shared<frame_data>(img, frame_id);
        asio::post(ios, [this, frame]{ send_frame(frame); });
    }

    asio::ip::udp::endpoint local() {
        return sock.local_endpoint();
    }

private:
    void received_packet(realtime::rtp_packet & pack) {
        realtime::video_header header;

        if (pack.payload().size() < sizeof(header)) {
            frame_offset = 0;
            curr_part = 1;
            qDebug() << "CORRUPTION\n";
            return;
        }

        std::memcpy(&header, pack.payload().data(), sizeof(header));

        if (header.frame_no == frame_in_counter && header.part_no == curr_part) {
            if (!frame_offset) {
                //qDebug() << "[new frame]\n";
                frame_offset = 0;
                curr_part = 1;
                curr_frame.resize(header.full_size);
            }

            //qDebug() << "frame_no " << header.frame_no << ", part_no " << header.part_no << '\n';
            std::memcpy(curr_frame.data() + frame_offset, pack.payload().data() + sizeof(header), header.part_size);
            frame_offset += header.part_size;
            curr_part++;

            if (frame_offset == header.full_size && header.parts == curr_part - 1) {
              /* qDebug() << "frame_no: " << header.frame_no << " ==? "<< frame_in_counter << " completed, full size: "
                        << header.full_size << " vec size: "
                        << frame_offset << '\n'; */

               frame_in_counter += 1;

               //cv::Mat img = cv::imdecode(curr_frame, 1);
               //cv::imwrite("C:\\Users\\Maxim\\Desktop\\income.jpg", img);

               /*/////////////////////////// */
               QByteArray bytes = QByteArray::fromRawData((const char*)(curr_frame.data()), curr_frame.size());
               QBuffer buffer(&bytes);
               QImageReader reader(&buffer);
               QImage img = reader.read();
               emit frame_gathered(QPixmap::fromImage(img));
               /*/////////////////////////// */

               frame_offset = 0;
               curr_part = 1;
               curr_frame.clear();
            }


        }

        else {
            //qDebug() << "reordering happened " << header.frame_no << ", part_no: " << header.part_no << ", going to next frame\n";
            frame_in_counter = header.frame_no;
            frame_offset = 0;
            curr_part = 1;
            curr_frame.resize(header.full_size);

            std::memcpy(curr_frame.data() + frame_offset, pack.payload().data() + sizeof(header), header.part_size);
            frame_offset += header.part_size;
            curr_part++;
        }
    }



    void send_frame(std::shared_ptr<frame_data> fd) {
        //if (frames_out.empty()) return;

        //frame_data fd;
        //bool attempt = frames_out.try_pop(fd);
        //if (!attempt) return;

        std::vector<uchar> image;

        std::vector<int> quality_params(2);
        quality_params[0] = CV_IMWRITE_JPEG_QUALITY; // Кодек JPEG
        quality_params[1] = 50;
        cv::imencode(".jpg", fd->frame, image, quality_params);

        std::size_t img_overage = image.size() % MAX_PART_SIZE;
        std::size_t img_parts = ((image.size() - img_overage) / MAX_PART_SIZE) + (img_overage > 0);

        std::size_t img_offset = 0;
        for (std::uint16_t i = 1; i <= img_parts; i++) {

            realtime::rtp_packet packet;
            packet.header().csrc_count(0);

            realtime::video_header videoHeader;
            videoHeader.frame_no = fd->id;
            videoHeader.parts = img_parts;
            videoHeader.part_no = i;
            videoHeader.full_size = image.size();

            if (i == img_parts && img_overage) {
                videoHeader.part_size = img_overage;
            } else videoHeader.part_size = MAX_PART_SIZE;

            packet.payload().resize(sizeof(videoHeader) + videoHeader.part_size);

            std::memcpy(packet.payload().data(), (char*)&videoHeader, sizeof(videoHeader));
            std::memcpy(packet.payload().data() + sizeof(videoHeader), image.data() + img_offset, videoHeader.part_size);
            img_offset += videoHeader.part_size;

            packet.set_remote(this->sock.remote_endpoint());
            enqueue(packet);
        }
        //qDebug() << "sending frame with id " << fd->id << " sizeof " << image.size() << '\n';
        init_send();
    }


signals:
    void frame_gathered(QPixmap frame);

private:
    /* temporary data for gathering frame */
    std::vector<uchar> curr_frame;
    std::uint16_t frame_in_counter{0};
    std::size_t frame_offset{0};
    std::size_t curr_part{1};
    /* temporary data for gathering frame */

    asio::io_context & ios;
    asio::ip::udp::endpoint remote;
    asio::ip::udp::socket sock;

    tsqueue<frame_data> frames_out;
    tsqueue<frame_data> frames_in;
};

#endif // DATA_SENDER_HPP
