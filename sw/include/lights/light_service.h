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
    void applyOutputs(bool brakeOn, bool leftOn, bool rightOn, bool reverseOn);

    EXPANDER expander;
    bool blinkState;
    unsigned long lastBlinkToggleMs;
};

#endif
