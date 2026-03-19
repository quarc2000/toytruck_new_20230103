#ifndef BASIC_TELEMETRY_EXPLORE_WEB_SERVER_H
#define BASIC_TELEMETRY_EXPLORE_WEB_SERVER_H

#include <Arduino.h>

class ObservedExplorerService;

class ExploreWebServerService
{
public:
    ExploreWebServerService();
    void Begin(const String &chipId, ObservedExplorerService *explorer);
    void Loop();

private:
    void startWifi();
    void registerRoutes();
    String renderIndexPage() const;
    String renderMapPage() const;
    String getModeLabel() const;
    String getIpLabel() const;
    bool readIntArg(const char *name, int32_t &value) const;
    bool readOptionalTimeoutMs(int32_t &value) const;
    void sendControlError(int statusCode, const String &message) const;

    String chipId;
    bool apMode;
    ObservedExplorerService *explorer;
};

#endif
