#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QImage>
#include <QPixmap>
#include <QCloseEvent>
#include <QMessageBox>
#include <QThread>

#include "video_worker.h"
#include "rtp_io.hpp"
#include "sip_engine.h"
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

protected:
    //void closeEvent(QCloseEvent *event);

private slots:
    void frame_gathered(QPixmap frame);

    void display_frame(QPixmap frame);

    void incoming_call();

    void on_registerBtn_clicked();

    void on_callButton_clicked();

protected:
    void closeEvent(QCloseEvent *event) override;
private:
    using work_entity = asio::io_context::work;
    using work_ptr = std::unique_ptr<work_entity>;

    Ui::MainWindow *ui;

    QThread videoThread;
    QGraphicsPixmapItem myPixmap;
    QGraphicsPixmapItem partnerPixmap;

    std::unique_ptr<video_worker> vworker;

    asio::io_context ios;
    work_ptr work;
    std::unique_ptr<sip_engine> sipper;
    std::unique_ptr<rtp_io> rtp_service;

    std::vector<std::thread> task_force;

};
#endif // MAINWINDOW_H
