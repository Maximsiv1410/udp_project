#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <vector>

#include <QBuffer>
#include <QMessageBox>

#include <fstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->graphicsView->setScene(new QGraphicsScene(this));
    ui->graphicsView->scene()->addItem(&myPixmap);

    work.reset(new work_entity(ios));

    auto threads = std::thread::hardware_concurrency();
    if (!threads) threads = 2;

    for (std::size_t i = 0; i < 4; i++) {
        task_force.push_back(std::thread([this]{ ios.run(); }));
    }

    //sipper.set_begin_handler([](){});
    //sipper.set_end_handler([](){});

    sipper = std::make_unique<sip_engine>(ios, "127.0.0.1", 5060);
    sipper->start(true);

    connect(sipper.get(), &sip_engine::incoming_call, this, &MainWindow::incoming_call);


}

void MainWindow::on_startBtn_clicked()
{
    if (video.isOpened()) {
        ui->startBtn->setText("Start");
        video.release();
        return;
    }

    int deviceID = 0;             // 0 = open default camera
    int apiID = cv::CAP_ANY;      // 0 = autodetect default API
    if (video.open(deviceID, apiID)) {
        ui->startBtn->setText("Stop");
        std::size_t id = 0;
        while (video.isOpened()) {
            cv::Mat frame;
            video >> frame;
            if (!frame.empty()) {
                QImage img(frame.data,
                           frame.cols,
                           frame.rows,
                           frame.step,
                           QImage::Format_RGB888);

                myPixmap.setPixmap(QPixmap::fromImage(img.rgbSwapped()));
                ui->graphicsView->fitInView(&myPixmap, Qt::KeepAspectRatio);
                //ui->graphicsView->scene()->setSceneRect(0, 0, img.width(), img.height()); // bad

                if (rtp_service) {
                    rtp_service->cache_frame(frame, id++);
                }
            }
            qApp->processEvents();
        }
    }


}


void MainWindow::frame_gathered(QImage frame) {
    qDebug() << "frame got!!! form\n";
    partnerPixmap.setPixmap(QPixmap::fromImage(frame.rgbSwapped()));
    ui->partnerGraphicsView->fitInView(&partnerPixmap, Qt::KeepAspectRatio);
}



void MainWindow::incoming_call(/* session_info */) {
    rtp_service.reset(new rtp_io(ios, "127.0.0.1", 45777));
    rtp_service->start(true);

    connect(rtp_service.get(), &rtp_io::frame_gathered, this, &MainWindow::frame_gathered);

    ui->partnerGraphicsView->setScene(new QGraphicsScene(this));
    ui->partnerGraphicsView->scene()->addItem(&partnerPixmap);

    sipper->incoming_call_accepted();

    startStream();
}

void MainWindow::startStream() {
    if (video.isOpened()) {
        ui->startBtn->setText("Start");
        video.release();
        return;
    }

    if (video.open(0)) {
         ui->startBtn->setText("Stop");
         std::size_t id = 0;
        while (video.isOpened()) {
            cv::Mat frame;
            video >> frame;
            if (!frame.empty()) {
                QImage img(frame.data,
                           frame.cols,
                           frame.rows,
                           frame.step,
                           QImage::Format_RGB888);

                myPixmap.setPixmap(QPixmap::fromImage(img.rgbSwapped()));
                ui->graphicsView->fitInView(&myPixmap, Qt::KeepAspectRatio);
                //ui->graphicsView->scene()->setSceneRect(0, 0, img.width(), img.height()); // bad

                if (rtp_service) {
                    rtp_service->cache_frame(frame, id++);
                }

            }
            qApp->processEvents();
        }
    }
}


void MainWindow::on_callButton_clicked()
{
    if (!sipper) return;
    if (ui->callName->text().isEmpty()) return;

    rtp_service.reset(new rtp_io(ios, "127.0.0.1", 45777));
    rtp_service->start(true);

    connect(rtp_service.get(), &rtp_io::frame_gathered, this, &MainWindow::frame_gathered);

    ui->partnerGraphicsView->setScene(new QGraphicsScene(this));
    ui->partnerGraphicsView->scene()->addItem(&partnerPixmap);

    sipper->invite(ui->callName->text().toStdString());

    startStream();
}



void MainWindow::on_registerBtn_clicked()
{
    if (ui->registerTxt->text().isEmpty()) return;

    sipper->do_register(ui->registerTxt->text().toStdString());
}




MainWindow::~MainWindow()
{
    delete ui;

   /* if (sipper) {
        sipper->shutdown();
    }

    if (rtp_service) {
        rtp_service->shutdown();
    } */

    if (work) {
        work.reset(nullptr);
    }

    if (!ios.stopped()) {
        ios.stop();
    }

    for (auto & thread : task_force) {
        if (thread.joinable()) {
            thread.join();
        }
    }

}
