#include "Arduino.h"
#include "hardware.h"
#include "general.h"

unsigned long START_TIME = getMillisFromHour(START_HOUR);

void setExpanderPin(int pin, bool state) {
    if (state) {
        expanderPinStates |= (1 << pin);
    } else {
        expanderPinStates &= ~(1 << pin);
    }
    PCF8574_Write(expanderPinStates);
}

void PCF8574_Write(byte data) {
    Wire.beginTransmission(EXPANDER_ADDRESS);
    Wire.write(data);
    Wire.endTransmission();
}

int mapOutdoorLighting(int lightReading) {
    if (lightReading < 380) {
        return 4;
    } else {
        return map(lightReading, 380, 679, 3, 0);
    }
}

void printTemperature(float temp) {
    int integerPart = (int)temp;
    int fractionalPart = (int)((temp - integerPart) * 10);
    char displayStr[10];
    snprintf(displayStr, sizeof(displayStr), "%d.%d C", integerPart, fractionalPart);
    printCentered(displayStr, 1);
}

void printCentered(const char* text, int row) {
    int startPos = (16 - strlen(text)) / 2;
    mainDisplay.setCursor(startPos, row);
    mainDisplay.print(text);
}

String getTimestamp() {
    unsigned long millisec = currentTime();
    unsigned long hours = (millisec / 3600000) % 24;
    unsigned long mins = (millisec / 60000) % 60;
    unsigned long secs = (millisec / 1000) % 60;
    unsigned long ms = millisec % 1000;

    char formattedTime[13]; // Buffer to hold formatted time string
    sprintf(formattedTime, "%02lu:%02lu:%02lu.%03lu", hours, mins, secs, ms);

    return String(formattedTime);
}

unsigned long currentTime() {
    return (unsigned long)millis() + START_TIME + ADDED_TIME;
}

unsigned long getMillisFromHour(int hour) {
    return (unsigned long)hour * 60 * 60 * 1000;
}

int hour() {
    return (currentTime() / 3600000) % 24;
}

int minute() {
    return (currentTime() / 60000) % 60;
}
