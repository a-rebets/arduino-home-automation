#ifndef ROOM_CONFIG_H
#define ROOM_CONFIG_H

class RoomConfig {
public:
    int tempSensorPin;
    int heatingPin;
    int coolingPin;
    int pirPin;
    int lightStripStartIndex;

    RoomConfig(int tempSensor, int heatPin, int coolPin, int pirSensor, int lightStripIndex)
        : tempSensorPin(tempSensor), heatingPin(heatPin), coolingPin(coolPin), pirPin(pirSensor), lightStripStartIndex(lightStripIndex) {}
};

#endif