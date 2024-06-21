#ifndef ENUMS_H
#define ENUMS_H

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

#endif // ENUMS_H