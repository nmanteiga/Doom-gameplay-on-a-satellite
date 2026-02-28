import socket
import numpy as np
import cv2
import sys

UDP_IP = "0.0.0.0"
UDP_PORT = 8080

MAC_IP = "172.20.10.7" 
UPLINK_PORT = 8081

sock_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv.bind((UDP_IP, UDP_PORT))
sock_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"[ESTACIÓN TERRENA] Escuchando vídeo... Listo para enviar UPLINK a {MAC_IP}")

key_map = {
    ord('w'): 173,  
    ord('s'): 175,  
    ord('a'): 172,  
    ord('d'): 174, 
    ord('e'): 32,   
    13: 13,         
    ord('f'): 157,  
}

try:
    while True:
        data, addr = sock_recv.recvfrom(65535) 
        
        if len(data) == 4000:
            frame = np.frombuffer(data, dtype=np.uint8)
            frame = frame.reshape((50, 80))
            frame_ampliado = cv2.resize(frame, (640, 400), interpolation=cv2.INTER_NEAREST)
            cv2.imshow("DOOM - Enlace Satelital", frame_ampliado)
            
            key = cv2.waitKey(1) & 0xFF
            
            if key != 255:
                if key in key_map:
                    doom_key = key_map[key]
                    sock_send.sendto(bytes([doom_key]), (MAC_IP, UPLINK_PORT))
                    print(f"Comando enviado: {chr(key) if key < 128 else key} -> DOOM: {doom_key}")
                elif key == ord('q'):
                    print("Cerrando conexión...")
                    break
except Exception as e:
    print(f"Error en la Estación Terrena: {e}")
finally:
    cv2.destroyAllWindows()
    sys.exit(0)