import socket
import numpy as np
import cv2
import sys
import os
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
    ord('e'): ord('e'), ord('f'): ord('f'), ord('m'): ord('m'), 13: 13, 27: 27
}

def send_key_event(pressed, key_char):
    if key_char in key_map:
        try:
            estado = 1 if pressed else 0
            sock_send.sendto(bytes([estado, key_char]), (MAC_IP, UPLINK_PORT))
            accion = "PULSADA" if pressed else "SOLTADA"
            #print(f"Enviando tecla: {chr(key_char).upper()} -> {accion}") # debug teclas
            
        except Exception as e:
            print(f"[ERROR] enviando por red: {e}")

def on_press(key):
    try:
        if key.char.lower() == 'q':
            os.kill(os.getpid(), 2) 
            return
            
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


listener = keyboard.Listener(on_press=on_press, on_release=on_release)
listener.start()


os.system('cls' if os.name == 'nt' else 'clear')

def imprimir_ascii(lista_chars, bytes_recibidos, tipo_frame):
    telemetria = f"\033[93m UPLINK: OK | MODO: ASCII DELTA | FRAME: {tipo_frame} | TAMAÑO: {bytes_recibidos:04d} bytes\n"
    telemetria += "="*80 + "\n\033[92m"
    
    pantalla = ""
    for i in range(50):
        inicio = i * 80
        pantalla += "".join(lista_chars[inicio:inicio+80]) + "\n"
    
    sys.stdout.write("\033[H" + telemetria + pantalla)
    sys.stdout.flush()

try:
    os.system('cls' if os.name == 'nt' else 'clear')
    buffer_hd = bytearray(64000) 
    ascii_pantalla = list(" " * 4000) 
    
    cv2.namedWindow("DOOM - Enlace Satelital") 
    
    while True:
        data, addr = sock_recv.recvfrom(65535) 
        
        if len(data) == 8001 and data[0] <= 7:
            id_fragmento = data[0]      
            inicio = id_fragmento * 8000
            buffer_hd[inicio:inicio+8000] = data[1:]
            
            if id_fragmento == 7:
                frame = np.frombuffer(buffer_hd, dtype=np.uint8)
                frame = frame.reshape((200, 320))
                frame_ampliado = cv2.resize(frame, (640, 400), interpolation=cv2.INTER_NEAREST)
                cv2.imshow("DOOM - Enlace Satelital", frame_ampliado)
                
                if cv2.waitKey(1) & 0xFF == ord('q'):
                    print("\n[ESTACIÓN TERRENA] Señal de cierre recibida (q). Desconectando...")
                    break

        elif len(data) == 4001 and data[0] == 255:
            # I-FRAME
            ascii_pantalla = list(data[1:].decode('ascii', errors='replace'))
            imprimir_ascii(ascii_pantalla, len(data), "I-FRAME (KEYFRAME)")
            
        elif data[0] == 254:
            # P-FRAME
            for i in range(1, len(data), 3):
                if i+2 < len(data):
                    idx = (data[i] << 8) | data[i+1]
                    char = chr(data[i+2])
                    if idx < 4000:
                        ascii_pantalla[idx] = char 
                        
            imprimir_ascii(ascii_pantalla, len(data), "P-FRAME (DELTA)   ")

except KeyboardInterrupt:
    print("\n[ESTACIÓN TERRENA] Cierre forzado. Desconectando...")
except Exception as e:
    print(f"\n[ERROR] en la Estación Terrena: {e}")
finally:
    print("\033[0m") 
    cv2.destroyAllWindows()
    sys.exit(0)