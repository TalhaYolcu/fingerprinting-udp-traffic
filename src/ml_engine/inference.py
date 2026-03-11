import socket
import json
import torch
import numpy as np
from model import ShadowStreamCNN

def start_inference_server(host='127.0.0.1', port=9999):
    print("[*] Yapay Zeka Modeli yukleniyor...")
    model = ShadowStreamCNN()
    
    try:
        model.load_state_dict(torch.load("../../data/models/shadow_stream_cnn.pth"))
        model.eval() # Çıkarım (Inference) moduna al
        print("[+] Model basariyla yuklendi!")
    except FileNotFoundError:
        print("[-] Hata: Egitilmis model bulunamadi. Once train.py calistirin.")
        return

    # Yerel bir TCP soketi baslat (IPC - Inter Process Communication icin)
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind((host, port))
    server_socket.listen(1)
    
    print(f"[*] IPC Sunucusu {host}:{port} uzerinde C++ motorunu dinliyor...")

    while True:
        conn, addr = server_socket.accept()
        # C++'tan gelen veriyi oku
        data = conn.recv(8192).decode('utf-8') 
        
        if not data:
            conn.close()
            continue
            
        try:
            # Gelen JSON verisini parse et (Format: {"sizes": [...50 adet...], "iats": [...50 adet...]})
            payload = json.loads(data)
            sizes = np.array(payload['sizes'])
            iats = np.array(payload['iats'])
            
            # Preprocess (Egitimde yaptigimiz ayni normalizasyon)
            sizes_norm = sizes / 1500.0
            iats_norm = np.log1p(iats)
            
            # Tensore cevir: (1, 2, 50) -> (Batch=1, Kanallar=2, Pencere=50)
            feature_matrix = np.vstack((sizes_norm, iats_norm))
            input_tensor = torch.tensor(feature_matrix, dtype=torch.float32).unsqueeze(0)
            
            # Modeli calistir (Inference)
            with torch.no_grad():
                output = model(input_tensor)
                score = output.item() # 0.0 ile 1.0 arasi skor
                
            # C++ tarafina sonucu JSON olarak geri gonder
            response = json.dumps({"score": score})
            conn.sendall(response.encode('utf-8'))
            
        except Exception as e:
            print(f"[-] Hata olustu: {e}")
            error_msg = json.dumps({"error": str(e)})
            conn.sendall(error_msg.encode('utf-8'))
            
        finally:
            conn.close()

if __name__ == "__main__":
    start_inference_server()