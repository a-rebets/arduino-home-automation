#ifndef MAIN_H
#define MAIN_H

#include "RoomControl.h"

RoomConfig room1Config;
RoomConfig room2Config;
RoomControl room1;
RoomControl room2;

unsigned long TIME_WHEEL_RANGE = 0;
int lastTimeWheelValue = 0;

void displayWelcomeScreen();
void handleWelcomeScreen();
void displayCurrentMenu();
void handleCurrentMenu();
void displayCurrentTime();
void updateStartTime();

#endif