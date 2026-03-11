import pandas as pd
import numpy as np
import os

def process_pcap_csv(csv_path, window_size=50, overlap=0.5):
    """
    CSV dosyasını okur ve 1D-CNN için kayan pencereler (sliding windows) oluşturur.
    
    Çıktı Boyutu: (Örnek Sayısı, Kanal Sayısı, Pencere Boyutu)
    Kanal 1: Paket Boyutları (Size_Bytes)
    Kanal 2: Geliş Zamanı Farkları (IAT_Microseconds)
    """
    print(f"[{csv_path}] dosyasi isleniyor...")
    
    try:
        df = pd.read_csv(csv_path)
    except FileNotFoundError:
        print(f"Hata: {csv_path} bulunamadi. C++ sniffer'in calistigindan emin olun.")
        return None
    
    windows = []
    step_size = int(window_size * (1 - overlap)) # %50 örtüşme (overlap) ile kaydır
    
    # Veriyi akışlara (Flow_ID) göre grupla
    grouped = df.groupby('Flow_ID')
    
    for flow_id, group in grouped:
        sizes = group['Size_Bytes'].values
        iats = group['IAT_Microseconds'].values
        
        # Bir akıştaki paket sayısı pencere boyutundan küçükse atla
        if len(sizes) < window_size:
            continue
            
        # Kayan pencereyi uygula
        for i in range(0, len(sizes) - window_size + 1, step_size):
            window_sizes = sizes[i : i + window_size]
            window_iats = iats[i : i + window_size]
            
            # Normalizasyon: Boyutları 0-1 arasına çek (Maksimum MTU genelde 1500 byte'tır)
            window_sizes_norm = window_sizes / 1500.0
            
            # Normalizasyon: IAT değerleri çok değişken olabilir, Logaritma alarak yumuşatıyoruz
            # Log(0) hatası almamak için +1 ekliyoruz
            window_iats_norm = np.log1p(window_iats)
            
            # Özellikleri birleştir: Shape -> (2, 50)
            feature_matrix = np.vstack((window_sizes_norm, window_iats_norm))
            windows.append(feature_matrix)
            
    # Listeyi Numpy dizisine çevir
    X = np.array(windows)
    
    print(f"Islem tamamlandi. Toplam {len(X)} adet pencere (örnek) olusturuldu.")
    print(f"Veri matrisi sekli: {X.shape} -> (Ornek_Sayisi, Kanallar(Boyut, IAT), Pencere_Boyutu)")
    
    return X

if __name__ == "__main__":
    # Test için çalıştır
    # C++ kodunun ürettiği csv dosyasının yolu
    csv_file_path = "../../data/raw/udp_flows.csv" 
    
    # Kodu test etmek için boş bir csv varsa hata vermemesi için kontrol
    if os.path.exists(csv_file_path):
        X_benign = process_pcap_csv(csv_file_path, window_size=50)
        
        if X_benign is not None:
            # İşlenmiş veriyi ileride kullanmak üzere numpy formatında kaydet
            os.makedirs("../../data/processed", exist_ok=True)
            np.save("../../data/processed/X_benign.npy", X_benign)
            print("Veri basariyla 'data/processed/X_benign.npy' olarak kaydedildi.")
    else:
        print(f"Lutfen once C++ sniffer ile {csv_file_path} dosyasini uretin.")