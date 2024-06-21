#ifndef ROOMCONTROL_H
#define ROOMCONTROL_H

#include <Arduino.h>
#include "enums.h"
#include "RoomConfig.h"

class RoomControl {
public:
    String name;
    RoomConfig config;
    float currentTemp = 0.0;
    float targetTemp = 22.0;
    int lightIntensity = 0;
    int selectedHour = 0;
    int hourOverride = -1;
    unsigned long lastMotionTime = 0;
    bool peoplePresent = false;
    bool inactive = false;
    bool scheduleActive = false;
    bool autoLightEnabled = false;
    bool isDisplayed = false;
    int schedule[24] = { 0 };
    ACState acState = OFF;
    SystemState menuStates[3];

    RoomControl(String roomName, RoomConfig roomConfig) : name(roomName), config(roomConfig) {}

    void display();
    void init();
    void displayRoomMenu();
    void handleRoomMenu();
    void displayRoomTempControl();
    void handleRoomTempControl();
    void autoUpdateTemperature();
    float readTemperature();
    void adjustAC();
    void setACState(ACState state);
    void displayRoomLightControl();
    void handleRoomLightControl();
    void updateNeoPixelBrightness(bool manual);
    void autoAdjustLight();
    void displayRoomSchedule();
    void handleRoomSchedule();
    void checkSchedule();
    void deactivateSchedule();
    void resetRoomOverride();
    void detectRoomMotion(bool motionDetected);
    void handleInactivity();
    bool shouldUpdate();
};

#endif // ROOMCONTROL_H