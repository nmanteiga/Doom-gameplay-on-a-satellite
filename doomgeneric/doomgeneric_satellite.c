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
#include <string.h> // para que funcione en arch    

// variables globales para el socket
int sockfd;
struct sockaddr_in servaddr;
int uplink_fd;
unsigned char tecla_en_memoria = 0;
int esperando_soltar = 0;

extern int key_up;
extern int key_down;
extern int key_left;
extern int key_right;
extern int key_fire;
extern int key_use;

// inicializar el sistema y el enlace de radio
void DG_Init() { 
    printf("\n[SATÉLITE] Sistemas online. Inicializando antena de bajada...\n"); 
    
    key_up = 'w';
    key_down = 's';
    key_left = 'a';
    key_right = 'd';
    key_fire = 'f'; 
    key_use = 'e';  

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8080);
    
    const char* ip_destino = "127.0.0.1"; 
    servaddr.sin_addr.s_addr = inet_addr(ip_destino);
    
    uplink_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in recvaddr;
    memset(&recvaddr, 0, sizeof(recvaddr));
    recvaddr.sin_family = AF_INET;
    recvaddr.sin_port = htons(8081); 
    recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    bind(uplink_fd, (struct sockaddr *)&recvaddr, sizeof(recvaddr));
    fcntl(uplink_fd, F_SETFL, O_NONBLOCK); 
    
    printf("\n[SATÉLITE] Sistemas online. Antenas operativas.\n");
}

// eljuego
// el juego
void DG_DrawFrame() { 
    uint8_t buffer_comprimido[64000];
    int indice = 0;
    
    for (int y = 0; y < 400; y += 2) {
        for (int x = 0; x < 640; x += 2) {
            int pixel_original = (y * 640) + x; 
            buffer_comprimido[indice] = (DG_ScreenBuffer[pixel_original] >> 8) & 0xFF;
            indice++;
        }
    }
    
    int tamaño_chunk = 8000;
    for (int i = 0; i < 8; i++) {
        int offset = i * tamaño_chunk; 

        sendto(sockfd, buffer_comprimido + offset, tamaño_chunk, 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    }
    
    printf("Frame transmitido en 8 fragmentos (64000 bytes)...\n");
    
    usleep(30000); 
}

// funciones de tiempo (para los mobs)
void DG_SleepMs(uint32_t ms) { 
    usleep(ms * 1000); 
}

uint32_t DG_GetTicksMs() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

// controles
unsigned char tecla_actual = 0;
uint32_t tiempo_ultima_tecla = 0;
unsigned char tecla_pendiente = 0;


int DG_GetKey(int* pressed, unsigned char* doomKey) { 
    if (tecla_pendiente != 0) {
        *pressed = 1;
        *doomKey = tecla_pendiente;
        tecla_actual = tecla_pendiente;
        tecla_pendiente = 0;
        tiempo_ultima_tecla = DG_GetTicksMs();
        return 1;
    }

    unsigned char tecla_recibida;
    unsigned char ultima_tecla = 0;
    int paquetes_leidos = 0;
    int n;

    while ((n = recvfrom(uplink_fd, &tecla_recibida, 1, 0, NULL, NULL)) > 0) {
        ultima_tecla = tecla_recibida;
        paquetes_leidos++;
    }

    if (paquetes_leidos > 0) {
        tiempo_ultima_tecla = DG_GetTicksMs(); 
        
        unsigned char tecla_final = ultima_tecla; 
        if (ultima_tecla == 13) tecla_final = KEY_ENTER; 

        if (tecla_final != tecla_actual) {
            if (tecla_actual != 0) {
                *pressed = 0;
                *doomKey = tecla_actual;
                tecla_pendiente = tecla_final; 
                tecla_actual = 0;
                return 1; 
            } else {
                *pressed = 1;
                *doomKey = tecla_final;
                tecla_actual = tecla_final;
                return 1;
            }
        }
        return 0; 
        
    } else {
        if (tecla_actual != 0) {
            if ((DG_GetTicksMs() - tiempo_ultima_tecla) > 150) {
                *pressed = 0; 
                *doomKey = tecla_actual;
                tecla_actual = 0;
                return 1;
            }
        }
    }
    return 0; 
}

// título de la ventana (vacío porque no hay ventana)
void DG_SetWindowTitle(const char * title) {}

int main(int argc, char **argv) {
    printf("[SATÉLITE] Arrancando motor DOOM...\n");
    doomgeneric_Create(argc, argv);
    
    while (1) { // bucle de juego 
        doomgeneric_Tick(); // calcula la física y llama a DG_DrawFrame
    }
    
    return 0;
}


/*
gcc -O2 -I . $(find . -maxdepth 1 -name "*.c" ! -name "doomgeneric_*.c" ! -name "i_sdl*.c" ! -name "i_allegro*.c") doomgeneric_satellite.c -o doom_satellite
./doom_satellite -iwad DOOM1.WAD
*/