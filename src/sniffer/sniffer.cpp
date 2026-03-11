#include <iostream>
#include <fstream>      // Dosya islemleri icin
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <unistd.h>
#include <chrono>
#include <map>
#include <tuple>
#include <string>

using namespace std::chrono;

typedef std::tuple<uint32_t, uint32_t, uint16_t, uint16_t> FlowKey;

struct FlowData {
    time_point<high_resolution_clock> last_packet_time;
    int packet_count;
};

// FlowKey'i okunabilir bir string'e ceviren yardimci fonksiyon (CSV icin)
std::string get_flow_id(uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport) {
    struct in_addr sip, dip;
    sip.s_addr = saddr;
    dip.s_addr = daddr;
    return std::string(inet_ntoa(sip)) + ":" + std::to_string(sport) + "-" +
           std::string(inet_ntoa(dip)) + ":" + std::to_string(dport);
}

int main() {
    int raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (raw_socket < 0) {
        std::cerr << "Soket hatasi! sudo ile calistirin." << std::endl;
        return 1;
    }

    // CSV Dosyasini olustur ve basliklari (Header) yaz
    // Not: Calistirdiginiz dizinde 'data/raw' klasorlerinin var oldugundan emin olun.
    std::ofstream csv_file("data/raw/udp_flows.csv");
    if (!csv_file.is_open()) {
        std::cerr << "CSV dosyasi acilamadi! data/raw/ klasoru var mi?" << std::endl;
        return 1;
    }
    
    // ML Modeli icin gereken kolonlar
    csv_file << "Flow_ID,Packet_Count,Size_Bytes,IAT_Microseconds\n";
    
    std::cout << "Sniffer basladi. Veriler 'data/raw/udp_flows.csv' dosyasina yaziliyor..." << std::endl;
    unsigned char buffer[65536]; 
    std::map<FlowKey, FlowData> flow_table;

    while (true) {
        int data_size = recvfrom(raw_socket, buffer, 65536, 0, NULL, NULL);
        if (data_size < 0) continue;

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
                std::string flow_id = get_flow_id(iph->saddr, iph->daddr, source_port, dest_port);

                long long iat = 0;

                if (flow_table.find(key) == flow_table.end()) {
                    flow_table[key] = {current_time, 1};
                } else {
                    auto duration = duration_cast<microseconds>(current_time - flow_table[key].last_packet_time);
                    iat = duration.count();
                    
                    flow_table[key].last_packet_time = current_time;
                    flow_table[key].packet_count++;
                }

                // Ekrana yazdir (opsiyonel, isterseniz yorum satirina alabilirsiniz)
                std::cout << "Flow: " << flow_id << " | Size: " << udp_length << " | IAT: " << iat << "\n";

                // Dosyaya yazdir
                csv_file << flow_id << "," 
                         << flow_table[key].packet_count << "," 
                         << udp_length << "," 
                         << iat << "\n";
                         
                csv_file.flush(); // Verinin aninda diske yazilmasini saglar
            }
        }
    }

    close(raw_socket);
    csv_file.close();
    return 0;
}