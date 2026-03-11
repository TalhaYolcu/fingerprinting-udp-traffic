#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_tcpSocket = new QTcpSocket(this);
    m_sniffer = new SnifferWorker(this);

    connect(m_sniffer, &SnifferWorker::logMessage, this, &MainWindow::appendLog);
    connect(m_sniffer, &SnifferWorker::batchReady, this, &MainWindow::onBatchReady);

    m_sniffer->start(); // Sniffer'ı arka planda başlat
}

MainWindow::~MainWindow()
{
    m_sniffer->stop();
    m_sniffer->wait();
    delete ui;
}

void MainWindow::appendLog(QString msg)
{
    QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->logTextEdit->append("[" + timeStr + "] " + msg);
}

void MainWindow::onBatchReady(QString flowId, QVector<double> sizes, QVector<double> iats)
{
    sendToPythonAI(flowId, sizes, iats);
}

void MainWindow::sendToPythonAI(QString flowId, const QVector<double> &sizes, const QVector<double> &iats)
{
    if(m_tcpSocket->state() != QAbstractSocket::ConnectedState) {
        m_tcpSocket->connectToHost("127.0.0.1", 9999);
        if(!m_tcpSocket->waitForConnected(1000)) {
            appendLog("HATA: Python AI Sunucusuna baglanilamadi! inference.py calisiyor mu?");
            return;
        }
    }

    QJsonObject json;
    QJsonArray sizesArray, iatsArray;
    for(double s : sizes) sizesArray.append(s);
    for(double i : iats) iatsArray.append(i);
    json["sizes"] = sizesArray;
    json["iats"] = iatsArray;

    QJsonDocument doc(json);
    m_tcpSocket->write(doc.toJson(QJsonDocument::Compact));

    if(m_tcpSocket->waitForReadyRead(2000)) {
        QByteArray response = m_tcpSocket->readAll();
        QJsonDocument resDoc = QJsonDocument::fromJson(response);
        double score = resDoc.object().value("score").toDouble();

        // 1. Durumu ve Rengi Belirle
        bool isMalicious = (score > 0.8);
        QString statusText = isMalicious ? "ZARARLI TÜNEL" : "GÜVENLİ";
        QColor bgColor = isMalicious ? QColor(255, 200, 200) : QColor(200, 255, 200); // Kırmızı veya Yeşil
        QColor textColor = isMalicious ? Qt::darkRed : Qt::darkGreen;

        // 2. Tabloya Yeni Satır Ekle
        int row = ui->flowTable->rowCount();
        ui->flowTable->insertRow(row);

        // Flow ID
        ui->flowTable->setItem(row, 0, new QTableWidgetItem(flowId));

        // Pencere Boyutu
        ui->flowTable->setItem(row, 1, new QTableWidgetItem("50 Paket"));

        // Skor
        QTableWidgetItem *scoreItem = new QTableWidgetItem(QString::number(score, 'f', 4));
        ui->flowTable->setItem(row, 2, scoreItem);

        // Durum (Renkli)
        QTableWidgetItem *statusItem = new QTableWidgetItem(statusText);
        statusItem->setBackground(bgColor);
        statusItem->setForeground(textColor);
        QFont font = statusItem->font();
        font.setBold(true);
        statusItem->setFont(font);
        ui->flowTable->setItem(row, 3, statusItem);

        // Tabloyu otomatik olarak en alta kaydır
        ui->flowTable->scrollToBottom();

        // Log ekranına da düş
        appendLog(QString("Analiz Tamamlandi -> Akis: %1 | Skor: %2").arg(flowId, QString::number(score, 'f', 4)));
    }
}
