/*
 * DOOM Satellite Link
 * Copyright (C) 2026 Naiara Manteiga and Xian Lens
 * * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>  
#include <arpa/inet.h>   
#include <stdint.h> 
#include <fcntl.h> 
#include <string.h>    

#define MAX_EVENTS 64
typedef struct { int pressed; unsigned char key; } KeyEvent;

// variables globales para el socket
int sockfd;
struct sockaddr_in servaddr;
int uplink_fd;

int modo_video = 0; // 0 = HD DELTA, 1 = ASCII DELTA
int frame_counter = 0;
char buffer_anterior[4000];
uint8_t buffer_anterior_hd[64000] = {0};

KeyEvent key_queue[MAX_EVENTS];
int q_head = 0;
int q_tail = 0;

extern int key_up;
extern int key_down;
extern int key_left;
extern int key_right;
extern int key_fire;
extern int key_use;

void DG_Init() { 
    printf("\n[SATÉLITE] Sistemas online. Inicializando antenas...\n"); 
    key_up = 'w'; key_down = 's'; key_left = 'a'; key_right = 'd'; key_fire = 'f'; key_use = 'e';  

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8080);
    
    //const char* ip_destino = "127.0.0.1"; 
    const char* ip_destino = "172.20.10.2"; 
    servaddr.sin_addr.s_addr = inet_addr(ip_destino);
    
    uplink_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in recvaddr;
    memset(&recvaddr, 0, sizeof(recvaddr));
    recvaddr.sin_family = AF_INET;
    recvaddr.sin_port = htons(8081); 
    recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    bind(uplink_fd, (struct sockaddr *)&recvaddr, sizeof(recvaddr));
    fcntl(uplink_fd, F_SETFL, O_NONBLOCK); 
}

void DG_DrawFrame() { 
    uint8_t buffer_comprimido[64000];
    int indice = 0;
    
    for (int y = 0; y < 400; y += 2) {
        for (int x = 0; x < 640; x += 2) {
            int pixel_original = (y * 640) + x; 
            buffer_comprimido[indice++] = (DG_ScreenBuffer[pixel_original] >> 8) & 0xFF;
        }
    }

    if (modo_video == 0) {
        uint8_t pkt[1400];
        int pkt_size = 0;
        int es_iframe = (frame_counter % 30 == 0); 
        
        for (int by = 0; by < 25; by++) {
            for (int bx = 0; bx < 40; bx++) {
                int block_id = by * 40 + bx;
                int changed = es_iframe;
                
                 if (!changed) {
                    for (int py = 0; py < 8; py++) {
                        int row_offset = ((by * 8) + py) * 320 + (bx * 8);
                        for (int px = 0; px < 8; px++) {
                            if ((buffer_comprimido[row_offset + px] & 0xF0) != (buffer_anterior_hd[row_offset + px] & 0xF0)) {
                                changed = 1;
                                break;
                            }
                        }
                        if (changed) break; 
                    }
                }

                
                if (changed) {
                    uint8_t primer_pixel = buffer_comprimido[(by * 8) * 320 + (bx * 8)] >> 4;
                    int es_solido = 1;
                    for (int py = 0; py < 8; py++) {
                        int row_offset = ((by * 8) + py) * 320 + (bx * 8);
                        for (int px = 0; px < 8; px++) {
                            if ((buffer_comprimido[row_offset + px] >> 4) != primer_pixel) {
                                es_solido = 0; break;
                            }
                        }
                        if (!es_solido) break;
                    }

                    if (pkt_size + 34 > 1000) {
                        sendto(sockfd, pkt, pkt_size, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
                        pkt_size = 0;
                    }
                    
                    if (es_solido) {
                        int header = block_id | 0x8000;
                        pkt[pkt_size++] = (header >> 8) & 0xFF;
                        pkt[pkt_size++] = header & 0xFF;
                        pkt[pkt_size++] = primer_pixel; 
                    } else {
                        int header = block_id;
                        pkt[pkt_size++] = (header >> 8) & 0xFF;
                        pkt[pkt_size++] = header & 0xFF;
                        
                        for (int py = 0; py < 8; py++) {
                            int row_offset = ((by * 8) + py) * 320 + (bx * 8);
                            for (int px = 0; px < 8; px += 2) {
                                uint8_t p1 = buffer_comprimido[row_offset + px] >> 4;
                                uint8_t p2 = buffer_comprimido[row_offset + px + 1] >> 4;
                                pkt[pkt_size++] = (p1 << 4) | p2;
                            }
                        }
                    }
                }
            }
        }
        
        if (pkt_size > 0) {
            sendto(sockfd, pkt, pkt_size, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        }
        uint8_t eof_pkt[1] = {253};
        sendto(sockfd, eof_pkt, 1, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        memcpy(buffer_anterior_hd, buffer_comprimido, 64000);

    } else {
        char ascii_buffer[4000];
        const char* paleta = " .:-=+*#@";
        int idx_ascii = 0;
        
        for (int y = 0; y < 400; y += 8) {
            for (int x = 0; x < 640; x += 8) {
                int px = (y * 640) + x;
                int brillo = (DG_ScreenBuffer[px] >> 8) & 0xFF;
                ascii_buffer[idx_ascii++] = paleta[(brillo * 8) / 255];
            }
        }

        int cambios = 0;
        for (int i = 0; i < 4000; i++) {
            if (ascii_buffer[i] != buffer_anterior[i]) cambios++;
        }

        if (frame_counter % 30 == 0 || (cambios * 3) > 4000) {
            uint8_t pkt[4001];
            pkt[0] = 255; 
            memcpy(&pkt[1], ascii_buffer, 4000);
            sendto(sockfd, pkt, 4001, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
            memcpy(buffer_anterior, ascii_buffer, 4000);
        } else if (cambios > 0) {
            uint8_t delta_pkt[8000]; 
            delta_pkt[0] = 254; 
            int d_size = 1;

            for (int i = 0; i < 4000; i++) {
                if (ascii_buffer[i] != buffer_anterior[i]) {
                    delta_pkt[d_size++] = (i >> 8) & 0xFF; 
                    delta_pkt[d_size++] = i & 0xFF;        
                    delta_pkt[d_size++] = ascii_buffer[i];
                    buffer_anterior[i] = ascii_buffer[i];
                }
            }
            sendto(sockfd, delta_pkt, d_size, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        }
    }
    frame_counter++;
    if (frame_counter % 2 != 0) return;

    usleep(30000); 
}

void push_key(int pressed, unsigned char key) {
    int next = (q_head + 1) % MAX_EVENTS;
    if (next != q_tail) { 
        key_queue[q_head].pressed = pressed;
        key_queue[q_head].key = key;
        q_head = next;
    }
}

int DG_GetKey(int* pressed, unsigned char* doomKey) { 
    unsigned char net_cmd[2];
    while (recvfrom(uplink_fd, net_cmd, 2, 0, NULL, NULL) == 2) {
        if (net_cmd[0] && net_cmd[1] == 'm') {
            modo_video = !modo_video; 
            continue; 
        }
        unsigned char k = net_cmd[1];
        if (k == 13) k = KEY_ENTER;
        if (k == 27) k = 27;
        push_key(net_cmd[0], k);
    }

    if (q_tail != q_head) {
        *pressed = key_queue[q_tail].pressed;
        *doomKey = key_queue[q_tail].key;
        q_tail = (q_tail + 1) % MAX_EVENTS;
        return 1;
    }
    return 0; 
}

void DG_SleepMs(uint32_t ms) { usleep(ms * 1000); }
uint32_t DG_GetTicksMs() {
    struct timeval tp; gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}
void DG_SetWindowTitle(const char * title) {}

int main(int argc, char **argv) {
    doomgeneric_Create(argc, argv);
    while (1) { doomgeneric_Tick(); }
    return 0;
}