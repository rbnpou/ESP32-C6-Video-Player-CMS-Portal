#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

// Configuración de tiempos
const int CLICK_TIMEOUT = 400; // Tiempo de espera para detectar múltiples clics

// Variables de estado
volatile int clickCount = 0;
unsigned long lastClickTime = 0;
bool actionPending = false;

// Prototipos de funciones que el .ino principal debe implementar
void onSingleClick();
void onDoubleClick();
void onTripleClick();

// Interrupción interna
void IRAM_ATTR buttonISR() {
    clickCount++;
    lastClickTime = millis();
    actionPending = true;
}

void initButton(int pin) {
    pinMode(pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin), buttonISR, FALLING);
}

// Esta función se llama en el loop() principal
void checkButton() {
    if (actionPending && (millis() - lastClickTime > CLICK_TIMEOUT)) {
        int finalClicks = clickCount;
        
        // Reset de variables antes de ejecutar para evitar bucles
        clickCount = 0;
        actionPending = false;

        // Ejecutar acción según número de clics
        if (finalClicks == 1) {
            onSingleClick();
        } 
        else if (finalClicks == 2) {
            onDoubleClick();
        } 
        else if (finalClicks >= 3) {
            onTripleClick();
        }
    }
}

#endif