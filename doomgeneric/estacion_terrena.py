# DOOM Satellite Link
# Copyright (C) 2026 Naiara Manteiga and Xian Lens
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

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

pico_maximo_bytes = 0
total_bytes_mision = 0
acumulador_10_frames = 0

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
        except Exception:
            pass

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
    bytes_hd_frame = 0
    contador_frames = 0 
    
    while True:
        data, addr = sock_recv.recvfrom(65535) 
        if len(data) == 1 and data[0] == 253:
            contador_frames += 1
            
            total_bytes_mision += bytes_hd_frame
            if bytes_hd_frame > pico_maximo_bytes:
                pico_maximo_bytes = bytes_hd_frame
                
            acumulador_10_frames += bytes_hd_frame
            if contador_frames % 10 == 0:
                media_ui = acumulador_10_frames // 10
                sys.stdout.write(f"\r📡 HD DELTA | Frame: {contador_frames:05d} | Ancho de banda usado: {media_ui:05d} bytes/frame   ")
                sys.stdout.flush()
                acumulador_10_frames = 0
            
            bytes_hd_frame = 0 
            
            frame = np.frombuffer(buffer_hd, dtype=np.uint8)
            frame = frame.reshape((200, 320))
            frame_ampliado = cv2.resize(frame, (1280, 800), interpolation=cv2.INTER_NEAREST)
            cv2.imshow("DOOM - Enlace Satelital", frame_ampliado)
            
            if cv2.waitKey(1) & 0xFF == ord('q'):
                print("\n[ESTACIÓN TERRENA] Cerrando...")
                break
        elif data[0] < 253:
            bytes_hd_frame += len(data)
            idx = 0
            
            while idx + 2 <= len(data):
                header = (data[idx] << 8) | data[idx+1]
                idx += 2
                
                is_solid = (header & 0x8000) != 0
                block_id = header & 0x7FFF
                
                bx = block_id % 40
                by = block_id // 40
                
                if is_solid:
                    if idx + 1 <= len(data):
                        byte_val = data[idx]
                        idx += 1
                        color_real = (byte_val & 0x0F) * 17
                        bloque_solido = bytes([color_real] * 8) 
                        for py in range(8):
                            inicio_buffer = ((by * 8) + py) * 320 + (bx * 8)
                            buffer_hd[inicio_buffer : inicio_buffer+8] = bloque_solido
                else:
                    if idx + 32 <= len(data):
                        pixeles_empaquetados = data[idx : idx+32]
                        idx += 32
                        
                        pixeles_reales = bytearray(64)
                        for i in range(32):
                            byte_val = pixeles_empaquetados[i]
                            pixeles_reales[i*2]     = (byte_val >> 4) * 17
                            pixeles_reales[i*2 + 1] = (byte_val & 0x0F) * 17
                        
                        for py in range(8):
                            inicio_buffer = ((by * 8) + py) * 320 + (bx * 8)
                            inicio_pixel = py * 8
                            buffer_hd[inicio_buffer : inicio_buffer+8] = pixeles_reales[inicio_pixel : inicio_pixel+8]

        elif len(data) == 4001 and data[0] == 255:
            ascii_pantalla = list(data[1:].decode('ascii', errors='replace'))
            imprimir_ascii(ascii_pantalla, len(data), "I-FRAME (KEYFRAME)")
            
        elif data[0] == 254:
            for i in range(1, len(data), 3):
                if i+2 < len(data):
                    idx_char = (data[i] << 8) | data[i+1]
                    char = chr(data[i+2])
                    if idx_char < 4000:
                        ascii_pantalla[idx_char] = char 
            imprimir_ascii(ascii_pantalla, len(data), "P-FRAME (DELTA)   ")

except KeyboardInterrupt:
    print("\n[ESTACIÓN TERRENA] Cierre forzado. Desconectando...")
except Exception as e:
    print(f"\n[ERROR] en la Estación Terrena: {e}")
finally:
    print("\033[0m")
    cv2.destroyAllWindows()
    
    if contador_frames > 0:
        media_total = total_bytes_mision // contador_frames
        print("\n" + "="*55)
        print("REPORTE FINAL DE TELEMETRÍA (UPLINK LEO)")
        print("="*55)
        print(f"Frames procesados:         {contador_frames} frames")
        print(f"Media de ancho de banda:   {media_total} bytes/frame")
        print(f"Pico máximo registrado:    {pico_maximo_bytes} bytes/frame")
        print("="*55 + "\n")
        
    sys.exit(0)