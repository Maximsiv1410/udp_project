#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCamera>
#include <QCameraImageCapture>
#include <QMediaRecorder>
#include <QScopedPointer>

#include "rtp_io.hpp"
#include "sip_engine.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setupCamera(const QCameraInfo & cameraInfo);

private slots:
    void on_frame_gathered(std::shared_ptr<std::vector<char>> frame);

    void processCapturedImage(int requestId, const QImage& img);

    void takeScreen();

    void startCall();

    void on_registerBtn_clicked();

private:
    using work_entity = asio::io_context::work;
    using work_ptr = std::unique_ptr<work_entity>;

    Ui::MainWindow *ui;

    QScopedPointer<QCamera> camera;
    QScopedPointer<QCameraImageCapture> imageCapture;
    QScopedPointer<QMediaRecorder> mediaRecorder;

    QImageEncoderSettings imageSettings;
    QAudioEncoderSettings audioSettings;
    QVideoEncoderSettings videoSettings;

    std::unique_ptr<sip_engine> sipper;
    std::unique_ptr<rtp_io> rtp_service;

    asio::io_context ios;
    work_ptr work;
    std::vector<std::thread> task_force;



};
#endif // MAINWINDOW_H
