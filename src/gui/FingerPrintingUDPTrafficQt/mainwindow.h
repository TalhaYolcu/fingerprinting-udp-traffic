#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include "SnifferWorker.h"

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
    void onBatchReady(QString flowId, QVector<double> sizes, QVector<double> iats);
    void appendLog(QString msg);

private:
    Ui::MainWindow *ui;
    SnifferWorker *m_sniffer;
    QTcpSocket *m_tcpSocket;

    void sendToPythonAI(QString flowId, const QVector<double> &sizes, const QVector<double> &iats);
};

#endif // MAINWINDOW_H
