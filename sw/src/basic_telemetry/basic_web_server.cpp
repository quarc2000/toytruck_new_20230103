#include <basic_telemetry/basic_web_server.h>

#include <WiFi.h>
#include <WebServer.h>

#include <basic_telemetry/basic_logger.h>
#include <variables/setget.h>

namespace {
constexpr char BASIC_AP_PASSWORD[] = "truckdebug";
constexpr char BASIC_AP_SSID[] = "ToyTruckDebug";

WebServer server(80);

String htmlEscape(const String &text)
{
    String out;
    for (size_t i = 0; i < text.length(); ++i) {
        const char c = text[i];
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out += c; break;
        }
    }
    return out;
}
} // namespace

BasicWebServerService::BasicWebServerService() : apMode(false) {}

void BasicWebServerService::Begin(const String &chipId)
{
    this->chipId = chipId;
    startWifi();
    registerRoutes();
    server.begin();
    basic_log_info("Web server started at http://" + getIpLabel() + "/");
}

void BasicWebServerService::Loop()
{
    server.handleClient();
}

void BasicWebServerService::startWifi()
{
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    const bool apStarted = WiFi.softAP(BASIC_AP_SSID, BASIC_AP_PASSWORD, 1, 0, 4);
    apMode = apStarted;
    if (apStarted) {
        const String apMsg = "Debug AP '" + String(BASIC_AP_SSID) + "' at " + WiFi.softAPIP().toString();
        basic_log_info(apMsg);
        Serial.println(apMsg);
    } else {
        basic_log_error("Failed to start debug AP");
        Serial.println("Failed to start debug AP");
    }
}

void BasicWebServerService::registerRoutes()
{
    server.on("/", [this]() {
        server.send(200, "text/html", renderIndexPage());
    });

    server.on("/status", [this]() {
        server.send(200, "application/json", renderStatusJson());
    });

    server.on("/logs", []() {
        server.send(200, "text/plain", basic_log_render_text());
    });
}

String BasicWebServerService::renderIndexPage() const
{
    String html;
    html.reserve(5000);
    html += "<!doctype html><html><head><meta charset='utf-8'>";
    html += "<meta http-equiv='refresh' content='2'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Truck Debug</title>";
    html += "<style>";
    html += "body{font-family:Verdana,sans-serif;background:#f4f1ea;color:#1f2a2e;margin:0;padding:24px;}";
    html += "h1,h2{margin:0 0 12px;}section{background:#fff;border:1px solid #d5d0c6;border-radius:12px;padding:16px;margin:0 0 16px;}";
    html += "table{border-collapse:collapse;width:100%;}th,td{border-bottom:1px solid #e6e0d6;padding:8px;text-align:left;}";
    html += "code{background:#f1eee7;padding:2px 4px;border-radius:4px;}ul{padding-left:20px;margin:0;}a{color:#7a3d00;}";
    html += "</style></head><body>";
    html += "<h1>Truck Debug</h1>";
    html += "<p>Mode: <code>" + htmlEscape(getModeLabel()) + "</code> | IP: <code>" + htmlEscape(getIpLabel()) + "</code> | Chip: <code>" + htmlEscape(chipId) + "</code></p>";

    html += "<section><h2>Selected Variables</h2><table><tr><th>Name</th><th>Value</th></tr>";
    html += "<tr><td>rawDistFront</td><td>" + String(globalVar_get(rawDistFront)) + "</td></tr>";
    html += "<tr><td>rawDistBack</td><td>" + String(globalVar_get(rawDistBack)) + "</td></tr>";
    html += "<tr><td>rawAccX</td><td>" + String(globalVar_get(rawAccX)) + "</td></tr>";
    html += "<tr><td>rawAccY</td><td>" + String(globalVar_get(rawAccY)) + "</td></tr>";
    html += "<tr><td>rawAccZ</td><td>" + String(globalVar_get(rawAccZ)) + "</td></tr>";
    html += "<tr><td>cleanedAccX</td><td>" + String(globalVar_get(cleanedAccX)) + "</td></tr>";
    html += "<tr><td>cleanedAccY</td><td>" + String(globalVar_get(cleanedAccY)) + "</td></tr>";
    html += "<tr><td>cleanedAccZ</td><td>" + String(globalVar_get(cleanedAccZ)) + "</td></tr>";
    html += "<tr><td>cleanedGyZ</td><td>" + String(globalVar_get(cleanedGyZ)) + "</td></tr>";
    html += "<tr><td>calcHeading</td><td>" + String(globalVar_get(calcHeading)) + "</td></tr>";
    html += "<tr><td>fuseForwardClear</td><td>" + String(globalVar_get(fuseForwardClear)) + "</td></tr>";
    html += "<tr><td>steerDirection</td><td>" + String(globalVar_get(steerDirection)) + "</td></tr>";
    html += "<tr><td>driver_desired_speed</td><td>" + String(globalVar_get(driver_desired_speed)) + "</td></tr>";
    html += "</table></section>";

    html += "<section><h2>Recent Logs</h2>";
    html += basic_log_render_html();
    html += "<p><a href='/logs'>Plain text logs</a> | <a href='/status'>JSON status</a></p>";
    html += "</section></body></html>";
    return html;
}

String BasicWebServerService::renderStatusJson() const
{
    String json = "{";
    json += "\"mode\":\"" + getModeLabel() + "\",";
    json += "\"ip\":\"" + getIpLabel() + "\",";
    json += "\"chip\":\"" + chipId + "\",";
    json += "\"rawDistFront\":" + String(globalVar_get(rawDistFront)) + ",";
    json += "\"rawDistBack\":" + String(globalVar_get(rawDistBack)) + ",";
    json += "\"rawAccX\":" + String(globalVar_get(rawAccX)) + ",";
    json += "\"rawAccY\":" + String(globalVar_get(rawAccY)) + ",";
    json += "\"rawAccZ\":" + String(globalVar_get(rawAccZ)) + ",";
    json += "\"cleanedAccX\":" + String(globalVar_get(cleanedAccX)) + ",";
    json += "\"cleanedAccY\":" + String(globalVar_get(cleanedAccY)) + ",";
    json += "\"cleanedAccZ\":" + String(globalVar_get(cleanedAccZ)) + ",";
    json += "\"cleanedGyZ\":" + String(globalVar_get(cleanedGyZ)) + ",";
    json += "\"calcHeading\":" + String(globalVar_get(calcHeading)) + ",";
    json += "\"fuseForwardClear\":" + String(globalVar_get(fuseForwardClear)) + ",";
    json += "\"steerDirection\":" + String(globalVar_get(steerDirection)) + ",";
    json += "\"driver_desired_speed\":" + String(globalVar_get(driver_desired_speed));
    json += "}";
    return json;
}

String BasicWebServerService::getModeLabel() const
{
    return apMode ? "AP" : "OFF";
}

String BasicWebServerService::getIpLabel() const
{
    return apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}
