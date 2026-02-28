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
    # Enviamos la letra pura para que el Mac la traduzca
    ord('w'): ord('w'),  
    ord('s'): ord('s'),
    ord('a'): ord('a'),
    ord('d'): ord('d'),  
    
    # flechas (mac) -> las convertimos a wasd
    0: ord('w'),  
    1: ord('s'),  
    2: ord('a'), 
    3: ord('d'),
    63232: ord('w'), 63233: ord('s'), 63234: ord('a'), 63235: ord('d'),
    
    # acción pura
    ord('e'): ord('e'),   # abrir puertas
    ord('f'): ord('f'),   # disparar
    13: 13,               # enter
}

try:
    buffer_frame = bytearray() 
    
    while True:
        data, addr = sock_recv.recvfrom(65535) 
        if len(data) == 8000:
            buffer_frame.extend(data)
            
            if len(buffer_frame) == 64000:
                frame = np.frombuffer(buffer_frame, dtype=np.uint8)
                frame = frame.reshape((200, 320))
                frame_ampliado = cv2.resize(frame, (640, 400), interpolation=cv2.INTER_NEAREST)
                cv2.imshow("DOOM - Enlace Satelital", frame_ampliado)
                
                buffer_frame = bytearray() 
                key = cv2.waitKeyEx(1)
                
                if key != -1 and key != 255:
                    clean_key = key & 0xFF
                    
                    if clean_key in key_map:
                        doom_key = key_map[clean_key]
                    elif key == 63232: doom_key = 173 
                    elif key == 63233: doom_key = 175 
                    elif key == 63234: doom_key = 172 
                    elif key == 63235: doom_key = 174 
                    else:
                        doom_key = None
                    
                    if doom_key is not None:
                        sock_send.sendto(bytes([doom_key]), (MAC_IP, UPLINK_PORT))
                    elif clean_key == ord('q'):
                        print("Cerrando conexión...")
                        break
                        
            elif len(buffer_frame) > 64000:
                buffer_frame = bytearray()
                
except Exception as e:
    print(f"Error en la Estación Terrena: {e}")
finally:
    cv2.destroyAllWindows()
    sys.exit(0)