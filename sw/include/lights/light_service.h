#ifndef LIGHT_SERVICE_H
#define LIGHT_SERVICE_H

#include <Arduino.h>
#include <expander.h>

class LightService {
public:
    LightService();
    void Begin();

private:
    static void taskEntry(void *parameter);
    void runTask();
    void applyOutputs(bool mainOn, bool brakeOn, bool leftOn, bool rightOn, bool reverseOn);

    EXPANDER expander;
    bool begun;
    bool blinkState;
    unsigned long lastBlinkToggleMs;
    unsigned long brakeHoldUntilMs;
};

#endif
