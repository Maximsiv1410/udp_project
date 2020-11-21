#include "video_worker.h"
#include <QMetaType>

video_worker::video_worker()
{
    qRegisterMetaType<cv::Mat>();
    qRegisterMetaType<std::uint32_t>("std::uint32_t");
}

void video_worker::start_stream() {
    if (capture.isOpened()) return;

    capture.open(0);
    if (!capture.isOpened()) return;

    capture.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M', 'J', 'P', 'G'));
    capture.set(CV_CAP_PROP_FRAME_WIDTH, 640);
    capture.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

    std::uint32_t id = 0;
    while(capture.isOpened()) {
        cv::Mat frame;
        capture >> frame;

        if (!frame.empty()) {
            QImage img(frame.data,
                       frame.cols,
                       frame.rows,
                       frame.step,
                       QImage::Format_BGR888); // RGB888 gives blue shade
            QPixmap qp = QPixmap::fromImage(img);

            emit display_frame(qp);
            emit send_frame(frame, id++);
        }
    }
}

void video_worker::stop_stream() {
    if (capture.isOpened()) {
        capture.release();
    }
}

