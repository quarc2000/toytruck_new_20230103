#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <sensors/usensor.h>
#include <variables/setget.h>

#define MAX_SENSORS 4

#define POLL_INTERVAL 50
// time in ms between each sensor poll, remember one sensor at a time is polled
// not to interfere with each other
// do not go below 15ms (12 ms + some margin, for a 199cm measure)
// as the previous sound might not have had time to reflect
// 30-50 ms should be enought in most cases, and will drastically reduce CPU load
// but until you experience CPU overload, 15-20ms is ok.
// you can expect the longest age of a measure to be 4 times (with the standard truck set-up)
// the length of the POLL_INTERVAL
// If you need to reduce this time or have a need of more sensors
// you need to base the distance measure on laser rather than sound, or
// Accept a smaller maximum measure distance. If you go down to 5 ms for instance
// the absolute maximu you could measure would be about 80 cm, above that the measure would say 199
// also consider the time it takes to move a task back and forth to the delay queue (overhead)
// set the time to what you NEED, not what would be cool :-) Rather start with a high number
// like 50ms or even 100ms and see if/when you need more. Even with 100ms and 4 sensors you will
// still measure the distance in all direction more then twice each second!

struct sensors
{
    uint8_t TRIG;
    uint8_t ECHO;
    VarNames NAME;
};

static sensors sensor[MAX_SENSORS];
static portMUX_TYPE usensorMux = portMUX_INITIALIZER_UNLOCKED;
static volatile int num_sensors = 0;
static volatile int current_sensor = -1;
static volatile bool pulse_active = false;
static volatile bool echo_started = false;
static volatile uint32_t startTime = 0;
static volatile bool measurement_ready = false;
static volatile int pending_distance = 199;
static volatile VarNames pending_name = rawDistFront;
static bool trigger_task_started = false;

void echoInterrupt()
{
    portENTER_CRITICAL_ISR(&usensorMux);

    const int sensor_index = current_sensor;
    if (sensor_index < 0 || sensor_index >= num_sensors)
    {
        portEXIT_CRITICAL_ISR(&usensorMux);
        return;
    }

    if (digitalRead(sensor[sensor_index].ECHO) == HIGH)
    {
        pulse_active = true;
        echo_started = true;
        startTime = micros();
    }
    else if (pulse_active && echo_started)
    {
        uint32_t travelTime = micros() - startTime;
        int distance = travelTime / 29 / 2;
        if (distance > 199)
        {
            distance = 199;
        }

        pulse_active = false;
        echo_started = false;
        pending_distance = distance;
        pending_name = sensor[sensor_index].NAME;
        measurement_ready = true;
    }

    portEXIT_CRITICAL_ISR(&usensorMux);
}

static bool dequeueMeasurement(VarNames *name, int *distance)
{
    bool ready = false;

    portENTER_CRITICAL(&usensorMux);
    if (measurement_ready)
    {
        *name = pending_name;
        *distance = pending_distance;
        measurement_ready = false;
        ready = true;
    }
    portEXIT_CRITICAL(&usensorMux);

    return ready;
}

static void TriggerTask(void *params)
{
    for (;;)
    {
        VarNames measured_name;
        int measured_distance;
        if (dequeueMeasurement(&measured_name, &measured_distance))
        {
            globalVar_set(measured_name, measured_distance);
        }

        int previous_sensor = -1;
        int active_sensors = 0;

        portENTER_CRITICAL(&usensorMux);
        active_sensors = num_sensors;
        previous_sensor = current_sensor;
        portEXIT_CRITICAL(&usensorMux);

        if (active_sensors > 0 && previous_sensor >= 0)
        {
            detachInterrupt(digitalPinToInterrupt(sensor[previous_sensor].ECHO));
        }

        if (active_sensors > 0)
        {
            int next_sensor;
            uint8_t trig_pin;
            uint8_t echo_pin;
            VarNames timeout_name = rawDistFront;
            bool emit_timeout = false;

            portENTER_CRITICAL(&usensorMux);
            active_sensors = num_sensors;
            if (active_sensors > 0)
            {
                if (pulse_active && !measurement_ready && current_sensor >= 0)
                {
                    emit_timeout = true;
                    timeout_name = sensor[current_sensor].NAME;
                }

                pulse_active = false;
                echo_started = false;
                next_sensor = current_sensor + 1;
                if (next_sensor >= active_sensors || next_sensor < 0)
                {
                    next_sensor = 0;
                }
                current_sensor = next_sensor;
                trig_pin = sensor[next_sensor].TRIG;
                echo_pin = sensor[next_sensor].ECHO;
            }
            portEXIT_CRITICAL(&usensorMux);

            if (emit_timeout)
            {
                globalVar_set(timeout_name, 199);
            }

            pinMode(trig_pin, OUTPUT);
            pinMode(echo_pin, INPUT);
            attachInterrupt(digitalPinToInterrupt(echo_pin), echoInterrupt, CHANGE);

            portENTER_CRITICAL(&usensorMux);
            pulse_active = true;
            echo_started = false;
            portEXIT_CRITICAL(&usensorMux);

            digitalWrite(trig_pin, HIGH);
            delayMicroseconds(10);
            digitalWrite(trig_pin, LOW);
        }
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL));
    }
}

Usensor::Usensor()
{
}

int Usensor::open(uint8_t trig, uint8_t echo, VarNames name)
{
    bool start_task = false;
    int opened_sensors;

    portENTER_CRITICAL(&usensorMux);
    if (num_sensors < MAX_SENSORS)
    {
        Serial.print("+");
        sensor[num_sensors].TRIG = trig;
        sensor[num_sensors].ECHO = echo;
        sensor[num_sensors].NAME = name;
        num_sensors++;
        if (!trigger_task_started)
        {
            trigger_task_started = true;
            start_task = true;
        }
    }
    opened_sensors = num_sensors;
    portEXIT_CRITICAL(&usensorMux);

    if (start_task)
    {
        xTaskCreate(TriggerTask, "testsetup", 2000, NULL, 1, NULL);
    }

    return opened_sensors;
}
