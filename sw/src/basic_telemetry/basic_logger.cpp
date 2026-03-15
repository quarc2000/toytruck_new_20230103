#include <basic_telemetry/basic_logger.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace {
constexpr size_t BASIC_LOG_CAPACITY = 64;
constexpr size_t BASIC_LOG_LEVEL_LEN = 8;
constexpr size_t BASIC_LOG_MESSAGE_LEN = 120;

struct BasicLogEntry {
    unsigned long timestampMs;
    char level[BASIC_LOG_LEVEL_LEN];
    char message[BASIC_LOG_MESSAGE_LEN];
};

BasicLogEntry entries[BASIC_LOG_CAPACITY];
size_t nextIndex = 0;
size_t count = 0;
SemaphoreHandle_t logMutex = nullptr;

String htmlEscape(const char *text)
{
    String out;
    while (*text != '\0') {
        switch (*text) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        default: out += *text; break;
        }
        ++text;
    }
    return out;
}

void appendLog(const char *level, const String &msg)
{
    if (logMutex == nullptr) {
        basic_log_init();
    }

    xSemaphoreTake(logMutex, portMAX_DELAY);
    BasicLogEntry &entry = entries[nextIndex];
    entry.timestampMs = millis();
    strncpy(entry.level, level, BASIC_LOG_LEVEL_LEN - 1);
    entry.level[BASIC_LOG_LEVEL_LEN - 1] = '\0';
    strncpy(entry.message, msg.c_str(), BASIC_LOG_MESSAGE_LEN - 1);
    entry.message[BASIC_LOG_MESSAGE_LEN - 1] = '\0';

    nextIndex = (nextIndex + 1) % BASIC_LOG_CAPACITY;
    if (count < BASIC_LOG_CAPACITY) {
        ++count;
    }
    xSemaphoreGive(logMutex);
}
} // namespace

void basic_log_init()
{
    if (logMutex == nullptr) {
        logMutex = xSemaphoreCreateMutex();
    }
}

void basic_log_info(const String &msg)
{
    appendLog("INFO", msg);
}

void basic_log_warn(const String &msg)
{
    appendLog("WARN", msg);
}

void basic_log_error(const String &msg)
{
    appendLog("ERROR", msg);
}

String basic_log_render_html()
{
    if (logMutex == nullptr) {
        return "<p>No logs yet.</p>";
    }

    String out = "<ul>";
    xSemaphoreTake(logMutex, portMAX_DELAY);
    for (size_t i = 0; i < count; ++i) {
        const size_t index = (nextIndex + BASIC_LOG_CAPACITY - count + i) % BASIC_LOG_CAPACITY;
        const BasicLogEntry &entry = entries[index];
        out += "<li><code>";
        out += String(entry.timestampMs);
        out += "ms ";
        out += htmlEscape(entry.level);
        out += "</code> ";
        out += htmlEscape(entry.message);
        out += "</li>";
    }
    xSemaphoreGive(logMutex);
    out += "</ul>";
    return out;
}

String basic_log_render_text()
{
    if (logMutex == nullptr) {
        return "No logs yet.\n";
    }

    String out;
    xSemaphoreTake(logMutex, portMAX_DELAY);
    for (size_t i = 0; i < count; ++i) {
        const size_t index = (nextIndex + BASIC_LOG_CAPACITY - count + i) % BASIC_LOG_CAPACITY;
        const BasicLogEntry &entry = entries[index];
        out += String(entry.timestampMs);
        out += "ms ";
        out += entry.level;
        out += " ";
        out += entry.message;
        out += "\n";
    }
    xSemaphoreGive(logMutex);
    return out;
}
