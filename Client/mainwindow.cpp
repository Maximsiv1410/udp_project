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
    auto threads = std::thread::hardware_concurrency();
    if (!threads) threads = 2;

    work.reset(new work_entity(ios));

    for (std::size_t i = 0; i < 4; i++) {
        task_force.push_back(std::thread([this]{ ios.run(); }));
    }

    //sipper.set_begin_handler([](){});
    //sipper.set_end_handler([](){});

    sipper = std::make_unique<sip_engine>(ios, "127.0.0.1", 5060);

    sipper->start(true);

    //connect(rtp_service.get(), &rtp_io::frame_gathered, this, &MainWindow::on_frame_gathered);
    connect(sipper.get(), &sip_engine::incoming_call, this, &MainWindow::incoming_call);

    setupCamera(QCameraInfo::defaultCamera());
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


void MainWindow::setupCamera(const QCameraInfo & cameraInfo) {
    // fot later needs to list for user all available cameras
    const QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    camera.reset(new QCamera(cameraInfo));

    imageCapture.reset(new QCameraImageCapture(camera.data()));
    imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);
    connect(imageCapture.data(), &QCameraImageCapture::imageCaptured, this, &MainWindow::processCapturedImage);

    camera->setViewfinder(ui->myViewFinder);
    camera->start();
}


void MainWindow::processCapturedImage(int requestId, const QImage& img) {
    Q_UNUSED(requestId)
    std::shared_ptr<QByteArray> bytes = std::make_shared<QByteArray>();
    QBuffer buffer(bytes.get());
    buffer.open(QIODevice::WriteOnly);

    // the higher the number -> better quality
    img.save(&buffer, "JPG", 5);
    qDebug() << buffer.size() << "\n";

    // here we can pass shared_ptr to media_client to make it cache our frame
    // and do neccessary manipulations to send fragmented frame

    std::ofstream ofs("C:\\Users\\Maxim\\Desktop\\frame.jpg", std::ios::binary);
    ofs.write(bytes->data(), bytes->size());
    ofs.close();

}


//  signals

/*void MainWindow::on_frame_gathered(std::shared_ptr<std::vector<char>> frame) {
    Q_UNUSED(frame)
} */

void MainWindow::incoming_call(std::string from) {
    QMessageBox msgbox;
    msgbox.setText(("You are being called by " + from).c_str());
    msgbox.exec();
}

void MainWindow::takeScreen() {
    qDebug() << "taking screenshot!\n";
    //m_isCapturingImage = true;
    imageCapture->capture();
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
