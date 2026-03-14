#include <task_safe_wire.h>

#include <Wire.h>
#include <assert.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

typedef struct
{
    SemaphoreHandle_t mutex;
    bool initialized;
    bool session_active;
    bool write_started;
} TaskSafeWireState;

static TaskSafeWireState task_safe_wire_state = {0};
static portMUX_TYPE task_safe_wire_setup_mux = portMUX_INITIALIZER_UNLOCKED;

static void task_safe_wire_ensure_mutex()
{
    if (task_safe_wire_state.mutex != NULL)
    {
        return;
    }

    portENTER_CRITICAL(&task_safe_wire_setup_mux);
    if (task_safe_wire_state.mutex == NULL)
    {
        task_safe_wire_state.mutex = xSemaphoreCreateMutex();
    }
    portEXIT_CRITICAL(&task_safe_wire_setup_mux);
}

void task_safe_wire_begin(uint8_t address)
{
    task_safe_wire_ensure_mutex();
    xSemaphoreTake(task_safe_wire_state.mutex, portMAX_DELAY);
    if (!task_safe_wire_state.initialized)
    {
        Wire.begin();
        task_safe_wire_state.initialized = true;
    }
    assert(!task_safe_wire_state.session_active);
    task_safe_wire_state.session_active = true;
    task_safe_wire_state.write_started = true;
    Wire.beginTransmission(address);
}

size_t task_safe_wire_write(uint8_t value)
{
    assert(task_safe_wire_state.session_active);
    return Wire.write(value);
}

void task_safe_wire_restart()
{
    assert(task_safe_wire_state.session_active);
    assert(task_safe_wire_state.write_started);
    Wire.endTransmission(false);
    task_safe_wire_state.write_started = false;
}

uint8_t task_safe_wire_request_from(uint8_t address, uint8_t quantity)
{
    assert(task_safe_wire_state.session_active);
    return Wire.requestFrom(address, quantity);
}

int task_safe_wire_read()
{
    assert(task_safe_wire_state.session_active);
    return Wire.read();
}

uint8_t task_safe_wire_end()
{
    uint8_t result = 0;
    assert(task_safe_wire_state.session_active);

    if (task_safe_wire_state.write_started)
    {
        result = Wire.endTransmission();
    }

    task_safe_wire_state.session_active = false;
    task_safe_wire_state.write_started = false;
    xSemaphoreGive(task_safe_wire_state.mutex);
    return result;
}
