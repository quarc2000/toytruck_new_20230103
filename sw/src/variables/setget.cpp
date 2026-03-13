// #include "FreeRTOS.h"
#include "variables/setget.h"
#include "task_safe_wire.h"
#include <Wire.h>
#include <assert.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// Define a struct to encapsulate each variable
typedef struct
{
    volatile long value;
    volatile long total;
    volatile long org_value;
    volatile long prev_value;
    volatile long lastUpdate;
    xSemaphoreHandle sem;
} GlobalVar;

// Create an array of global variables
GlobalVar vars[NUM_VARS];

typedef struct
{
    SemaphoreHandle_t mutex;
    bool initialized;
    bool session_active;
    bool write_started;
    bool read_phase;
    bool lock_only;
} TaskSafeWireState;

static TaskSafeWireState task_safe_wire_state = {0};
static portMUX_TYPE task_safe_wire_setup_mux = portMUX_INITIALIZER_UNLOCKED;

static void task_safe_wire_ensure_mutex()
{
    if (task_safe_wire_state.mutex != NULL)
    {
        return;
    }

    taskENTER_CRITICAL(&task_safe_wire_setup_mux);
    if (task_safe_wire_state.mutex == NULL)
    {
        task_safe_wire_state.mutex = xSemaphoreCreateMutex();
    }
    taskEXIT_CRITICAL(&task_safe_wire_setup_mux);
}

void task_safe_wire_init()
{
    task_safe_wire_ensure_mutex();
    xSemaphoreTake(task_safe_wire_state.mutex, portMAX_DELAY);
    if (!task_safe_wire_state.initialized)
    {
        Wire.begin();
        task_safe_wire_state.initialized = true;
    }
    xSemaphoreGive(task_safe_wire_state.mutex);
}

void task_safe_wire_lock()
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
    task_safe_wire_state.write_started = false;
    task_safe_wire_state.read_phase = false;
    task_safe_wire_state.lock_only = true;
}

void task_safe_wire_unlock()
{
    assert(task_safe_wire_state.session_active);
    task_safe_wire_state.session_active = false;
    task_safe_wire_state.write_started = false;
    task_safe_wire_state.read_phase = false;
    task_safe_wire_state.lock_only = false;
    xSemaphoreGive(task_safe_wire_state.mutex);
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
    task_safe_wire_state.read_phase = false;
    task_safe_wire_state.lock_only = false;
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
    assert(!task_safe_wire_state.lock_only);
    Wire.endTransmission(false);
    task_safe_wire_state.write_started = false;
}

uint8_t task_safe_wire_request_from(uint8_t address, uint8_t quantity)
{
    assert(task_safe_wire_state.session_active);
    task_safe_wire_state.read_phase = true;
    return Wire.requestFrom(address, quantity);
}

int task_safe_wire_available()
{
    assert(task_safe_wire_state.session_active);
    return Wire.available();
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

    if (task_safe_wire_state.write_started && !task_safe_wire_state.lock_only)
    {
        result = Wire.endTransmission();
    }

    task_safe_wire_state.session_active = false;
    task_safe_wire_state.write_started = false;
    task_safe_wire_state.read_phase = false;
    task_safe_wire_state.lock_only = false;
    xSemaphoreGive(task_safe_wire_state.mutex);
    return result;
}

void globalVar_init()
{
    // Initialize the semaphores for each variable
    for (int i = 0; i < NUM_VARS; i++)
    {
        vars[i].sem = xSemaphoreCreateMutex();
        vars[i].total = 0;
        vars[i].prev_value = 9999;
        vars[i].org_value = 9999;
    }
}

void globalVar_set(VarNames varName, long value)
{
    xSemaphoreTake(vars[varName].sem, portMAX_DELAY);
    if (vars[varName].org_value == 9999)
        vars[varName].org_value = value;
    if (vars[varName].prev_value == 9999)
    {
        vars[varName].prev_value = value;
    }
    vars[varName].prev_value = vars[varName].value;
    vars[varName].value = value;
    vars[varName].total += value;
    vars[varName].lastUpdate = xTaskGetTickCount() * portTICK_PERIOD_MS;
    xSemaphoreGive(vars[varName].sem);
}

long globalVar_get(VarNames varName)
{
    long value;
    xSemaphoreTake(vars[varName].sem, portMAX_DELAY);
    value = vars[varName].value;
    xSemaphoreGive(vars[varName].sem);
    return value;
}

long globalVar_get_total(VarNames varName)
{
    long value;
    xSemaphoreTake(vars[varName].sem, portMAX_DELAY);
    value = vars[varName].total;
    xSemaphoreGive(vars[varName].sem);
    return value;
}

void globalVar_reset_total(VarNames varName)
{
    xSemaphoreTake(vars[varName].sem, portMAX_DELAY);
    vars[varName].total = 0;
    xSemaphoreGive(vars[varName].sem);
}

long globalVar_get_delta(VarNames varName)
{
    long value;
    xSemaphoreTake(vars[varName].sem, portMAX_DELAY);
    value = vars[varName].value - vars[varName].prev_value;
    xSemaphoreGive(vars[varName].sem);
    return value;
}

long globalVar_get_TOT_delta(VarNames varName)
{
    long value;
    xSemaphoreTake(vars[varName].sem, portMAX_DELAY);
    value = vars[varName].value - vars[varName].org_value;
    xSemaphoreGive(vars[varName].sem);
    return value;
}

long globalVar_get(VarNames varName, long *age)
{
    long value;
    xSemaphoreTake(vars[varName].sem, portMAX_DELAY);
    value = vars[varName].value;
    *age = xTaskGetTickCount() * portTICK_PERIOD_MS - vars[varName].lastUpdate;
    xSemaphoreGive(vars[varName].sem);
    return value;
}
