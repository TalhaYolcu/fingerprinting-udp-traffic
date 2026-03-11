#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <unistd.h>
#include <cstring>
#include <chrono> // Zaman olcumu icin

using namespace std::chrono;

int main() {
    int raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (raw_socket < 0) {
        std::cerr << "Soket hatasi! sudo ile calistirin." << std::endl;
        return 1;
    }

    std::cout << "Sniffer basladi. UDP Paketleri ve IAT (Zaman Farki) analiz ediliyor..." << std::endl;
    unsigned char buffer[65536]; 

    // Bir onceki paketin gelis zamanini tutacak degisken
    time_point<high_resolution_clock> last_packet_time = high_resolution_clock::now();
    bool is_first_packet = true;

    while (true) {
        int data_size = recvfrom(raw_socket, buffer, 65536, 0, NULL, NULL);
        if (data_size < 0) continue;

        // Paketin geldigi ani yakala
        auto current_packet_time = high_resolution_clock::now();

        struct ethhdr *eth = (struct ethhdr *)buffer;
        
        if (ntohs(eth->h_proto) == ETH_P_IP) {
            struct iphdr *iph = (struct iphdr *)(buffer + sizeof(struct ethhdr));

            if (iph->protocol == IPPROTO_UDP) {
                int ip_header_len = iph->ihl * 4;
                struct udphdr *udph = (struct udphdr *)(buffer + sizeof(struct ethhdr) + ip_header_len);

                uint16_t udp_length = ntohs(udph->len);

                // IAT Hesaplama (Mikrosaniye cinsinden)
                long long iat_microseconds = 0;
                if (!is_first_packet) {
                    auto duration = duration_cast<microseconds>(current_packet_time - last_packet_time);
                    iat_microseconds = duration.count();
                } else {
                    is_first_packet = false;
                }

                // Ekrana IAT ve Boyut bilgisini yazdir
                std::cout << "Boyut: " << udp_length << " byte | " 
                          << "IAT: " << iat_microseconds << " mikrosaniye" 
                          << std::endl;

                // Mevcut zamanı "son paket zamanı" olarak guncelle
                last_packet_time = current_packet_time;
            }
        }
    }

    close(raw_socket);
    return 0;
}