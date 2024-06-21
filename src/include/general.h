#ifndef GENERAL_H
#define GENERAL_H

#include "StateStack.h"

StateStack stateStack;
SystemState currentState = WELCOME_SCREEN;
byte expanderPinStates = 0x00;

bool leftButtonPressed = false;
bool rightButtonPressed = false;
bool backButtonPressed = false;
bool scheduleButtonPressed = false;
bool lightAdjusted = false;
bool tempAdjusted = false;
bool scheduleAdjusted = false;
bool timeAdjusted = false;

const int START_HOUR = 8;
unsigned long START_TIME = 0;
unsigned long ADDED_TIME = 0;

void PCF8574_Write(byte data);
void setExpanderPin(int pin, bool state);
void printTemperature(float temp);
void printCentered(const char* text, int row);
String getTimestamp();
unsigned long currentTime();
unsigned long getMillisFromHour(int hour);
int hour();
int minute();
int mapOutdoorLighting(int lightReading);

#endif // GENERAL_H
