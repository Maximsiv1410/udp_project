#include "rtp_io.h"

rtp_io::rtp_io(asio::io_context & ios, std::string addr, std::uint16_t port)
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

void rtp_io::cache_frame(cv::Mat img, std::size_t frame_id) {
    auto frame = std::make_shared<frame_data>(img, frame_id);

    // probably we should have strand here
    // because we want to be guaranteed
    // to enqueue each 'frame' consistently
    asio::post(ios, [this, frame]{ send_frame(frame); });
}

asio::ip::udp::endpoint rtp_io::local() {
    return sock.local_endpoint();
}


void rtp_io::received_packet(realtime::rtp_packet & pack) {
    realtime::video_header header;

    if (pack.payload().size() < sizeof(header)) {
        frame_offset = 0;
        curr_part = 1;
        qDebug() << "CORRUPTION\n";
        return;
    }

    std::memcpy(&header, pack.payload().data(), sizeof(header));

    // just ordinary consistently gathering frame
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
    }
    // oops, reordering, just skip current frame and start gathering next
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

    // shoud it be here or in the body of 'if (header.frame_no == frame_in_counter &&...' ?
    if (frame_offset == header.full_size && header.parts == curr_part - 1) {
      /* qDebug() << "frame_no: " << header.frame_no << " ==? "<< frame_in_counter << " completed, full size: "
                << header.full_size << " vec size: "
                << frame_offset << '\n'; */

       frame_in_counter += 1;

       QByteArray bytes = QByteArray::fromRawData(reinterpret_cast<const char*>(curr_frame.data()), curr_frame.size());
       QBuffer buffer(&bytes);
       QImageReader reader(&buffer);
       QImage img = reader.read();

       emit frame_gathered(QPixmap::fromImage(img));

       frame_offset = 0;
       curr_part = 1;
       curr_frame.clear();
    }
}



void rtp_io::send_frame(std::shared_ptr<frame_data> fd) {
    std::vector<uchar> image;

    std::vector<int> quality_params(2);
    quality_params[0] = CV_IMWRITE_JPEG_QUALITY;
    quality_params[1] = 40;
    cv::imencode(".jpg", fd->frame, image, quality_params);

    std::size_t img_overage = image.size() % MAX_PART_SIZE;
    std::size_t img_parts = ((image.size() - img_overage) / MAX_PART_SIZE) + (img_overage > 0);

    std::size_t img_offset = 0;
    for (std::uint16_t i = 1; i <= img_parts; i++) {

        realtime::rtp_packet packet;
        packet.header().csrc_count(0); // to avoid csrc id's parsing

        realtime::video_header videoHeader;
        videoHeader.frame_no = fd->id;
        videoHeader.parts = img_parts;
        videoHeader.part_no = i;
        videoHeader.full_size = image.size();

        if (i == img_parts && img_overage) {
            videoHeader.part_size = img_overage;
        } else videoHeader.part_size = MAX_PART_SIZE;

        packet.payload().resize(sizeof(videoHeader) + videoHeader.part_size);

        std::memcpy(packet.payload().data(), reinterpret_cast<char*>(&videoHeader), sizeof(videoHeader));
        std::memcpy(packet.payload().data() + sizeof(videoHeader), image.data() + img_offset, videoHeader.part_size);
        img_offset += videoHeader.part_size;

        packet.set_remote(this->sock.remote_endpoint());
        enqueue(packet);
    }
    //qDebug() << "sending frame with id " << fd->id << " sizeof " << image.size() << '\n';
    init_send();
}


