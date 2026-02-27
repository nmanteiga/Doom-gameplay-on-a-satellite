#include "doomkeys.h"
#include "m_argv.h"
#include "doomgeneric.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>  
#include <arpa/inet.h>   
#include <stdint.h>      

// variables globales para el socket
int sockfd;
struct sockaddr_in servaddr;

// inicializar el sistema y el enlace de radio
void DG_Init() { 
    printf("\n[SATÉLITE] Sistemas online. Inicializando antena de bajada...\n"); 
    
    // Crear socket UDP
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8080);
    
    // !!! IMPORTANTE: PON AQUÍ LA IP DEL LINUX DE TU COMPAÑERO !!!
    // Si estáis probando los dos scripts en el mismo ordenador (el Mac), pon "127.0.0.1"
    servaddr.sin_addr.s_addr = inet_addr("10.20.38.13/21"); 
}

// el juego
void DG_DrawFrame() { 
    // creamos un buffer de 64.000 bytes (320 * 200 = 64000)
    uint8_t buffer_comprimido[64000];
    
    // DG_ScreenBuffer tiene el frame original en 32 bits.
    for (int i = 0; i < 64000; i++) {
        buffer_comprimido[i] = (DG_ScreenBuffer[i] >> 8) & 0xFF; 
    }
    
    // enviar el paquete a la Tierra
    sendto(sockfd, buffer_comprimido, sizeof(buffer_comprimido), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    
    printf("Frame transmitido (64 KB)...\n");
    usleep(30000); // Control de FPS
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
int DG_GetKey(int* pressed, unsigned char* doomKey) { 
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