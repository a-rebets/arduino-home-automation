#include "StateStack.h"

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
