import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, TensorDataset
from sklearn.model_selection import train_test_split
import numpy as np
import os

# Kendi yazdığımız modülleri içe aktaralım
from preprocess import process_pcap_csv
from model import ShadowStreamCNN

def prepare_data():
    print("[*] Veriler hazirlaniyor...")
    
    # 1. CSV dosyalarını oku ve pencerelere böl
    X_benign = process_pcap_csv("../../data/raw/benign_flows.csv", window_size=50)
    X_malicious = process_pcap_csv("../../data/raw/malicious_flows.csv", window_size=50)
    
    if X_benign is None or X_malicious is None:
        raise FileNotFoundError("Veri setleri bulunamadi!")
        
    # 2. Etiketleri oluştur (0: Güvenli, 1: Zararlı Tünel)
    y_benign = np.zeros(len(X_benign))
    y_malicious = np.ones(len(X_malicious))
    
    # 3. Verileri birleştir
    X = np.vstack((X_benign, X_malicious))
    y = np.concatenate((y_benign, y_malicious))
    
    # 4. Eğitim ve Test (%80 Eğitim, %20 Test) olarak ayır
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42, stratify=y)
    
    # 5. PyTorch Tensörlerine Çevir
    X_train_t = torch.tensor(X_train, dtype=torch.float32)
    y_train_t = torch.tensor(y_train, dtype=torch.float32).view(-1, 1)
    X_test_t = torch.tensor(X_test, dtype=torch.float32)
    y_test_t = torch.tensor(y_test, dtype=torch.float32).view(-1, 1)
    
    return X_train_t, y_train_t, X_test_t, y_test_t

def train_model():
    # Verileri al
    X_train, y_train, X_test, y_test = prepare_data()
    
    # DataLoader oluştur (Veriyi 32'şerli paketler halinde modele beslemek için)
    train_dataset = TensorDataset(X_train, y_train)
    train_loader = DataLoader(train_dataset, batch_size=32, shuffle=True)
    
    # Modeli, Kayıp Fonksiyonunu ve Optimizatörü tanımla
    model = ShadowStreamCNN()
    criterion = nn.BCELoss() # Binary Cross Entropy (İkili Sınıflandırma için ideal)
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    
    epochs = 10
    print(f"\n[*] Egitim basliyor... Toplam Veri: {len(X_train)} Egitim, {len(X_test)} Test")
    
    for epoch in range(epochs):
        model.train()
        epoch_loss = 0
        
        for batch_X, batch_y in train_loader:
            optimizer.zero_grad()
            outputs = model(batch_X)
            loss = criterion(outputs, batch_y)
            loss.backward()
            optimizer.step()
            epoch_loss += loss.item()
            
        # Her epoch sonunda test verisiyle doğruluğu (Accuracy) ölç
        model.eval()
        with torch.no_grad():
            test_outputs = model(X_test)
            predictions = (test_outputs >= 0.8).float() # 0.8 Eşiğini geçiyorsa Zararlı (1) say
            correct = (predictions == y_test).sum().item()
            accuracy = correct / len(y_test) * 100
            
        print(f"Epoch [{epoch+1}/{epochs}] | Kayip (Loss): {epoch_loss/len(train_loader):.4f} | Dogruluk: %{accuracy:.2f}")

    # Eğitilmiş modeli kaydet
    os.makedirs("../../data/models", exist_ok=True)
    model_path = "../../data/models/shadow_stream_cnn.pth"
    torch.save(model.state_dict(), model_path)
    print(f"\n[+] Egitim tamamlandi! Model agirliklari kaydedildi: {model_path}")

if __name__ == "__main__":
    train_model()