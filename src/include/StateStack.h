#ifndef STATE_STACK_H
#define STATE_STACK_H

#include "enums.h"

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

#endif