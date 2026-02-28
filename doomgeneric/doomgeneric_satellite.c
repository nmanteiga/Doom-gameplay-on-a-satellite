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

#define MAX_EVENTS 64
typedef struct { int pressed; unsigned char key; } KeyEvent;

// variables globales para el socket
int sockfd;
struct sockaddr_in servaddr;
int uplink_fd;
unsigned char tecla_en_memoria = 0;
int esperando_soltar = 0;

KeyEvent key_queue[MAX_EVENTS];
int q_head = 0;
int q_tail = 0;

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
    //const char* ip_destino = "172.20.10.7"; 
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
    uint8_t paquete_udp[8001]; 

    for (int i = 0; i < 8; i++) {
        paquete_udp[0] = i; // CABECERA DE TELEMETRÍA
        
        int offset = i * tamaño_chunk; 
        memcpy(&paquete_udp[1], buffer_comprimido + offset, tamaño_chunk);
        
        sendto(sockfd, paquete_udp, sizeof(paquete_udp), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
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
    int n;

    // Leemos TODOS los mensajes de red acumulados (2 bytes: Estado + Letra)
    while ((n = recvfrom(uplink_fd, net_cmd, 2, 0, NULL, NULL)) == 2) {
        int is_pressed = net_cmd[0];
        unsigned char raw_key = net_cmd[1];
        
        unsigned char tecla_final = raw_key;
        if (raw_key == 13) tecla_final = KEY_ENTER;
        if (raw_key == 27) tecla_final = 27; // ESC

        push_key(is_pressed, tecla_final);
    }

    // Le damos a DOOM la siguiente tecla de la cola
    if (q_tail != q_head) {
        *pressed = key_queue[q_tail].pressed;
        *doomKey = key_queue[q_tail].key;
        q_tail = (q_tail + 1) % MAX_EVENTS;
        return 1;
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