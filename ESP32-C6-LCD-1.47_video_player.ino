/*
 * PROYECTO: Reproductor MJPEG con Gestión Web y Control de Energía
 * HARDWARE: ESP32-C6 + LCD 1.47" de la marca WAVESHARE
 * LIBRERÍAS: GFX Library for Arduino, MJPEG, SD, WiFi, WebServer
 * Por @rbnpou
 */

#include "PINS_ESP32-C6-LCD-1_47.h" 
#include "MjpegClass.h"             
#include "SD.h"                     
#include "Arduino.h"
#include <WiFi.h>                   

// Módulos personalizados
#include "WiFiDrive.h"  
#include "ButtonHandler.h"

// --- CONFIGURACIÓN ---
#define GFX_BRIGHTNESS 180          // Brillo de la pantalla (0-255)
#define MJPEG_FOLDER "/mjpeg"
#define MAX_FILES 30

// --- VARIABLES GLOBALES ---
MjpegClass mjpeg;
uint8_t *mjpeg_buf;
uint16_t *output_buf;
long output_buf_size, estimateBufferSize;

bool driveMode = false; 
bool skipRequested = false;
String mjpegFileList[MAX_FILES];
int mjpegCount = 0;
int currentMjpegIndex = 0;

// --- ACCIONES DEL MÓDULO DE BOTONES ---

void onSingleClick() {
    Serial.println("Acción: Siguiente Video");
    skipRequested = true;
}

void onDoubleClick() {
    if (driveMode) {
        Serial.println("Acción: Desactivar WiFi");
        stopWiFiDrive();
        driveMode = false;
        skipRequested = true;
        gfx->fillScreen(RGB565_BLACK);
    } else {
        Serial.println("Acción: Activar WiFi");
        driveMode = true;
        skipRequested = true;
        
        // Pantalla informativa
        gfx->fillScreen(RGB565_NAVY);
        gfx->setTextColor(RGB565_YELLOW);
        gfx->setTextSize(2);
        gfx->setCursor(10, 30); gfx->println("MODO WIFI");
        gfx->setTextSize(1);
        gfx->setTextColor(RGB565_WHITE);
        gfx->setCursor(10, 70); gfx->println("Red: ESP32-C6-DRIVE");
        gfx->setCursor(10, 90); gfx->println("URL: 192.168.4.1");
        
        startWiFiDrive(MJPEG_FOLDER);
    }
}

void onTripleClick() {
    Serial.println("Acción: Apagado (Deep Sleep)");
    gfx->fillScreen(RGB565_BLACK);
    gfx->setCursor(10, 60);
    gfx->setTextColor(RGB565_RED);
    gfx->println("APAGANDO...");
    delay(1000);
    ledcWrite(GFX_BL, 0); // Apagar luz de fondo
    esp_deep_sleep_start();
}

// --- FUNCIONES DE REPRODUCCIÓN Y CARGA ---

void loadMjpegFilesList() {
    mjpegCount = 0;
    // 1. Cargar Playlist desde la SD si existe
    if (SD.exists("/playlist.conf")) {
        File f = SD.open("/playlist.conf", FILE_READ);
        while (f.available() && mjpegCount < MAX_FILES) {
            String line = f.readStringUntil('\n');
            line.trim();
            if (line.length() > 0 && SD.exists(String(MJPEG_FOLDER) + "/" + line)) {
                mjpegFileList[mjpegCount] = line;
                mjpegCount++;
            }
        }
        f.close();
    }

    // 2. Si no hay selección, cargar todos los archivos de la carpeta
    if (mjpegCount == 0) {
        if (!SD.exists(MJPEG_FOLDER)) SD.mkdir(MJPEG_FOLDER);
        File mjpegDir = SD.open(MJPEG_FOLDER);
        while (true) {
            File file = mjpegDir.openNextFile();
            if (!file) break;
            if (!file.isDirectory() && String(file.name()).endsWith(".mjpeg")) {
                mjpegFileList[mjpegCount] = file.name();
                mjpegCount++;
            }
            file.close();
        }
        mjpegDir.close();
    }
}

int jpegDrawCallback(JPEGDRAW *pDraw) {
    gfx->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
    return 1;
}

void playSelectedMjpeg(int mjpegIndex) {
    if (driveMode) return;
    String fullPath = String(MJPEG_FOLDER) + "/" + mjpegFileList[mjpegIndex];
    File vFile = SD.open(fullPath, "r");
    
    if (vFile && !vFile.isDirectory()) {
        gfx->fillScreen(RGB565_BLACK);
        mjpeg.setup(&vFile, mjpeg_buf, jpegDrawCallback, true, 0, 0, gfx->width(), gfx->height());
        
        while (!skipRequested && vFile.available() && mjpeg.readMjpegBuf()) {
            mjpeg.drawJpg();
            checkButton(); // Revisar si el usuario pulsa mientras reproduce
            if (driveMode) break; 
            yield(); 
        }
        vFile.close();
        skipRequested = false; 
    }
}

// --- SETUP Y LOOP ---

void setup() {
    // Despertar con el botón
    esp_deep_sleep_enable_gpio_wakeup(1ULL << BTN_A, ESP_GPIO_WAKEUP_GPIO_LOW);

    setCpuFrequencyMhz(50); // Velocidad en MHZ del procesador para ahorro de energía y menos calor
    WiFi.mode(WIFI_OFF);
    btStop();

    Serial.begin(115200);
    DEV_DEVICE_INIT();

    // Pantalla
    if (!gfx->begin(GFX_SPEED)) while (1);
    gfx->setRotation(0);
    ledcAttachChannel(GFX_BL, 1000, 8, 1);
    ledcWrite(GFX_BL, GFX_BRIGHTNESS);

    // SD
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
    if (!SD.begin(SD_CS, SPI, 80000000, "/sd")) {
        gfx->println("Error SD");
        while (1);
    }

    // Memoria para MJPEG
    output_buf_size = gfx->width() * 4 * 2;
    output_buf = (uint16_t *)heap_caps_aligned_alloc(16, output_buf_size * sizeof(uint16_t), MALLOC_CAP_DMA);
    estimateBufferSize = gfx->width() * gfx->height() * 2 / 5;
    mjpeg_buf = (uint8_t *)heap_caps_malloc(estimateBufferSize, MALLOC_CAP_8BIT);

    loadMjpegFilesList();
    initButton(BTN_A); // Inicializar módulo de botones
}

void loop() {
    checkButton(); // Escanear pulsaciones

    if (driveMode) {
        handleWiFiDrive();
    } else {
        if (mjpegCount > 0) {
            playSelectedMjpeg(currentMjpegIndex);
            currentMjpegIndex = (currentMjpegIndex + 1) % mjpegCount;
        } else {
            gfx->setCursor(10, 60);
            gfx->println("No hay videos...");
            delay(1000);
            loadMjpegFilesList();
        }
    }
}