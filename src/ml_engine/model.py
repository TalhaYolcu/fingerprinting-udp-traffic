import torch
import torch.nn as nn

class ShadowStreamCNN(nn.Module):
    def __init__(self, window_size=50):
        super(ShadowStreamCNN, self).__init__()
        
        # Girdi: (Batch_Size, 2 Kanal, 50 Paket)
        # 2 Kanal = (Paket Boyutu, IAT)
        
        # 1. Evrişim Bloğu (Özellik Çıkarımı)
        self.conv1 = nn.Conv1d(in_channels=2, out_channels=16, kernel_size=3, padding=1)
        self.relu1 = nn.ReLU()
        self.pool1 = nn.MaxPool1d(kernel_size=2) # Boyutu yarıya düşürür (50 -> 25)
        
        # 2. Evrişim Bloğu (Derin Özellik Çıkarımı)
        self.conv2 = nn.Conv1d(in_channels=16, out_channels=32, kernel_size=3, padding=1)
        self.relu2 = nn.ReLU()
        self.pool2 = nn.MaxPool1d(kernel_size=2) # Boyutu yarıya düşürür (25 -> 12)
        
        # Tam Bağlantılı (Fully Connected) Katmanlar
        # Havuzlamadan sonra boyut: 32 kanal * 12 özellik = 384
        self.fc1 = nn.Linear(32 * 12, 64)
        self.relu3 = nn.ReLU()
        self.dropout = nn.Dropout(0.5) # Aşırı öğrenmeyi (Overfitting) engellemek için
        
        # Çıktı Katmanı: 0.0 ile 1.0 arası tek bir olasılık skoru
        self.fc2 = nn.Linear(64, 1)
        self.sigmoid = nn.Sigmoid()

    def forward(self, x):
        # x.shape = (Batch, 2, 50)
        x = self.pool1(self.relu1(self.conv1(x)))
        x = self.pool2(self.relu2(self.conv2(x)))
        
        x = x.view(x.size(0), -1) # Düzleştir (Flatten)
        
        x = self.dropout(self.relu3(self.fc1(x)))
        x = self.sigmoid(self.fc2(x)) # Çıktı: 0 (Benign) ile 1 (Malicious) arası
        
        return x

if __name__ == "__main__":
    # Modeli test edelim
    model = ShadowStreamCNN()
    dummy_input = torch.randn(1, 2, 50) # Rastgele 1 adet 50 paketlik örnek
    output = model(dummy_input)
    print(f"Model Test Çıktısı (Olasılık Skoru): {output.item():.4f}")