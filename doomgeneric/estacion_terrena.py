import socket
import numpy as np
import cv2
import sys
from pynput import keyboard

UDP_IP = "0.0.0.0"
UDP_PORT = 8080
MAC_IP = "172.20.10.7" 
UPLINK_PORT = 8081

sock_recv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock_recv.bind((UDP_IP, UDP_PORT))
sock_send = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

print(f"[ESTACIÓN TERRENA] Escuchando vídeo... Listo para enviar UPLINK a {MAC_IP}")

key_map = {
    ord('w'): ord('w'), ord('s'): ord('s'), ord('a'): ord('a'), ord('d'): ord('d'),
    ord('e'): ord('e'), ord('f'): ord('f'), 13: 13, 27: 27
}

def send_key_event(pressed, key_char):
    if key_char in key_map:
        try:
            estado = 1 if pressed else 0
            sock_send.sendto(bytes([estado, key_char]), (MAC_IP, UPLINK_PORT))
            
            accion = "PULSADA" if pressed else "SOLTADA"
            print(f"📡 Enviando tecla: {chr(key_char).upper()} -> {accion}")
            
        except Exception as e:
            print(f"❌ Error enviando por red: {e}")

def on_press(key):
    try:
        send_key_event(True, ord(key.char.lower()))
    except AttributeError:
        if key == keyboard.Key.up: send_key_event(True, ord('w'))
        elif key == keyboard.Key.down: send_key_event(True, ord('s'))
        elif key == keyboard.Key.left: send_key_event(True, ord('a'))
        elif key == keyboard.Key.right: send_key_event(True, ord('d'))
        elif key == keyboard.Key.space: send_key_event(True, ord('e'))
        elif key == keyboard.Key.enter: send_key_event(True, 13)
        elif key == keyboard.Key.esc: send_key_event(True, 27)

def on_release(key):
    try:
        send_key_event(False, ord(key.char.lower()))
    except AttributeError:
        if key == keyboard.Key.up: send_key_event(False, ord('w'))
        elif key == keyboard.Key.down: send_key_event(False, ord('s'))
        elif key == keyboard.Key.left: send_key_event(False, ord('a'))
        elif key == keyboard.Key.right: send_key_event(False, ord('d'))
        elif key == keyboard.Key.space: send_key_event(False, ord('e'))
        elif key == keyboard.Key.enter: send_key_event(False, 13)
        elif key == keyboard.Key.esc: send_key_event(False, 27)

# Arrancamos el chivato del teclado en segundo plano
listener = keyboard.Listener(on_press=on_press, on_release=on_release)
listener.start()

try:
    buffer_frame = bytearray(64000) 
    
    while True:
        data, addr = sock_recv.recvfrom(65535) 
        
        # Si llega un paquete con cabecera (1 byte ID + 8000 bytes imagen)
        if len(data) == 8001:
            id_fragmento = data[0]      
            datos_imagen = data[1:]     
            
            # Encajamos la pieza del puzzle
            inicio = id_fragmento * 8000
            buffer_frame[inicio:inicio+8000] = datos_imagen
            
            # Si hemos recibido la última pieza (7), pintamos la pantalla
            if id_fragmento == 7:
                frame = np.frombuffer(buffer_frame, dtype=np.uint8)
                frame = frame.reshape((200, 320))
                frame_ampliado = cv2.resize(frame, (640, 400), interpolation=cv2.INTER_NEAREST)
                cv2.imshow("DOOM - Enlace Satelital", frame_ampliado)
                
                # Mantenemos viva la ventana y permitimos salir con la 'q'
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    print("Cerrando conexión...")
                    break
                        
except Exception as e:
    print(f"Error en la Estación Terrena: {e}")
finally:
    cv2.destroyAllWindows()
    sys.exit(0)