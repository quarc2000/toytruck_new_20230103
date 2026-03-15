#ifndef BASIC_TELEMETRY_BASIC_LOGGER_H
#define BASIC_TELEMETRY_BASIC_LOGGER_H

#include <Arduino.h>

void basic_log_init();
void basic_log_info(const String &msg);
void basic_log_warn(const String &msg);
void basic_log_error(const String &msg);
String basic_log_render_html();
String basic_log_render_text();

#endif
