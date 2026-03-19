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

    server.on("/mapview", [this]() {
        server.send(200, "text/html", renderMapPage());
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
    html.reserve(2600);
    html += "<!doctype html><html><head><meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Truck Explorer</title>";
    html += "<style>";
    html += "body{font-family:Verdana,sans-serif;background:#f4f1ea;color:#1f2a2e;margin:0;padding:24px;}";
    html += "section{background:#fff;border:1px solid #d5d0c6;border-radius:12px;padding:16px;margin:0 0 16px;}";
    html += "a.button{display:inline-block;margin:0 12px 12px 0;padding:10px 14px;background:#2f6f73;color:#fff;text-decoration:none;border-radius:8px;}";
    html += "code{background:#f1eee7;padding:2px 4px;border-radius:4px;}";
    html += "</style></head><body>";
    html += "<h1>Truck Explorer</h1>";
    html += "<p>Mode: <code>" + htmlEscape(getModeLabel()) + "</code> | IP: <code>" + htmlEscape(getIpLabel()) + "</code> | Chip: <code>" + htmlEscape(chipId) + "</code></p>";
    html += "<section><h2>Explorer Summary</h2>";
    html += explorer != nullptr ? explorer->renderSummaryHtml() : "<p>No explorer.</p>";
    html += "<p><a class='button' href='/mapview'>Live Map</a><a class='button' href='/status'>JSON Status</a><a class='button' href='/map'>JSON Map</a><a class='button' href='/control'>JSON Control</a><a class='button' href='/logs'>Logs</a></p>";
    html += "</section>";
    html += "<section><h2>Live Map</h2><p>Open the dedicated map page for 1 second auto-refresh, robot position, and heading overlay.</p><p><a href='/mapview'>Go to live map page</a></p></section>";
    html += "</body></html>";
    return html;
}

String ExploreWebServerService::renderMapPage() const
{
    String html;
    html.reserve(8200);
    html += "<!doctype html><html><head><meta charset='utf-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>Truck Explorer Map</title>";
    html += "<style>";
    html += "body{font-family:Verdana,sans-serif;background:#f4f1ea;color:#1f2a2e;margin:0;padding:18px;}";
    html += "section{background:#fff;border:1px solid #d5d0c6;border-radius:12px;padding:16px;margin:0 0 16px;}";
    html += "canvas{display:block;border:1px solid #d5d0c6;background:#fbf9f4;image-rendering:pixelated;width:min(94vw,900px);height:min(94vw,900px);margin:0 auto;}";
    html += ".top{display:flex;flex-wrap:wrap;gap:12px;align-items:center;justify-content:space-between;}";
    html += ".status{display:flex;flex-wrap:wrap;gap:10px;font-size:14px;}";
    html += ".pill{background:#f1eee7;border-radius:999px;padding:6px 10px;}";
    html += ".legend{display:flex;flex-wrap:wrap;gap:12px;font-size:14px;}";
    html += ".swatch{display:inline-block;width:12px;height:12px;border:1px solid #777;vertical-align:middle;margin-right:6px;}";
    html += "a{color:#2f6f73;}";
    html += "</style></head><body>";
    html += "<div class='top'><h1>Observed Map</h1><p><a href='/'>Back to front page</a></p></div>";
    html += "<section><div class='status'>";
    html += "<span class='pill'>Mode: <strong>" + htmlEscape(getModeLabel()) + "</strong></span>";
    html += "<span class='pill'>IP: <strong>" + htmlEscape(getIpLabel()) + "</strong></span>";
    html += "<span class='pill'>Refresh: <strong>1 s</strong></span>";
    html += "<span class='pill' id='robotPose'>Robot: <strong>loading</strong></span>";
    html += "<span class='pill' id='robotHeading'>Heading: <strong>loading</strong></span>";
    html += "<span class='pill' id='robotState'>State: <strong>loading</strong></span>";
    html += "</div></section>";
    html += "<section><canvas id='map' width='900' height='900'></canvas></section>";
    html += "<section><div class='legend'>";
    html += "<span><span class='swatch' style='background:#fbf9f4'></span>Unknown</span>";
    html += "<span><span class='swatch' style='background:#d8e6cf'></span>Known free</span>";
    html += "<span><span class='swatch' style='background:#86c06c'></span>Visited</span>";
    html += "<span><span class='swatch' style='background:#9ca3af'></span>Suspect</span>";
    html += "<span><span class='swatch' style='background:#2f2f2f'></span>Blocked</span>";
    html += "<span><span class='swatch' style='background:#cc4b00'></span>Robot</span>";
    html += "</div></section>";
    html += "<script>";
    html += "const canvas=document.getElementById('map');const ctx=canvas.getContext('2d');";
    html += "const robotPose=document.getElementById('robotPose');const robotHeading=document.getElementById('robotHeading');const robotState=document.getElementById('robotState');";
    html += "function colorFor(ch){if(ch=='#')return '#2f2f2f';if(ch=='~')return '#9ca3af';if(ch=='v')return '#86c06c';if(ch=='.')return '#d8e6cf';return '#fbf9f4';}";
    html += "function drawRobot(robot,view){if(!robot)return;const cw=canvas.width/view.w;const ch=canvas.height/view.h;const cx=((robot.x-view.minX)+0.5)*cw;const cy=((robot.y-view.minY)+0.5)*ch;const radius=Math.max(10,Math.min(cw,ch)*0.35);ctx.fillStyle='#cc4b00';ctx.beginPath();ctx.arc(cx,cy,radius,0,Math.PI*2);ctx.fill();ctx.strokeStyle='#ffffff';ctx.lineWidth=Math.max(2,radius*0.35);ctx.beginPath();ctx.arc(cx,cy,radius+2,0,Math.PI*2);ctx.stroke();const angle=(robot.heading/10-90)*Math.PI/180;ctx.strokeStyle='#102a43';ctx.lineWidth=Math.max(3,radius*0.35);ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(cx+Math.cos(angle)*radius*2.2,cy+Math.sin(angle)*radius*2.2);ctx.stroke();}";
    html += "function stateLabel(v){if(v===0)return 'idle';if(v===1)return 'forward';if(v===2)return 'recover_stop';if(v===3)return 'recover_reverse';if(v===4)return 'recover_pause';if(v===5)return 'finished';return String(v);}";
    html += "function computeView(m){let minX=m.robot.x,maxX=m.robot.x,minY=m.robot.y,maxY=m.robot.y;for(let y=0;y<m.height;y++){const row=m.rows[y];for(let x=0;x<m.width;x++){const ch=row[x];if(ch!='?'){if(x<minX)minX=x;if(x>maxX)maxX=x;if(y<minY)minY=y;if(y>maxY)maxY=y;}}}const pad=6;minX=Math.max(0,minX-pad);minY=Math.max(0,minY-pad);maxX=Math.min(m.width-1,maxX+pad);maxY=Math.min(m.height-1,maxY+pad);const w=Math.max(12,maxX-minX+1);const h=Math.max(12,maxY-minY+1);return {minX,minY,maxX,maxY,w,h};}";
    html += "async function refresh(){try{const r=await fetch('/map',{cache:'no-store'});const m=await r.json();if(!m.rows||!m.robot)return;const view=computeView(m);const cw=canvas.width/view.w;const ch=canvas.height/view.h;ctx.clearRect(0,0,canvas.width,canvas.height);for(let y=view.minY;y<=view.maxY;y++){const row=m.rows[y];for(let x=view.minX;x<=view.maxX;x++){const chv=row[x];ctx.fillStyle=colorFor(chv);ctx.fillRect((x-view.minX)*cw,(y-view.minY)*ch,cw,ch);}}ctx.strokeStyle='rgba(80,80,80,0.18)';ctx.lineWidth=1;for(let x=0;x<=view.w;x++){ctx.beginPath();ctx.moveTo(x*cw,0);ctx.lineTo(x*cw,canvas.height);ctx.stroke();}for(let y=0;y<=view.h;y++){ctx.beginPath();ctx.moveTo(0,y*ch);ctx.lineTo(canvas.width,y*ch);ctx.stroke();}drawRobot(m.robot,view);robotPose.innerHTML='Robot: <strong>'+m.robot.x+', '+m.robot.y+'</strong>';robotHeading.innerHTML='Heading: <strong>'+(m.robot.heading/10).toFixed(1)+' deg</strong>';robotState.innerHTML='State: <strong>'+stateLabel(m.state)+'</strong>'; }catch(e){robotState.innerHTML='State: <strong>map fetch failed</strong>';}}";
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
