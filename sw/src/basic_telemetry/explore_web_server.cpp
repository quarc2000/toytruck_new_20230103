#include <basic_telemetry/explore_web_server.h>

#include <WiFi.h>
#include <WebServer.h>

#include <basic_telemetry/basic_logger.h>
#include <navigation/observed_explorer.h>

namespace
{
constexpr char BASIC_AP_PASSWORD[] = "truckdebug";
constexpr char BASIC_AP_SSID[] = "ToyTruckDebug";

WebServer server(80);

String htmlEscape(const String &text)
{
    String out;
    for (size_t i = 0; i < text.length(); ++i)
    {
        const char c = text[i];
        switch (c)
        {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out += c; break;
        }
    }
    return out;
}

bool parseIntStrict(const String &text, int32_t &value)
{
    if (text.length() == 0)
    {
        return false;
    }

    size_t i = 0;
    bool negative = false;
    if (text[0] == '-' || text[0] == '+')
    {
        negative = text[0] == '-';
        i = 1;
    }
    if (i >= text.length())
    {
        return false;
    }

    int32_t parsed = 0;
    for (; i < text.length(); ++i)
    {
        const char c = text[i];
        if (c < '0' || c > '9')
        {
            return false;
        }
        parsed = parsed * 10 + (c - '0');
    }
    value = negative ? -parsed : parsed;
    return true;
}
} // namespace

ExploreWebServerService::ExploreWebServerService() : apMode(false), explorer(nullptr)
{
}

void ExploreWebServerService::Begin(const String &chipId, ObservedExplorerService *explorer)
{
    this->chipId = chipId;
    this->explorer = explorer;
    startWifi();
    registerRoutes();
    server.begin();
    basic_log_info("Explore web server started at http://" + getIpLabel() + "/");
}

void ExploreWebServerService::Loop()
{
    server.handleClient();
}

void ExploreWebServerService::startWifi()
{
    WiFi.mode(WIFI_AP);
    WiFi.setSleep(false);
    apMode = WiFi.softAP(BASIC_AP_SSID, BASIC_AP_PASSWORD, 1, 0, 4);
}

void ExploreWebServerService::registerRoutes()
{
    server.on("/", [this]() {
        server.send(200, "text/html", renderIndexPage());
    });

    server.on("/status", [this]() {
        server.send(200, "application/json", explorer != nullptr ? explorer->renderStatusJson() : "{}");
    });

    server.on("/map", [this]() {
        server.send(200, "application/json", explorer != nullptr ? explorer->renderMapJson() : "{}");
    });

    server.on("/control", HTTP_GET, [this]() {
        server.send(200, "application/json", explorer != nullptr ? explorer->renderControlJson() : "{}");
    });

    server.on("/control/enable", HTTP_POST, [this]() {
        if (explorer == nullptr)
        {
            sendControlError(503, "Explorer unavailable");
            return;
        }
        int32_t timeoutMs = 0;
        if (server.hasArg("timeoutMs") && !readOptionalTimeoutMs(timeoutMs))
        {
            sendControlError(400, "Expected integer timeoutMs");
            return;
        }
        if (!explorer->enableRemoteControl(timeoutMs))
        {
            sendControlError(409, "Remote control could not be enabled");
            return;
        }
        server.send(200, "application/json", explorer->renderControlJson());
    });

    server.on("/control/drive", HTTP_POST, [this]() {
        if (explorer == nullptr)
        {
            sendControlError(503, "Explorer unavailable");
            return;
        }
        int32_t speed = 0;
        int32_t steerCommand = 0;
        int32_t timeoutMs = 0;
        if (!readIntArg("speed", speed) || !readIntArg("steer", steerCommand))
        {
            sendControlError(400, "Expected integer speed and steer parameters");
            return;
        }
        if (server.hasArg("timeoutMs") && !readOptionalTimeoutMs(timeoutMs))
        {
            sendControlError(400, "Expected integer timeoutMs");
            return;
        }
        if (!explorer->remoteDrive(speed, steerCommand, timeoutMs))
        {
            sendControlError(409, "Remote control is not active; enable it first");
            return;
        }
        server.send(200, "application/json", explorer->renderControlJson());
    });

    server.on("/control/stop", HTTP_POST, [this]() {
        if (explorer == nullptr)
        {
            sendControlError(503, "Explorer unavailable");
            return;
        }
        int32_t timeoutMs = 0;
        if (server.hasArg("timeoutMs") && !readOptionalTimeoutMs(timeoutMs))
        {
            sendControlError(400, "Expected integer timeoutMs");
            return;
        }
        if (!explorer->remoteStop(timeoutMs))
        {
            sendControlError(409, "Remote control is not active; enable it first");
            return;
        }
        server.send(200, "application/json", explorer->renderControlJson());
    });

    server.on("/control/disable", HTTP_POST, [this]() {
        if (explorer == nullptr)
        {
            sendControlError(503, "Explorer unavailable");
            return;
        }
        if (!explorer->disableRemoteControl())
        {
            sendControlError(409, "Remote control could not be disabled");
            return;
        }
        server.send(200, "application/json", explorer->renderControlJson());
    });

    server.on("/logs", []() {
        server.send(200, "text/plain", basic_log_render_text());
    });
}

String ExploreWebServerService::renderIndexPage() const
{
    String html;
    html.reserve(7000);
    html += "<!doctype html><html><head><meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Truck Explorer</title>";
    html += "<style>";
    html += "body{font-family:Verdana,sans-serif;background:#f4f1ea;color:#1f2a2e;margin:0;padding:24px;}";
    html += "section{background:#fff;border:1px solid #d5d0c6;border-radius:12px;padding:16px;margin:0 0 16px;}";
    html += "canvas{border:1px solid #d5d0c6;image-rendering:pixelated;width:min(90vw,700px);height:min(90vw,700px);}";
    html += "code{background:#f1eee7;padding:2px 4px;border-radius:4px;}";
    html += "</style></head><body>";
    html += "<h1>Truck Explorer</h1>";
    html += "<p>Mode: <code>" + htmlEscape(getModeLabel()) + "</code> | IP: <code>" + htmlEscape(getIpLabel()) + "</code> | Chip: <code>" + htmlEscape(chipId) + "</code></p>";
    html += "<section><h2>Explorer Summary</h2>";
    html += explorer != nullptr ? explorer->renderSummaryHtml() : "<p>No explorer.</p>";
    html += "<p><a href='/status'>JSON status</a> | <a href='/map'>JSON map</a> | <a href='/control'>JSON control</a> | <a href='/logs'>Logs</a></p>";
    html += "</section>";
    html += "<section><h2>Observed Map</h2><canvas id='map' width='700' height='700'></canvas></section>";
    html += "<script>";
    html += "const canvas=document.getElementById('map');const ctx=canvas.getContext('2d');";
    html += "function colorFor(ch){if(ch=='#')return '#2f2f2f';if(ch=='v')return '#86c06c';if(ch=='.')return '#d8e6cf';if(ch=='R')return '#cc4b00';return '#f6f3ee';}";
    html += "async function refresh(){const r=await fetch('/map');const m=await r.json();if(!m.rows)return;const cw=canvas.width/m.width;const ch=canvas.height/m.height;for(let y=0;y<m.height;y++){const row=m.rows[y];for(let x=0;x<m.width;x++){ctx.fillStyle=colorFor(row[x]);ctx.fillRect(x*cw,y*ch,cw,ch);}}}";
    html += "setInterval(refresh,1000);refresh();";
    html += "</script></body></html>";
    return html;
}

String ExploreWebServerService::getModeLabel() const
{
    return apMode ? "AP" : "OFF";
}

String ExploreWebServerService::getIpLabel() const
{
    return apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
}

bool ExploreWebServerService::readIntArg(const char *name, int32_t &value) const
{
    if (!server.hasArg(name))
    {
        return false;
    }
    return parseIntStrict(server.arg(name), value);
}

bool ExploreWebServerService::readOptionalTimeoutMs(int32_t &value) const
{
    if (!server.hasArg("timeoutMs"))
    {
        value = 0;
        return false;
    }
    return parseIntStrict(server.arg("timeoutMs"), value);
}

void ExploreWebServerService::sendControlError(int statusCode, const String &message) const
{
    String out = "{";
    out += "\"error\":\"" + htmlEscape(message) + "\"";
    out += "}";
    server.send(statusCode, "application/json", out);
}
