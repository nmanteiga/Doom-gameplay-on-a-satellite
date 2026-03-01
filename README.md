## How to execute
1. Tener 2 ordenadores conectados a la misma red (evitar firewalls)
2. Ver la ip de cada dispositivo
3. Poner ip del dispositivo satélite en el doomgeneric/doomgeneric_satellite.c (línea 56, parte entrecomillada)
4. Poner ip dispositivo tierra en doomgeneric/estacion_terrena.py (línea 17)
5. Abrir terminal en satélite y poner los comandos:

    Arch Linux:
    gcc -std=gnu99 -O0 -I . $(find . -maxdepth 1 -name "*.c" ! -name "doomgeneric_*.c" ! -name "i_sdl*.c" ! -name "i_allegro*.c") doomgeneric_satellite.c -o doom_satellite

    ./doom_satellite -iwad DOOM1.WAD

    MacOS:
    gcc -O2 -I . $(find . -maxdepth 1 -name "*.c" ! -name "doomgeneric_*.c" ! -name "i_sdl*.c" ! -name "i_allegro*.c") doomgeneric_satellite.c -o doom_satellite

    ./doom_satellite
6. Abrir terminal en dispositivo tierra (evitar Wayland, da problemas, mejor X11 (xfce)) y poner:
    python3 estacion_terrena.py
7. Disfrutar :)

Puedes hacerlo en el mismo ordenador, descomentando la línea 55 y comentando la 56 (adapta la ip) en el .c y en el .py poner la del dispositivo.

## Inspiration
Nos inspiramos en las misiones espaciales reales (como los CubeSats o los rovers de Marte), donde el ancho de banda es súper limitado, las baterías se agotan y no puedes simplemente mandar un vídeo en 4K por Wi-Fi. Queríamos simular esa angustia y ese desafío técnico de comunicarnos con el espacio, pero haciéndolo divertido y jugable.

## What it does
Hemos diseñado una arquitectura de software que simula un enlace satelital en órbita LEO (Low Earth Orbit). El sistema consta de un "Nodo Satelital" (que ejecuta el juego) y una "Estación Terrena" (que lo controla). Hemos planteado un escenario con dos limitaciones autoimpuestas:

Modo Enlace Nominal en HD (Simulando Banda S): El juego se ve normal, pero usamos un algoritmo propio que solo envía los píxeles que cambian. Si te quedas quieto, el consumo de datos cae a casi cero.

Modo Supervivencia en ASCII (Simulando telemetría UHF): Si simulamos que el satélite se queda sin batería o pierde señal, pulsamos la tecla 'm'. El satélite apaga el vídeo pesado y transforma el juego en caracteres de texto (estilo Matrix). El juego sigue siendo totalmente jugable pero gasta poquísimos datos.

Además, tenemos un panel de telemetría en la terminal que nos chiva en tiempo real cuántos bytes estamos ahorrando.

## How we built it
Juntamos C y Python mediante una arquitectura bidireccional usando Sockets UDP no bloqueantes. Decidimos usar UDP en lugar de TCP para priorizar la latencia cero (vital en un videojuego), asumiendo nosotros el control de los paquetes perdidos.

El Nodo Satelital (C + DoomGeneric): Compilado nativamente en arquitectura AArch64 para asegurar la viabilidad técnica, aquí programamos todo el motor de compresión y Edge Computing desde cero. Como el tamaño máximo de un paquete de red (MTU) no nos dejaba enviar un frame entero, creamos nuestro propio protocolo binario con cabeceras personalizadas (255 para I-Frames, 254 para Delta, 253 para Fin de Trama). Compresión HD por Macrobloques: Dividimos la pantalla en cuadrículas de 8x8 píxeles. El C compara la memoria del frame actual con el anterior; si un bloque no ha cambiado, no viaja por la antena. Detección de Bloques Sólidos (RLE): Para exprimir el ancho de banda, le enseñamos al algoritmo a detectar zonas de color liso (como techos o pasillos). Si un bloque de 8x8 es sólido, activamos un bit especial en la cabecera y lo comprimimos en tan solo 3 bytes en lugar de 34. Procesamiento ASCII: Para el "Modo Supervivencia", el satélite escanea los valores de iluminación de la matriz, los mapea matemáticamente a una paleta estable de 9 niveles de gris (caracteres) para evitar parpadeos visuales, y envía un Delta de texto puro.

La Estación Terrena (Python): Actúa como decodificador en tiempo real y centro de control. Ensamblaje y Telemetría: Recibe los paquetes UDP desordenados, lee nuestras cabeceras para colocar cada macrobloque en su sitio de la matriz (numpy), e inyecta la imagen en OpenCV. Además, procesa los metadatos de red para calcular medias móviles y mostrar un dashboard de telemetría por terminal. Uplink de Control Multihilo: Para mover al marine, no podíamos usar el control básico de la ventana de vídeo. Usamos la librería pynput para capturar eventos asíncronos de "tecla pulsada" (estado 1) y "tecla soltada" (estado 0). Empaquetamos esto en bytes y se lo disparamos al satélite, logrando un movimiento diagonal totalmente fluido y profesional.

Tolerancia a Fallos (Resiliencia): Al usar UDP, si hay un microcorte Wi-Fi el juego se llenaría de fallos gráficos permanentes. Lo solucionamos implementando "I-Frames" dinámicos. Le dijimos al código que cada 30 frames (1 segundo), o si detecta que la compresión Delta va a ocupar demasiado, inyecte un frame completo. Así, ante cualquier caída de red, el sistema se auto-repara en menos de un segundo sin que el juego se cuelgue.

## Challenges we ran into
Al principio, intentamos mandar la imagen entera y la red de nuestros ordenadores colapsaba porque los paquetes eran demasiado grandes.

Luego, al trocear la imagen, tuvimos un bug masivo de rendimiento. Como Python recibía la imagen en 15 trocitos distintos, intentaba repintar la pantalla 15 veces por cada frame. Frió nuestros procesadores y el juego iba a cámara lenta. Tuvimos que inventar un "Marcador de Fin de Frame" (un pequeño paquete con el número 253) para decirle a la Tierra: "Oye, ya he terminado de enviarte los trozos, ahora sí, pinta la pantalla".

Además, también tuvimos problemas configurando el Doom y los controles, que al principio no iban; tuvimos que usar la librería pynput para usar el teclado de forma global.

También nos volvimos locos a las tantas de la madrugada calculando mal la media de ancho de banda en la terminal... nos salían picos de 200.000 bytes hasta que nos dimos cuenta de que estábamos sumando los datos dos veces en el bucle.

## Accomplishments that we're proud of
Estamos en general muy orgullosos del proyecto completo, pero sobre todo de los resultados matemáticos: al final de nuestra misión de prueba, el reporte final de telemetría demostró que pasamos de enviar 64.000 bytes por frame a una media de 9.520 bytes. Eso es un 85% de ahorro de ancho de banda manteniendo la jugabilidad a 30 FPS.

También estamos súper orgullosos de lo visual que es el proyecto. Pasar del modo ventana en HD a la terminal verde en ASCII con solo pulsar un botón (la tecla 'm'), y ver cómo los bytes de consumo se desploman en vivo en el panel, es una demostración técnica que parece magia.
