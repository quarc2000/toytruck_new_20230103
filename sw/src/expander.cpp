#include "expander.h"
#include "task_safe_wire.h"

#define SWITCH_I2C_ADDR 0x70  // Example address for the 9548 switch
#define GPIO_I2C_ADDR 0x20    // Example address for the 23017 GPIO expander

static constexpr uint8_t MCP23017_IODIRA = 0x00;
static constexpr uint8_t MCP23017_GPIOA = 0x12;
static constexpr uint8_t MCP23017_OLATA = 0x14;

static uint8_t readExpanderRegister(uint8_t device_addr, uint8_t reg)
{
    task_safe_wire_begin(device_addr);
    task_safe_wire_write(reg);
    task_safe_wire_restart();
    task_safe_wire_request_from(device_addr, (uint8_t)1);
    const uint8_t value = task_safe_wire_read();
    task_safe_wire_end();
    return value;
}

static void writeExpanderRegister(uint8_t device_addr, uint8_t reg, uint8_t value)
{
    task_safe_wire_begin(device_addr);
    task_safe_wire_write(reg);
    task_safe_wire_write(value);
    task_safe_wire_end();
}

// Constructor
EXPANDER::EXPANDER(uint8_t switchAddr, uint8_t gpioAddr)
    : switchAddr(switchAddr), gpioAddr(gpioAddr), switchStackIndex(0), currentChannel(0) {
}

// Private method retained for interface stability; bus init is lazy in task_safe_wire_begin().
void EXPANDER::beginI2C() {}

// Initialize the 9548 switch to default channel (0)
void EXPANDER::initSwitch() {
    beginI2C();
    setChannel(0);  // Set to channel 0 by default
}

// Set the channel for the 9548 switch (0-7)
void EXPANDER::setChannel(uint8_t channel) {
    if (channel > 7) return;  // Validate channel
    task_safe_wire_begin(switchAddr);
    task_safe_wire_write(1 << channel);  // Write the corresponding channel
    task_safe_wire_end();
    currentChannel = channel;
}

// Push the current channel onto the stack and switch to the new channel
void EXPANDER::pushChannel(uint8_t channel) {
    if (switchStackIndex < 8) {
        // Save the current channel (in case pop is called)
        switchStack[switchStackIndex++] = currentChannel;
        setChannel(channel);  // Set the new channel
    }
}

// Pop the last channel from the stack and switch back to it
void EXPANDER::popChannel() {
    if (switchStackIndex > 0) {
        uint8_t lastChannel = switchStack[--switchStackIndex];
        setChannel(lastChannel);  // Switch back to the last channel
    }
}

// Initialize GPIO Expander (23017)
void EXPANDER::initGPIO() {
    beginI2C();
    // Known light outputs on the current truck harness.
    setGPIOPinMode(0, false);  // Headlight / full beam candidate
    setGPIOPinMode(1, false);  // Brake light candidate
    setGPIOPinMode(7, false);  // LED / light output already used in tests
    setGPIOPinMode(8, false);  // Indicator candidate on port B
    setGPIOPinMode(9, false);  // Indicator candidate on port B
    setGPIOPinMode(11, false); // Reverse light candidate on port B

    // Leave the remaining higher pins available as inputs for later add-ons
    // such as pulse or distance-related sensors until a concrete output need exists.
    for (uint8_t i = 10; i <= 15; i++) {
        if (i == 11) {
            continue;
        }
        setGPIOPinMode(i, true);
    }
}

// Set GPIO pin mode to input or output
void EXPANDER::setGPIOPinMode(uint8_t pin, bool input) {
    pushChannel(0);  // Switch to channel 0 to communicate with MCP23017
    uint8_t mask = 1 << (pin % 8);
    const uint8_t direction_reg = MCP23017_IODIRA + (pin / 8);
    uint8_t currentMode = readExpanderRegister(gpioAddr, direction_reg);
    if (input) {
        currentMode |= mask;  // Set bit for input
    } else {
        currentMode &= ~mask; // Clear bit for output
    }
    writeExpanderRegister(gpioAddr, direction_reg, currentMode);
    popChannel();  // Return to the previous channel
}

// Write high/low to a GPIO pin
void EXPANDER::writeGPIO(uint8_t pin, bool value) {
    pushChannel(0);  // Switch to channel 0 to communicate with MCP23017
    const uint8_t latch_reg = MCP23017_OLATA + (pin / 8);
    uint8_t mask = 1 << (pin % 8);
    uint8_t currentLatch = readExpanderRegister(gpioAddr, latch_reg);
    if (value) {
        currentLatch |= mask;
    } else {
        currentLatch &= ~mask;
    }
    writeExpanderRegister(gpioAddr, latch_reg, currentLatch);
    popChannel();  // Return to the previous channel
}

// Read the state of a GPIO pin
bool EXPANDER::readGPIO(uint8_t pin) {
    pushChannel(0);  // Switch to channel 0 to communicate with MCP23017
    const uint8_t pinState = readExpanderRegister(gpioAddr, MCP23017_GPIOA + (pin / 8));
    popChannel();  // Return to the previous channel
    return (pinState & (1 << (pin % 8))) != 0;
}

// Toggle the GPIO pin state (high/low)
void EXPANDER::toggleGPIO(uint8_t pin) {
    bool currentState = readGPIO(pin);
    writeGPIO(pin, !currentState);
}

// Set the LED state on GPA7
void EXPANDER::setLED(bool state) {
    writeGPIO(7, state);  // GPA7 controls the LED (set high to light up)
}
