import socket
import numpy as np
import cv2
import sys

# --- CAMBIOS A LOCALHOST ---
#UDP_IP = "127.0.0.1
UDP_IP = "0.0.0.0"
UDP_PORT = 8080
MAC_IP = "127.0.0.1" 
UPLINK_PORT = 8081

sock_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv.bind((UDP_IP, UDP_PORT))
sock_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"[ESTACIÓN TERRENA] Escuchando vídeo... Listo para enviar UPLINK a {MAC_IP}")

key_map = {
    # WASD 
    ord('w'): 173,  
    ord('s'): 175,
    ord('a'): 172,
    ord('d'): 174,  
    
    # flechas (mac)
    0: 173,  
    1: 175,  
    2: 172, 
    3: 174,  
    
    # acción
    ord('e'): 32,   # abrir puertas
    32: 32,         # espacio
    13: 13,         # enter
    ord('f'): 157,  # disparar
}

try:
    while True:
        data, addr = sock_recv.recvfrom(65535) 
        
        if len(data) == 4000:
            frame = np.frombuffer(data, dtype=np.uint8)
            frame = frame.reshape((50, 80))
            frame_ampliado = cv2.resize(frame, (640, 400), interpolation=cv2.INTER_NEAREST)
            cv2.imshow("DOOM - Enlace Satelital", frame_ampliado)
            
            key = cv2.waitKeyEx(1)
            
            if key != -1 and key != 255:
                clean_key = key & 0xFF
                
                if clean_key in key_map:
                    doom_key = key_map[clean_key]
                elif key == 63232: doom_key = 173 # arriba
                elif key == 63233: doom_key = 175 # abajo
                elif key == 63234: doom_key = 172 # izquierda
                elif key == 63235: doom_key = 174 # derecha
                else:
                    doom_key = None
                
                if doom_key is not None:
                    sock_send.sendto(bytes([doom_key]), (MAC_IP, UPLINK_PORT))
                    print(f"Comando enviado: código original {key} -> DOOM: {doom_key}")
                elif clean_key == ord('q'):
                    print("Cerrando conexión...")
                    break
except Exception as e:
    print(f"Error en la Estación Terrena: {e}")
finally:
    cv2.destroyAllWindows()
    sys.exit(0)