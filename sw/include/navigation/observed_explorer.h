#ifndef NAVIGATION_OBSERVED_EXPLORER_H
#define NAVIGATION_OBSERVED_EXPLORER_H

#include <Arduino.h>
#include <navigation/grid_map.h>

class ObservedExplorerService
{
public:
    ObservedExplorerService();
    void Begin();
    bool enableRemoteControl(int32_t timeoutMs);
    bool disableRemoteControl();
    bool remoteDrive(int speed, int steer, int32_t timeoutMs);
    bool remoteStop(int32_t timeoutMs);

    String renderStatusJson() const;
    String renderMapJson() const;
    String renderControlJson() const;
    String renderSummaryHtml() const;

private:
    static void taskEntry(void *parameter);
    void runTask();
};

#endif
