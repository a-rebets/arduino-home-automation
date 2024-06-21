#include "main.h"
#include "hardware.h"
#include "general.h"

// Hardware
LiquidCrystal mainDisplay(12, 13, 11, 10, 9, 8);
Adafruit_NeoPixel strip(8, 4, NEO_GRB + NEO_KHZ800);
Adafruit_7segment clockDisplay = Adafruit_7segment();
unsigned long TIME_WHEEL_RANGE = getMillisFromHour(4);

// Rooms
RoomConfig room1Config(ROOM1_TEMP_SENSOR_PIN, ROOM1_HEATING_PIN, ROOM1_COOLING_PIN, ROOM1_PIR_PIN, ROOM1_LIGHT_STRIP_IND);
RoomConfig room2Config(ROOM2_TEMP_SENSOR_PIN, ROOM2_HEATING_PIN, ROOM2_COOLING_PIN, ROOM2_PIR_PIN, ROOM2_LIGHT_STRIP_IND);
RoomControl room1("Room 1", room1Config);
RoomControl room2("Room 2", room2Config);

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
