# TOF Laser Range Sensor Mini

## Summary

This is the Waveshare `TOF Laser Range Sensor Mini` module.

It should be treated as a module-level device with its own controller and protocol, not as a generic interchangeable bare ToF chip like a `VL53L0X`.

For this project, the intended interface is `I2C`.

## Source References

- Product page: [Waveshare TOF Laser Range Sensor Mini](https://www.waveshare.com/tof-laser-range-sensor-mini.htm)
- Wiki: [Waveshare TOF Laser Range Sensor Mini Wiki](https://www.waveshare.com/wiki/TOF_Laser_Range_Sensor_Mini)
- Amazon listing used by the user: [Waveshare Mini Laser Range Sensor](https://www.amazon.se/-/en/Waveshare-Mini-Laser-Range-Sensor/dp/B0DP98Z2DW)

## Naming Notes

- The useful name is `TOF Laser Range Sensor Mini`
- The source material does not clearly identify the underlying ranging IC in the same way that ST `VL53` parts are identified
- The Waveshare documentation instead presents the module as a controller-backed product with its own protocol family

## I2C Facts Worth Reusing

- The module supports `I2C`
- The documented I2C speed is up to `400 kHz`
- The documented default 7-bit I2C slave address is `0x08`
- The slave address is described as `0x08 + module ID`
- The module ID can be changed, which is how multiple modules can coexist on the same bus
- The vendor documentation says up to 8 modules can be connected in parallel through I2C

## Protocol Notes

- The module does not appear to use a generic `VL53L0X`-style register map
- The Waveshare wiki refers to `NLink_TOFSense_IIC_Frame0` for I2C communication
- That means future integration should start from the vendor's module protocol documentation, not from assumptions based on other ToF sensors

## Electrical Notes

- Documented supply voltage is `4.3 V` to `5.2 V`
- The wiki describes the module family as using a built-in controller and ranging algorithm
- The listed mini-module range is about `0.02 m` to `7.8 m`

## Why This Matters

- This module is a strong candidate for the planned indoor ceiling-height mapping path
- Because it is module-protocol-driven rather than obviously chip-register-driven, future software work should begin with the vendor I2C frame format and not with the existing `VL53L0X` helper

## Open Questions

- What exact I2C frame layout is needed for the simplest distance read on ESP32
- How the module should be configured into I2C mode if it does not arrive in that mode by default
- Whether the project will keep the default module ID and address or assign a different one for multi-sensor setups
