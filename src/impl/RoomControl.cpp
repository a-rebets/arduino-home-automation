#include "hardware.h"
#include "general.h"
#include "RoomControl.h"

void RoomControl::display() {
    isDisplayed = true;
    stateStack.push(ROOM_MENU);
}

void RoomControl::init() {
    pinMode(config.pirPin, INPUT);
}

void RoomControl::displayRoomMenu() {
    printCentered(name.c_str(), 0);
    mainDisplay.setCursor(0, 1);
    mainDisplay.print(F("<Light    Temp.>"));
}

void RoomControl::handleRoomMenu() {
    if (leftButtonPressed) {
        stateStack.push(ROOM_LIGHT_CONTROL);
        mainDisplay.clear();
        delay(200);
    } else if (rightButtonPressed) {
        stateStack.push(ROOM_TEMP_CONTROL);
        mainDisplay.clear();
        delay(200);
    }
}

void RoomControl::displayRoomTempControl() {
    mainDisplay.setCursor(0, 0);
    mainDisplay.print(name.c_str());
    mainDisplay.print(F(": "));
    mainDisplay.setCursor(9, 0);
    mainDisplay.print(currentTemp, 1);
    if (currentTemp != targetTemp) {
        mainDisplay.write(byte((targetTemp > currentTemp) ? 1 : 2));
    }
    mainDisplay.print(F("   "));
    printTemperature(targetTemp);
    mainDisplay.setCursor(0, 1);
    mainDisplay.print(F("-"));
    mainDisplay.setCursor(15, 1);
    mainDisplay.print(F("+"));
}

void RoomControl::handleRoomTempControl() {
    if (leftButtonPressed && targetTemp > 10) {
        targetTemp -= 0.5;
        tempAdjusted = true;
        delay(200);
    } else if (rightButtonPressed && targetTemp < 30) {
        targetTemp += 0.5;
        tempAdjusted = true;
        delay(200);
    }
}

void RoomControl::autoUpdateTemperature() {
    float temp = readTemperature();
    if (temp != currentTemp) {
        currentTemp = temp;
        tempAdjusted = true;
    }
    adjustAC();
}

float RoomControl::readTemperature() {
    int sensorValue = analogRead(config.tempSensorPin);
    float voltage = sensorValue * (5.0 / 1023.0);
    float temperatureC = (voltage - 0.5) * 100.0;
    return round(temperatureC * 10) / 10.0;
}

void RoomControl::adjustAC() {
    if (currentTemp < targetTemp && acState != HEATING) {
        setACState(HEATING);
    } else if (currentTemp > targetTemp && acState != COOLING) {
        setACState(COOLING);
    } else if (currentTemp == targetTemp && acState != OFF) {
        setACState(OFF);
    }
}

void RoomControl::setACState(ACState state) {
    switch (state) {
    case HEATING:
        setExpanderPin(config.heatingPin, HIGH);
        setExpanderPin(config.coolingPin, LOW);
        break;
    case COOLING:
        setExpanderPin(config.heatingPin, LOW);
        setExpanderPin(config.coolingPin, HIGH);
        break;
    case OFF:
        setExpanderPin(config.heatingPin, LOW);
        setExpanderPin(config.coolingPin, LOW);
        break;
    }
    acState = state;
}

void RoomControl::displayRoomLightControl() {
    char buffer[17];
    snprintf(buffer, sizeof(buffer), "%s Light", name.c_str());
    printCentered(buffer, 0);
    mainDisplay.setCursor(0, 1);
    mainDisplay.print(F("- "));
    int fullBlocks = lightIntensity * 12 / 4;
    for (int i = 0; i < 12; i++) {
        if (i < fullBlocks) {
            mainDisplay.write(byte(0));
        } else {
            mainDisplay.write(' ');
        }
    }
    mainDisplay.print(F(" +"));
}

void RoomControl::handleRoomLightControl() {
    if (leftButtonPressed && lightIntensity > 0) {
        --lightIntensity;
        updateNeoPixelBrightness(true);
        hourOverride = hour();
        delay(200);
    } else if (rightButtonPressed && lightIntensity < 4) {
        ++lightIntensity;
        updateNeoPixelBrightness(true);
        hourOverride = hour();
        delay(200);
    } else if (scheduleButtonPressed) {
        stateStack.push(ROOM_SCHEDULE);
        selectedHour = hour();
        delay(200);
    }
}

void RoomControl::updateNeoPixelBrightness(bool manual) {
    if (manual) {
        autoLightEnabled = false;
    }
    int startIndex = config.lightStripStartIndex;
    int endIndex = startIndex + 4;
    // clearing
    for (int i = startIndex; i < endIndex; i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
    // setting color
    for (int i = startIndex; i < startIndex + lightIntensity; i++) {
        strip.setPixelColor(i, strip.Color(0, 255, 0));
    }
    strip.show();
    lightAdjusted = true;

}

void RoomControl::autoAdjustLight() {
    int outdoorLightLevel = analogRead(PHOTO_RESISTOR_PIN);
    int targetIntensity = mapOutdoorLighting(outdoorLightLevel);
    if (autoLightEnabled && !scheduleActive && targetIntensity != lightIntensity) {
        lightIntensity = targetIntensity;
        updateNeoPixelBrightness(false);
    }
}

void RoomControl::displayRoomSchedule() {
    int nextHour = (selectedHour + 1) % 24;
    int scheduledLightIntensity = schedule[selectedHour];

    mainDisplay.clear();
    char buffer[17];

    if (scheduledLightIntensity == 0) {
        printCentered("[unset]", 0);
    } else {
        int percentage = scheduledLightIntensity * 25;
        sprintf(buffer, "[%d%% light]", percentage);
        printCentered(buffer, 0);
    }

    sprintf(buffer, "< %02d:00", selectedHour);
    mainDisplay.setCursor(0, 1);
    mainDisplay.print(buffer);

    sprintf(buffer, "%02d:00 >", nextHour);
    mainDisplay.setCursor(9, 1);
    mainDisplay.print(buffer);
}

void RoomControl::handleRoomSchedule() {
    if (leftButtonPressed) {
        selectedHour = (selectedHour == 0) ? 23 : selectedHour - 1;
        scheduleAdjusted = true;
        delay(200);
    } else if (rightButtonPressed) {
        selectedHour = (selectedHour + 1) % 24;
        scheduleAdjusted = true;
        delay(200);
    } else if (scheduleButtonPressed) {
        schedule[selectedHour] = lightIntensity;
        if (schedule[selectedHour] != 0) {
            scheduleActive = true;
        }
        scheduleAdjusted = true;
        delay(200);
    }
}

void RoomControl::checkSchedule() {
    int currentHour = hour();
    int scheduledLight = schedule[currentHour];
    bool shouldUpdate = scheduledLight != 0 && scheduledLight != lightIntensity;

    if (shouldUpdate && currentHour != hourOverride) {
        lightIntensity = scheduledLight;
        scheduleActive = true;
        updateNeoPixelBrightness(false);
    } else if (scheduledLight == 0 && scheduleActive) {
        deactivateSchedule();
    }
    if (currentHour != hourOverride && hourOverride != -1) {
        resetRoomOverride();
    }
}

void RoomControl::deactivateSchedule() {
    scheduleActive = false;
    lightIntensity = 0;
    updateNeoPixelBrightness(false);
}

void RoomControl::resetRoomOverride() {
    hourOverride = -1;
    inactive = false;
    peoplePresent = true;
    lastMotionTime = currentTime();
}

void RoomControl::detectRoomMotion(bool motionDetected) {
    unsigned long timeDiff = currentTime() - lastMotionTime;

    if (motionDetected && (timeDiff > 2000)) {
        lastMotionTime = currentTime();
        autoLightEnabled = true;
        peoplePresent = true;
    }
    if (peoplePresent && !scheduleActive && hour() != hourOverride) {
        handleInactivity();
    }
}

void RoomControl::handleInactivity() {
    unsigned long timeDiff = currentTime() - lastMotionTime;

    if (timeDiff > 20000 && inactive) { // 20 seconds of inactivity
        if (lightIntensity != 0) {
            lightIntensity = 0;
            updateNeoPixelBrightness(true);
        }
        peoplePresent = false;
        inactive = false;
    } else if (timeDiff > 15000 && !inactive) { // 15 seconds of inactivity
        if (targetTemp != 18.0) {
            targetTemp = 18.0;
            tempAdjusted = true;
        }
        if (lightIntensity > 0) {
            lightIntensity = (lightIntensity > 1) ? 1 : 0;
            updateNeoPixelBrightness(true);
        }
        inactive = true;
    }
}

bool RoomControl::shouldUpdate() {
    autoAdjustLight();
    autoUpdateTemperature();
    checkSchedule();
    detectRoomMotion(digitalRead(config.pirPin));
    if (lightAdjusted && (currentState == ROOM_LIGHT_CONTROL)) {
        return true;
    }
    if (tempAdjusted && (currentState == ROOM_TEMP_CONTROL)) {
        return true;
    }
    if (scheduleAdjusted && (currentState == ROOM_SCHEDULE)) {
        return true;
    }
    return false;
}