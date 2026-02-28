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
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8080);
    
    const char* ip_destino = "172.20.10.2"; 
    servaddr.sin_addr.s_addr = inet_addr(ip_destino);
    
    if (servaddr.sin_addr.s_addr == INADDR_NONE) {
        printf("❌ [CRÍTICO] Formato de IP inválido. El sistema colapsará.\n");
    }
}

// el juego
void DG_DrawFrame() { 
    // Volvemos a los 4000 bytes seguros
    uint8_t buffer_comprimido[4000];
    int indice = 0;
    
    // CORRECCIÓN FINAL: Pantalla completa. 
    // Saltamos de 8 en 8 para reducir 640x400 a 80x50.
    for (int y = 0; y < 400; y += 8) {
        for (int x = 0; x < 640; x += 8) {
            int pixel_original = (y * 640) + x; 
            buffer_comprimido[indice] = (DG_ScreenBuffer[pixel_original] >> 8) & 0xFF;
            indice++;
        }
    }
    
    int bytes_enviados = sendto(sockfd, buffer_comprimido, sizeof(buffer_comprimido), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    
    if (bytes_enviados < 0) {
        perror("❌ ERROR en antena de bajada"); 
    } else {
        printf("✅ Frame transmitido (%d bytes)...\n", bytes_enviados);
    }
    
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