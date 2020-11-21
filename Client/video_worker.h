#ifndef VIDEO_WORKER_H
#define VIDEO_WORKER_H

#include <opencv2/highgui.hpp>
#include <QPixmap>
#include <QImage>

Q_DECLARE_METATYPE(cv::Mat);

class video_worker : public QObject
{
    Q_OBJECT
public:
    video_worker();

    void start_stream();

    void stop_stream();

signals:
    void display_frame(QPixmap frame);

    void send_frame(cv::Mat matrix, std::uint32_t id);

private:
    cv::VideoCapture capture;
};

#endif // VIDEO_WORKERHPP_H
