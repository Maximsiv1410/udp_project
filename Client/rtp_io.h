#ifndef RTP_IO_H
#define RTP_IO_H

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
   cv::Mat frame; // or ptr<cv::Mat> to avoid copy
   std::uint32_t id = 0;

   frame_data(const cv::Mat & mat, std::uint32_t id)
       : frame(mat)
       , id(id)
   {}
};


class rtp_io : public QObject, public realtime::client  {
    Q_OBJECT
public:
    rtp_io(asio::io_context & ioc, std::string addr, std::uint16_t port);

public slots:
    void cache_frame(cv::Mat img, std::size_t frame_id);

    asio::ip::udp::endpoint local();

private:
    void received_packet(realtime::rtp_packet & pack);

    void send_frame(std::shared_ptr<frame_data> fd);

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
    std::unique_ptr<asio::io_context::strand> frame_strand;

    tsqueue<frame_data> frames_out;
    tsqueue<frame_data> frames_in;
};

#endif // RTP_IO_H
