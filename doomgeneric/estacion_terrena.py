import socket
import numpy as np
import cv2

UDP_IP = "0.0.0.0" # escucha en todas las interfaces de red
UDP_PORT = 8080

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print("[ESTACIÓN TERRENA] Antena desplegada. Escuchando en el puerto 8080...")

while True:
    data, addr = sock.recvfrom(65535) 
    
    if len(data) == 64000:
        # convertimos los bytes a una matriz de imagen
        frame = np.frombuffer(data, dtype=np.uint8)
        frame = frame.reshape((200, 320))
        
        # mostramos la imagen ampliada para que se vea mejor
        frame_ampliado = cv2.resize(frame, (640, 400), interpolation=cv2.INTER_NEAREST)
        cv2.imshow("DOOM - Enlace Satelital", frame_ampliado)
        
        # si pulsas 'q' en la ventana de la imagen, se cierra
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

cv2.destroyAllWindows()