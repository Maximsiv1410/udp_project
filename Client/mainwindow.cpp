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

    for (std::size_t i = 0; i < 2; i++) {
        task_force.push_back(std::thread([this]{ ios.run(); }));
    }

    // later rtp_service should be created only
    // when session is established
    // with PARTICALUAR PORT, SPECIFIED BY SERVER in SDP
    // what's more => sipper will use callback(signal) to MainWindow
    // passing there struct session_info{server_port, etc... }
    // to create rtp_io with server-specified port and start communication
    // and also it will use callback(signal) to stop commincation over rtp_io
    rtp_service.reset(new rtp_io(ios, "127.0.0.1", 45777));
    connect(rtp_service.get(), &rtp_io::frame_gathered, this, &MainWindow::frame_gathered);
    rtp_service->start(true);

    // later sip_engine should send to remote x.x.x.x:5060 port
    // and also listen on localhost:5060, because it's SIP port
    sipper = std::make_unique<sip_engine>(ios, "127.0.0.1", 5060, rtp_service->local().port());
    connect(sipper.get(), &sip_engine::incoming_call, this, &MainWindow::incoming_call);
    sipper->start(true);

}

void MainWindow::incoming_call(/* session_info */) {
    ui->partnerGraphicsView->setScene(new QGraphicsScene(this));
    ui->partnerGraphicsView->scene()->addItem(&partnerPixmap);

    sipper->incoming_call_accepted();

    vworker.reset(new video_worker());
    vworker->moveToThread(&videoThread);

    connect(vworker.get(), &video_worker::display_frame, this, &MainWindow::display_frame);
    connect(vworker.get(), &video_worker::send_frame, rtp_service.get(), &rtp_io::cache_frame);
    connect(&videoThread, &QThread::started, vworker.get(), &video_worker::start_stream);
    connect(&videoThread, &QThread::finished, vworker.get(), &video_worker::stop_stream);

    videoThread.start();
}



void MainWindow::on_callButton_clicked()
{
    if (!sipper) return;
    if (ui->callName->text().isEmpty()) return;

    ui->partnerGraphicsView->setScene(new QGraphicsScene(this));
    ui->partnerGraphicsView->scene()->addItem(&partnerPixmap);

    sipper->invite(ui->callName->text().toStdString());

    vworker.reset(new video_worker());
    vworker->moveToThread(&videoThread);

    connect(vworker.get(), &video_worker::display_frame, this, &MainWindow::display_frame);
    connect(vworker.get(), &video_worker::send_frame, rtp_service.get(), &rtp_io::cache_frame);
    connect(&videoThread, &QThread::started, vworker.get(), &video_worker::start_stream);
    connect(&videoThread, &QThread::finished, vworker.get(), &video_worker::stop_stream);

    videoThread.start();
}

void MainWindow::on_registerBtn_clicked()
{
    if (ui->registerTxt->text().isEmpty()) return;
    sipper->do_register(ui->registerTxt->text().toStdString());
}

void MainWindow::on_stopCall_clicked()
{

}

/*
 * display frame, gathered by network
 */

void MainWindow::frame_gathered(QPixmap frame) {
    //qDebug() << "displaying [REMOTE] frame\n";
    partnerPixmap.setPixmap(frame);
    ui->partnerGraphicsView->fitInView(&partnerPixmap, Qt::KeepAspectRatio);
}


/*
 * display frame, gathered by local camera
 */
void MainWindow::display_frame(QPixmap frame) {
   // qDebug() << "displaying [LOCAL] frame\n";
    myPixmap.setPixmap(frame);
    ui->graphicsView->fitInView(&myPixmap, Qt::KeepAspectRatio);
}



void MainWindow::closeEvent(QCloseEvent *event) {
    qDebug() << "closing\n";
    event->accept();
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

    videoThread.quit();
    videoThread.wait();

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

    if (sipper) {
        sipper->shutdown();
    }

    if (rtp_service) {
        rtp_service->shutdown();
    }

}


