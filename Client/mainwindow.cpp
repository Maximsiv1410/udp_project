#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <vector>

#include <QMediaService>
#include <QMediaRecorder>
#include <QCameraViewfinder>
#include <QCameraInfo>
#include <QMediaMetaData>
#include <QBuffer>
#include <QMessageBox>

#include <fstream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->graphicsView->setScene(new QGraphicsScene(this));
    ui->graphicsView->scene()->addItem(&pixmap);



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

    //connect(rtp_service.get(), &rtp_io::frame_gathered, this, &MainWindow::on_frame_gathered);
    connect(sipper.get(), &sip_engine::incoming_call, this, &MainWindow::incoming_call);


}

void MainWindow::on_startBtn_clicked()
{
    if (video.isOpened()) {
        ui->startBtn->setText("Start");
        video.release();
        return;
    }

    if (video.open(0)) {
         ui->startBtn->setText("Stop");
        while (video.isOpened()) {
            cv::Mat frame;
            video >> frame;
            if (!frame.empty()) {
                QImage img(frame.data,
                           frame.cols,
                           frame.rows,
                           frame.step,
                           QImage::Format_RGB888);

                pixmap.setPixmap(QPixmap::fromImage(img.rgbSwapped()));
                ui->graphicsView->fitInView(&pixmap, Qt::KeepAspectRatio);
                //ui->graphicsView->scene()->setSceneRect(0, 0, img.width(), img.height()); // bad


            }
            qApp->processEvents();
        }
    }


}













void MainWindow::incoming_call(std::string from) {
    QMessageBox msgbox;
    msgbox.setText(("You are being called by " + from).c_str());
    msgbox.exec();
}


void MainWindow::startCall() {
    //ui->callButton->setEnabled(false);

    if (!sipper) return;
    if (ui->callName->text().isEmpty()) return;
    std::string who = ui->callName->text().toStdString();

    sipper->invite(ui->callName->text().toStdString());
}

void MainWindow::on_registerBtn_clicked()
{
    if (ui->registerTxt->text().isEmpty()) return;

    sipper->do_register(ui->registerTxt->text().toStdString());
}








MainWindow::~MainWindow()
{
    delete ui;

    if (sipper) {
        sipper->shutdown();
    }

    if (rtp_service) {
        rtp_service->shutdown();
    }

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

