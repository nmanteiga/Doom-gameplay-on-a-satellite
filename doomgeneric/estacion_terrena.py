import socket
import numpy as np
import cv2

# UDP_IP = "127.0.0.1"
UDP_IP = "0.0.0.0"
UDP_PORT = 8080

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print("[ESTACIÓN TERRENA] Escuchando en 127.0.0.1:8080...")

while True:
    data, addr = sock.recvfrom(65535) 
    print(f"¡Impacto! Recibidos {len(data)} bytes desde {addr}") 
    
    # Ahora esperamos 4000 bytes en lugar de 64000
    # Volvemos a esperar el paquete seguro de 4000
    if len(data) == 4000:
        frame = np.frombuffer(data, dtype=np.uint8)
        frame = frame.reshape((50, 80)) # Matriz pequeña
        
        # El escalado hace la magia de rellenar la pantalla
        frame_ampliado = cv2.resize(frame, (640, 400), interpolation=cv2.INTER_NEAREST)
        cv2.imshow("DOOM - Enlace Satelital", frame_ampliado)

cv2.destroyAllWindows()