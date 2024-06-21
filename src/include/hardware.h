#ifndef HARDWARE_H
#define HARDWARE_H

#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

LiquidCrystal mainDisplay;
Adafruit_NeoPixel strip;
Adafruit_7segment clockDisplay;

#define EXPANDER_ADDRESS 0x20
#define CLOCK_ADDRESS 0x70
#define LEFT_BUTTON_PIN 3
#define RIGHT_BUTTON_PIN 2
#define BACK_BUTTON_PIN 7
#define SCHEDULE_BUTTON_PIN A5
#define PHOTO_RESISTOR_PIN A2
#define TIME_WHEEL_PIN A3

// Room 1 pin definitions
#define ROOM1_TEMP_SENSOR_PIN A1
#define ROOM1_HEATING_PIN 0
#define ROOM1_COOLING_PIN 1
#define ROOM1_PIR_PIN 5
#define ROOM1_LIGHT_STRIP_IND 0

// Room 2 pin definitions
#define ROOM2_TEMP_SENSOR_PIN A0
#define ROOM2_HEATING_PIN 7
#define ROOM2_COOLING_PIN 6
#define ROOM2_PIR_PIN 6
#define ROOM2_LIGHT_STRIP_IND 4

// Custom characters for the LCD
byte solidBlock[8] = {
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111 };
byte arrowUp[8] = {
    B00100,
    B01110,
    B11111,
    B00100,
    B00100,
    B00100,
    B00100,
    B00000 };
byte arrowDown[8] = {
    B00100,
    B00100,
    B00100,
    B00100,
    B11111,
    B01110,
    B00100,
    B00000 };

#endif // HARDWARE_H