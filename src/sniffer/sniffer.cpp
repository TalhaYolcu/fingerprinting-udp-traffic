#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_ether.h> // ETH_P_ALL için gerekli
#include <unistd.h>
#include <cstring>

int main() {
    // 1. Raw Socket Oluşturma
    // AF_PACKET: Düşük seviyeli paket arayüzü
    // SOCK_RAW: Ham ağ protokolü erişimi
    // ETH_P_ALL: Gelen ve giden tüm paketleri yakala
    int raw_socket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (raw_socket < 0) {
        std::cerr << "Soket olusturulamadi! Programi 'sudo' ile calistirdiginiza emin olun." << std::endl;
        return 1;
    }

    std::cout << "Sniffer baslatildi. Paketler bekleniyor..." << std::endl;

    // Paketi okumak için bir buffer (tam bir Ethernet çerçevesi genelde 1514 byte'tır, biz 65536 veriyoruz)
    unsigned char buffer[65536]; 

    // 2. Sonsuz döngüde paketleri dinleme
    while (true) {
        // Ağdan gelen veriyi buffer'a al
        int data_size = recvfrom(raw_socket, buffer, 65536, 0, NULL, NULL);

        if (data_size < 0) {
            std::cerr << "Paket okuma hatasi!" << std::endl;
            return 1;
        }

        // Şimdilik sadece yakalanan paketin boyutunu ekrana yazdırıyoruz
        std::cout << "Paket yakalandi! Boyut: " << data_size << " byte." << std::endl;
        
        // Çok hızlı akacağı için terminali dondurmaması adına küçük bir gecikme ekliyoruz (opsiyonel)
        usleep(10000); 
    }

    close(raw_socket);
    return 0;
}