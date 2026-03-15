# VL53L5CX

## Summary

The `VL53L5CX` is an ST time-of-flight multizone ranging sensor that can report either `4x4` or `8x8` zones.

For first bring-up, the important facts are:

- the I2C address convention
- the `LPn` reset or enable behavior
- the fact that normal use depends on the ST ultra-light driver (`ULD`) flow rather than a tiny one-register bring-up
- what minimal output should be requested first to keep I2C traffic manageable

## Datasheet

- Local file: [vl53l5cx.pdf](../datasheets/vl53l5cx.pdf)
- Original source: [ST product page](https://www.st.com/en/imaging-and-photonics-solutions/vl53l5cx)

## Related ST Documentation

- ULD usage guide: [UM2884](https://www.st.com/resource/en/user_manual/dm00797144-a-guide-to-using-the-vl53l5cx-multizone-timeofflight-ranging-sensor-with-wide-field-of-view-ultra-lite-driver-uld-stmicroelectronics.pdf)
- ULD integration guide: [UM2887](https://www.st.com/resource/en/user_manual/um2887-a-software-integration-guide-to-implement-the-ultra-light-driver-of-the-vl53l5cx-timeofflight-8-x-8-multizone-ranging-sensor-with-wide-field-of-view-stmicroelectronics.pdf)

## Facts We Rely On

- The sensor supports `4x4` and `8x8` zone output modes
- The full `8x8` mode produces `64` zone measurements
- The sensor uses `I2C`
- The datasheet exposes a dedicated `I2C control interface` section
- The host-visible control path is not a single measurement register. Normal host software uses the ST `ULD` driver flow over I2C.
- The power-up sequence and `LPn` behavior matter for successful I2C access
- `INT` is the practical data-ready signal when using the normal ranging flow
- ST documentation commonly describes the default device address as:
  - `0x29` in 7-bit form
  - `0x52` for the left-shifted write-byte convention
- The address can be changed when multiple sensors are on the same bus
- ST public driver material exposes a device ID register at `0x010F`
- The expected device ID value is `0xF0 0x02` when read as two bytes, commonly treated as `0xF002`

## Address

- Default `7-bit` I2C address: `0x29`
- Same address in the old `8-bit` write-byte notation sometimes shown by ST material: `0x52`
- For firmware on this project, prefer the `7-bit` form `0x29`
- In multi-sensor setups, the usual pattern is:
  - hold other sensors unavailable with `LPn`
  - bring one sensor up
  - assign it a new address
  - repeat for the next sensor

## Early Identification Check

- A useful first custom bring-up check after plain I2C `ACK` is reading the device ID register
- ST public driver material uses:
  - register: `0x010F`
  - expected value: `0xF002`
- This is a much stronger first checkpoint than plain `ACK`, because it confirms the bus device is behaving like a `VL53L5CX` rather than just responding at `0x29`

## Setup-Critical Pins And Signals

- `LPn`
  - hardware low-power or reset-style control used during startup sequencing
  - if held low, the sensor is not available for normal I2C use
  - commonly used to isolate one sensor at a time when assigning addresses on a shared bus
- `INT`
  - used as the practical data-ready signal in the normal ranging flow
  - can be polled indirectly through the driver if no GPIO interrupt path is used yet

## Setup Registers And Configuration Reality

For this chip, the important practical point is that bring-up is not usually done by manually poking a short list of user-facing setup registers the way one might with a `VL53L0X` or `MPU6050`.

What matters instead:

- the ST `ULD` initialization sequence
- the firmware load over I2C
- resolution selection (`4x4` or `8x8`)
- ranging start or stop control
- polling or waiting for data ready

So for this project, the reusable setup guidance is:

- do not plan around "three or four key setup registers"
- plan around the `ULD` platform hooks and init flow instead
- keep the first configuration minimal:
  - one sensor
  - one mux channel
  - one resolution
  - one target frame path

## How Data Is Retrieved

The normal host-side flow is:

1. power and `LPn` sequencing so the sensor is actually reachable
2. I2C probe at `0x29`
3. `ULD` init path, including firmware load
4. choose resolution such as `4x4` or `8x8`
5. start ranging
6. wait for data ready
7. read one result frame through the driver result structure

For a first successful read, the output should stay small:

- whether init succeeded
- whether ranging started
- whether data ready became true
- one frame summary:
  - center zone distance
  - minimum zone distance
  - maximum zone distance
  - maybe one compact `8x8` dump later

This is the right first data path. The first goal is not direct raw register dumping of all `64` zones.

## Bring-Up Notes

- First bring-up should treat `LPn` as important:
  - `LPn` low keeps the sensor unavailable
  - `LPn` high allows normal startup and I2C communication
- Multi-sensor setups normally rely on holding other devices in reset with `LPn` while one sensor is assigned a new address
- Normal operation is not a tiny bare-register workflow like `VL53L0X`
- ST's documented software path expects the `ULD` flow:
  - initialize the platform layer
  - load firmware
  - configure the sensor
  - start ranging
  - poll or interrupt on data ready
  - read the result structure
- For a TCA9548-style mux path, the safest first structure is:
  - select mux channel
  - ensure only the intended sensor is reachable
  - complete init on that sensor
  - read one summarized frame

## I2C and Data Volume Notes

- The ULD integration guide says initialization loads a large firmware image over I2C
- The same guide notes roughly `86 kB` must be loaded during initialization
- That means bring-up code must support larger I2C transfers than simple sensor drivers
- ST also documents that packet sizes depend on:
  - resolution (`4x4` vs `8x8`)
  - number of targets per zone
  - which outputs are enabled
- A first project bring-up should keep the output set small

## Practical First-Read Strategy

For this project, the first useful success criteria should be:

- sensor detected on I2C
- initialization succeeds
- ranging starts
- data-ready becomes true
- one valid frame is read

And the first printed outputs should stay minimal:

- resolution in use
- number of zones
- one center-zone distance
- frame minimum distance
- frame maximum distance

This is a better first step than dumping all `64` zones immediately.

## Known Quirks

- Address notation is easy to misread because ST material and community examples sometimes mention `0x29` and sometimes `0x52`
- The sensor is much heavier than simple ToF parts in both initialization and readout size
- A host implementation that works for tiny one-register sensors may still fail here if the I2C read or write helpers do not support the required transfer sizes

## Open Questions

- Whether the first project bring-up should use the ST `ULD` code directly or wrap only the minimal platform layer needed for the existing firmware style
- Which outputs should be enabled for the first truck test so the I2C payload stays small
- Whether the current expander-mux path and task-safe I2C wrapper need any size or timeout adjustments for the `ULD` initialization load
- Which exact `INT` handling model is best for the first truck bring-up:
  - pure polling
  - one GPIO interrupt line
  - or no `INT` usage in the very first pass

## Verified vs Inferred

Verified from the local datasheet outline and ST ULD documentation:

- `4x4` and `8x8` zone modes exist
- `I2C` is the control interface
- power-up and I2C access require attention to `LPn`
- default address usage is described as `0x29` or `0x52` depending on notation
- initialization depends on a large firmware load

Inference for this project:

- the first clean truck integration should aim for a minimal valid frame and a compact summary printout before trying to integrate the full `8x8` dataset into mapping or shared variables
