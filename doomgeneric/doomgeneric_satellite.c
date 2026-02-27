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
    
    
    //servaddr.sin_addr.s_addr = inet_addr("172.20.10.2/28"); 
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
}

// el juego
void DG_DrawFrame() { 
    // buffer enano de 4000 bytes (Resolución 80x50)
    uint8_t buffer_comprimido[4000];
    int indice = 0;
    
    // leemos 1 de cada 4 píxeles (Downsampling brutal para ahorrar datos)
    for (int y = 0; y < 200; y += 4) {
        for (int x = 0; x < 320; x += 4) {
            int pixel_original = (y * 320) + x;
            buffer_comprimido[indice] = (DG_ScreenBuffer[pixel_original] >> 8) & 0xFF;
            indice++;
        }
    }
    
    // Transmitimos a la Tierra y GUARDAMOS el resultado
    int bytes_enviados = sendto(sockfd, buffer_comprimido, sizeof(buffer_comprimido), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    
    // Comprobamos si el Mac ha matado el paquete
    if (bytes_enviados < 0) {
        perror("❌ ERROR en antena de bajada"); // Esto imprimirá el error real del sistema
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