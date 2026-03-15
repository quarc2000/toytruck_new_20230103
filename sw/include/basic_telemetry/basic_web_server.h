#ifndef BASIC_TELEMETRY_BASIC_WEB_SERVER_H
#define BASIC_TELEMETRY_BASIC_WEB_SERVER_H

#include <Arduino.h>

class BasicWebServerService {
public:
    BasicWebServerService();
    void Begin(const String &chipId);
    void Loop();

private:
    void startWifi();
    void registerRoutes();
    String renderIndexPage() const;
    String renderStatusJson() const;
    String getModeLabel() const;
    String getIpLabel() const;

    String chipId;
    bool apMode;
};

#endif
