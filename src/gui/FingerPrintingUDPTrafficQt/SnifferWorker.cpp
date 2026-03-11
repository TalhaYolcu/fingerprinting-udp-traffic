#include "SnifferWorker.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <unistd.h>
#include <QDebug>

using namespace std::chrono;

SnifferWorker::SnifferWorker(QObject *parent) : QThread(parent), m_running(false) {}

void SnifferWorker::stop() {
    m_running = false;
}

void SnifferWorker::run() {
    m_running = true;
    int raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (raw_socket < 0) {
        emit logMessage("Hata: Raw Socket baslatilamadi. Programi sudo ile calistirin.");
        return;
    }

    emit logMessage("Sniffer Thread basladi. Paketler bekleniyor...");
    unsigned char buffer[65536];
    std::map<FlowKey, FlowData> flow_table;

    while (m_running) {
        int data_size = recvfrom(raw_socket, buffer, 65536, MSG_DONTWAIT, NULL, NULL);
        if (data_size <= 0) {
            msleep(1); // İşlemciyi yormamak için çok kısa bir bekleme
            continue;
        }

        auto current_time = high_resolution_clock::now();
        struct ethhdr *eth = (struct ethhdr *)buffer;

        if (ntohs(eth->h_proto) == ETH_P_IP) {
            struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));

            if (iph->protocol == IPPROTO_UDP) {
                int ip_header_len = iph->ihl * 4;
                struct udphdr *udph = (struct udphdr *)(buffer + sizeof(struct ethhdr) + ip_header_len);

                uint16_t source_port = ntohs(udph->source);
                uint16_t dest_port = ntohs(udph->dest);
                uint16_t udp_length = ntohs(udph->len);

                FlowKey key = std::make_tuple(iph->saddr, iph->daddr, source_port, dest_port);

                if (flow_table.find(key) == flow_table.end()) {
                    flow_table[key] = {current_time, 1, QVector<double>(), QVector<double>()};
                    flow_table[key].sizes.append(udp_length);
                    flow_table[key].iats.append(0.0);
                } else {
                    auto duration = duration_cast<microseconds>(current_time - flow_table[key].last_packet_time);
                    double iat = static_cast<double>(duration.count());

                    flow_table[key].last_packet_time = current_time;
                    flow_table[key].packet_count++;
                    flow_table[key].sizes.append(udp_length);
                    flow_table[key].iats.append(iat);

                    // PENCERE DOLDU (50 Paket) -> Python'a gonderilmek uzere sinyal firlat
                    if (flow_table[key].packet_count == 50) {
                        struct in_addr sip, dip;
                        sip.s_addr = iph->saddr;
                        dip.s_addr = iph->daddr;
                        QString flowId = QString("%1:%2 -> %3:%4")
                                             .arg(inet_ntoa(sip)).arg(source_port)
                                             .arg(inet_ntoa(dip)).arg(dest_port);

                        emit batchReady(flowId, flow_table[key].sizes, flow_table[key].iats);

                        // Penceriyi sifirla (Sliding Window yerine atlayarak (jumping) gidiyoruz basitlik icin)
                        flow_table[key].packet_count = 0;
                        flow_table[key].sizes.clear();
                        flow_table[key].iats.clear();
                    }
                }
            }
        }
    }
    close(raw_socket);
}
