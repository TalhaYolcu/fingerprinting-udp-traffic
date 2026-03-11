#ifndef SNIFFERWORKER_H
#define SNIFFERWORKER_H


#include <QObject>
#include <QWidget>
#include <QThread>
#include <QVector>
#include <QString>
#include <map>
#include <tuple>
#include <chrono>

typedef std::tuple<uint32_t, uint32_t, uint16_t, uint16_t> FlowKey;

struct FlowData {
    std::chrono::time_point<std::chrono::high_resolution_clock> last_packet_time;
    int packet_count;
    QVector<double> sizes;
    QVector<double> iats;
};

class SnifferWorker : public QThread
{
    Q_OBJECT
public:
    explicit SnifferWorker(QObject *parent = nullptr);
    void run() override;   // İş parçacığının ana döngüsü
    void stop();           // Sniffer'ı durdurmak için

signals:
    // 50 paket biriktiğinde yapay zekaya gönderilmek üzere ana ekrana fırlatılacak sinyal
    void batchReady(QString flowId, QVector<double> sizes, QVector<double> iats);
    void logMessage(QString msg);

private:
    bool m_running;
};

#endif // SNIFFERWORKER_H
