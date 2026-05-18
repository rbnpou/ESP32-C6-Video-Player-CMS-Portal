# ESP32-C6 Video Player & CMS Portal 🎬🔋

Este proyecto transforma un **ESP32-C6 con pantalla LCD de 1.47" de la marca WAVESHARE** (https://www.waveshare.com/esp32-c6-lcd-1.47.htm) en un reproductor de video MJPEG optimizado, que incluye un sistema de gestión de contenidos (CMS) inalámbrico y un conversor de video integrado en el navegador.

El módulo de reproducción de video es un port del trabajo de https://github.com/thelastoutpostworkshop.

## 🎯 Objetivo
El objetivo principal es reproducir contenido multimedia de alta calidad (320x240 @ 15fps) sin sobrecalentamiento, permitiendo al usuario gestionar los archivos y la lista de reproducción de forma remota a través de un **Portal Cautivo WiFi**, eliminando la necesidad de extraer la tarjeta SD para actualizar el contenido.

## 🚀 Funcionalidades Principales

* **Reproducción MJPEG Fluida:** Optimizado para funcionar a 50MHz, garantizando estabilidad térmica y eficiencia energética.
* **Portal Cautivo WiFi:** Al activar el modo WiFi, el dispositivo crea un punto de acceso que redirige automáticamente al navegador del smartphone o PC conectado.
* **Conversor de Video "Client-Side":** La interfaz web utiliza la potencia del dispositivo cliente (celular/PC) para convertir archivos MP4/MOV a formato MJPEG en tiempo real antes de subirlos.
* **Gestión de Playlist:** Permite seleccionar qué videos de la SD deben reproducirse y cuáles ocultar, guardando la configuración de forma persistente.
* **Explorador de Archivos:** Posibilidad de visualizar y eliminar archivos directamente desde la interfaz web.
* **Control Inteligente por Botón:**
    * **1 Clic:** Saltar al siguiente video.
    * **2 Clics:** Activar/Desactivar modo WiFi.
    * **3 Clics:** Apagado total (Deep Sleep).
    * **Despertar:** Una pulsación con el equipo apagado inicia el sistema instantáneamente.

## 🛠️ Arquitectura del Software

El proyecto se basa en una estructura modular para facilitar su mantenimiento:

1.  **`ESP32_C6_VideoPlayer.ino`**: Núcleo del programa. Gestiona la lógica de reproducción, los estados del sistema y la orquestación entre módulos.
2.  **`WiFiDrive.h`**: Servidor web, motor DNS para el Portal Cautivo y la lógica de subida/eliminación de archivos.
3.  **`ButtonHandler.h`**: Módulo especializado en la gestión de interrupciones del botón, conteo de clics y eliminación de rebotes (*debouncing*).
4.  **`MjpegClass.h`**: Decodificador de video de alto rendimiento.
5.  **`PINS_ESP32-C6-LCD-1_47.h`**: Definiciones de hardware específicas para la placa.

## 📖 Evolución y Uso del Archivo Original

Este proyecto evolucionó a partir de un reproductor de video MJPEG escrito por https://github.com/thelastoutpostworkshop. La integración fue la siguiente:
* **Optimización de Recursos:** Se modificó la frecuencia de reloj original y se implementó un manejo de buffers DMA para permitir que el ESP32-C6 maneje la pantalla de 1.47" sin lag.
* **Persistencia:** Se añadió el archivo `playlist.conf` en la raíz de la SD para que las selecciones realizadas en la interfaz web se mantengan tras reiniciar el equipo.
* **Gestión Térmica:** Se integró el apagado de radios (WiFi/BT) durante la reproducción para minimizar el ruido eléctrico y el calor residual.

## 🔧 Configuración de Compilación

Para compilar este proyecto en Arduino IDE, es necesario configurar:
* **Placa:** ESP32-C6 Dev Module.
* **Partition Scheme:** `Huge App (3MB No OTA/1MB SPIFFS)`. Esto es crítico debido al tamaño de la interfaz web integrada.
* **CPU Frequency:** `80MHz`.

---
*Desarrollado por @rbnpou con enfoque en la eficiencia y la experiencia de usuario (UX) en sistemas embebidos.*
