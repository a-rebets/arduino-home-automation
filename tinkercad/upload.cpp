#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

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

// Enum for the system state
enum SystemState {
    WELCOME_SCREEN,
    ROOM_MENU,
    ROOM_LIGHT_CONTROL,
    ROOM_TEMP_CONTROL,
    ROOM_SCHEDULE
};

enum ACState {
    OFF,
    HEATING,
    COOLING
};

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

// general
void displayWelcomeScreen();
void handleWelcomeScreen();
void displayCurrentMenu();
void handleCurrentMenu();
void displayCurrentTime();
void updateStartTime();

// helper methods
class StateStack {
private:
    static const int MAX_STACK_SIZE = 10;
    SystemState stack[MAX_STACK_SIZE];
    int top;

public:
    StateStack();

    void push(SystemState state);
    void pop();
    SystemState topState() const;
    bool isHistoryAvailable() const;
    int size() const;
};

void printTemperature(float temp);
void printCentered(const char* text, int row);
void PCF8574_Write(byte data);
void setExpanderPin(int pin, bool state);
String getTimestamp();
unsigned long currentTime();
unsigned long getMillisFromHour(int hour);
int hour();
int minute();
int mapOutdoorLighting(int lightReading);

// Hardware
LiquidCrystal mainDisplay(12, 13, 11, 10, 9, 8);
Adafruit_NeoPixel strip(8, 4, NEO_GRB + NEO_KHZ800);
Adafruit_7segment clockDisplay = Adafruit_7segment();

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

StateStack stateStack;
SystemState currentState = WELCOME_SCREEN;
RoomConfig room1Config(ROOM1_TEMP_SENSOR_PIN, ROOM1_HEATING_PIN, ROOM1_COOLING_PIN, ROOM1_PIR_PIN, ROOM1_LIGHT_STRIP_IND);
RoomConfig room2Config(ROOM2_TEMP_SENSOR_PIN, ROOM2_HEATING_PIN, ROOM2_COOLING_PIN, ROOM2_PIR_PIN, ROOM2_LIGHT_STRIP_IND);
RoomControl room1("Room 1", room1Config);
RoomControl room2("Room 2", room2Config);
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
const unsigned long TIME_WHEEL_RANGE = getMillisFromHour(4);
unsigned long START_TIME = getMillisFromHour(START_HOUR);
unsigned long ADDED_TIME = 0;
int lastTimeWheelValue = 0;

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

void displayWelcomeScreen() {
    room1.isDisplayed = false;
    room2.isDisplayed = false;
    printCentered(" Welcome Artem! ", 0);
    printCentered("<Room 1  Room 2>", 1);
}

void handleWelcomeScreen() {
    if (leftButtonPressed) {
        room1.display();
        mainDisplay.clear();
        delay(200);
    } else if (rightButtonPressed) {
        room2.display();
        mainDisplay.clear();
        delay(200);
    }
}

void displayCurrentMenu() {
    RoomControl& room = (room1.isDisplayed) ? room1 : room2;
    switch (currentState) {
    case WELCOME_SCREEN:
        displayWelcomeScreen();
        break;
    case ROOM_MENU:
        room.displayRoomMenu();
        break;
    case ROOM_LIGHT_CONTROL:
        room.displayRoomLightControl();
        break;
    case ROOM_TEMP_CONTROL:
        room.displayRoomTempControl();
        break;
    case ROOM_SCHEDULE:
        room.displayRoomSchedule();
        break;
    }
}

void handleCurrentMenu() {
    RoomControl& room = (room1.isDisplayed) ? room1 : room2;
    switch (currentState) {
    case WELCOME_SCREEN:
        handleWelcomeScreen();
        break;
    case ROOM_MENU:
        room.handleRoomMenu();
        break;
    case ROOM_LIGHT_CONTROL:
        room.handleRoomLightControl();
        break;
    case ROOM_TEMP_CONTROL:
        room.handleRoomTempControl();
        break;
    case ROOM_SCHEDULE:
        room.handleRoomSchedule();
        break;
    }
}

void displayCurrentTime() {
    // don't execute if not within one sec from minute change
    if ((currentTime() % 60000) > 1000 && !timeAdjusted) {
        return;
    }
    clockDisplay.clear();

    int currentHour = hour();
    int currentMinute = minute();

    char timeStr[5]; // HHMM format
    sprintf(timeStr, "%02d%02d", currentHour, currentMinute);

    clockDisplay.drawColon(true);
    clockDisplay.println(timeStr);
    clockDisplay.writeDisplay();

    timeAdjusted = false;
}

void updateStartTime() {
    unsigned long potValue = analogRead(TIME_WHEEL_PIN);
    if (potValue != lastTimeWheelValue) {
        ADDED_TIME = (unsigned long)((float)potValue / 1023.0 * TIME_WHEEL_RANGE);
        lastTimeWheelValue = potValue;
        timeAdjusted = true;
    }
}

void loop() {
    leftButtonPressed = !digitalRead(LEFT_BUTTON_PIN);
    rightButtonPressed = !digitalRead(RIGHT_BUTTON_PIN);
    backButtonPressed = !digitalRead(BACK_BUTTON_PIN);
    scheduleButtonPressed = !digitalRead(SCHEDULE_BUTTON_PIN);

    updateStartTime();

    if (backButtonPressed && stateStack.isHistoryAvailable()) {
        stateStack.pop();
        mainDisplay.clear();
        delay(200);
    }
    if (currentState != stateStack.topState() || room1.shouldUpdate() || room2.shouldUpdate()) {
        currentState = stateStack.topState();
        displayCurrentMenu();
        lightAdjusted = false;
        tempAdjusted = false;
        scheduleAdjusted = false;
    }
    handleCurrentMenu();
    displayCurrentTime();
}

void setup() {
    Serial.begin(9600);
    Wire.begin();

    clockDisplay.begin(CLOCK_ADDRESS);
    clockDisplay.setBrightness(15);

    mainDisplay.begin(16, 2);
    mainDisplay.createChar(0, solidBlock);
    mainDisplay.createChar(1, arrowUp);
    mainDisplay.createChar(2, arrowDown);

    pinMode(TIME_WHEEL_PIN, INPUT);
    pinMode(PHOTO_RESISTOR_PIN, INPUT);
    pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
    pinMode(BACK_BUTTON_PIN, INPUT_PULLUP);
    pinMode(SCHEDULE_BUTTON_PIN, INPUT_PULLUP);

    room1.init();
    room2.init();

    strip.begin();
    strip.show();

    stateStack.push(WELCOME_SCREEN);
    displayCurrentMenu();
}

// HELPER FUNCTIONS

StateStack::StateStack() : top(-1) {}

void StateStack::push(SystemState state) {
    if (top < MAX_STACK_SIZE - 1) {
        stack[++top] = state;
    }
}

void StateStack::pop() {
    if (top >= 0) {
        top--;
    }
}

SystemState StateStack::topState() const {
    if (top >= 0) {
        return stack[top];
    }
    return WELCOME_SCREEN;
}

bool StateStack::isHistoryAvailable() const {
    return top > 0;
}

int StateStack::size() const {
    return top + 1;
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
