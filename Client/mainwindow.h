#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>
#include <QCloseEvent>
#include <QMessageBox>

#include "rtp_io.hpp"
#include "sip_engine.hpp"
#include "opencv2/highgui.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


private slots:
    //void on_frame_gathered(std::shared_ptr<std::vector<char>> frame);

    void incoming_call(std::string from);

    void startCall();

    void on_registerBtn_clicked();

    void on_startBtn_clicked();

private:
    using work_entity = asio::io_context::work;
    using work_ptr = std::unique_ptr<work_entity>;

    Ui::MainWindow *ui;


    QGraphicsPixmapItem pixmap;
    cv::VideoCapture video;

    std::unique_ptr<sip_engine> sipper;
    std::unique_ptr<rtp_io> rtp_service;

    asio::io_context ios;
    work_ptr work;
    std::vector<std::thread> task_force;

};
#endif // MAINWINDOW_H
