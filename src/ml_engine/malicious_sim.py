import socket
import time
import random

def simulate_c2_beacon(target_ip="127.0.0.1", target_port=53, duration=300):
    """
    DNS portu (53) üzerinden sahte bir C2 (Command & Control) tüneli simüle eder.
    Sabit boyutlu paketleri, belirli bir zaman aralığında (küçük jitter ile) gönderir.
    """
    print(f"[!] Zararli Trafik (C2 Beacon) Simülatörü Basladi!")
    print(f"[!] Hedef: {target_ip}:{target_port} (UDP)")
    print(f"[!] Süre: {duration} saniye boyunca sahte tünel trafigi uretilecek...\n")
    
    # UDP Soketi oluştur
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    
    # Botnet'lerin şifreli komut paketleri genelde sabit boyutludur (örneğin 128 byte)
    # Biz de tam 128 bytelik anlamsız veri (payload) oluşturuyoruz
    malicious_payload = os.urandom(128) if 'os' in globals() else b'\x99' * 128
    
    start_time = time.time()
    packet_count = 0
    
    try:
        while time.time() - start_time < duration:
            # Paketi gönder
            sock.sendto(malicious_payload, (target_ip, target_port))
            packet_count += 1
            
            # C2 Beacon ritmi: Her 0.2 saniyede bir sinyal gönder
            # Tespit edilmemek için %10'luk bir "Jitter" (rastgele sapma) ekliyoruz
            base_sleep = 0.2
            jitter = random.uniform(-0.02, 0.02) 
            time.sleep(base_sleep + jitter)
            
            if packet_count % 50 == 0:
                print(f"[*] {packet_count} adet sahte tünel paketi gönderildi...")
                
    except KeyboardInterrupt:
        print("\n[!] Simülasyon manuel olarak durduruldu.")
        
    finally:
        sock.close()
        print(f"\n[+] Simülasyon bitti. Toplam {packet_count} paket gönderildi.")

if __name__ == "__main__":
    # Gerekirse os kütüphanesini içeri alalım
    import os
    simulate_c2_beacon()