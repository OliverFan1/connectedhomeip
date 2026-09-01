# All Devices App for NXP

This example demonstrates a dynamic Matter device application that allows you to
configure the device type at runtime via CLI commands. The selected device type
is persisted in NVM (Non-Volatile Memory) and automatically restored on
subsequent boots.

## Supported Hardware

| Board       |OS         | Connectivity | Config file              |
| ----------- |-----------| ------------ | ------------------------ |
| FRDM-RW612  |FreeRTOS   | Wi-Fi        | `prj_wifi.conf`          |
| FRDM-RW612  |FreeRTOS   | Thread (FTD) | `prj_thread_ftd.conf`    |


## Prerequisites

Set up the NXP SDK and Matter environment. Refer to the NXP platform
guide:

-   [docs/platforms/nxp/nxp_examples_freertos_platforms.md](../../../docs/platforms/nxp/nxp_examples_freertos_platforms.md)


## Building the Example

### FRDM-RW612 (Wi-Fi)

```bash
west build -d build-rw612-all-devices-app-wifi -b frdmrw612 examples/all-devices-app/nxp \
    -DCONF_FILE_NAME=prj_wifi.conf
```

### FRDM-RW612 (Thread FTD)

```bash
west build -d build-rw612-all-devices-app-thread -b frdmrw612 examples/all-devices-app/nxp \
    -DCONF_FILE_NAME=prj_thread_ftd.conf
```

## Flashing

For flashing and debugging the example application, follow detailed instructions
from the [nxp_rw61x_guide.md](../../../docs/platforms/nxp/nxp_rw61x_guide.md).

## Usage

### Serial Console

Testing the example with the CLI enabled will require connecting to UART1 and
UART2, here are more details to follow for RW61x platform :

-   UART1 : `Flexcomm3`. To view output on this UART, a USB cable could be
    plugged in. 115200 baud.
-   UART2 : `Flexcomm0`. To view output on this UART, a pin should be plugged to
    an `USB to UART adapter`. 115200 baud.
    -   For [`NXP RD-RW612-BGA`] board, use connector `HD2 pin 03`.
    -   For [`NXP FRDM-RW612`] board, use `J5 pin 4` (`mikroBUS`: TX).

### First Boot (No Stored Device Type)

On first boot, the application will display:

```
==================================================
No stored device type found.
Use command: devtype set <device-type>
==================================================
```

The Matter server does **not** start until a device type is selected.

### Setting a Device Type

Use the shell command to set and initialize a device type:

```
devtype set <device-type>
```

Example:

```
devtype set occupancy-sensor
```

On successful initialization, the device type is saved to NVM and the Matter
server starts.

### Subsequent Boots

On subsequent boots, the application automatically:

1. Reads the stored device type from NVM
2. Initializes the device with that type
3. Starts the Matter server

```
==================================================
Found stored device type: occupancy-sensor
Auto-initializing...
==================================================
```

### Supported Device Types

The available device types depend on what is registered in `DeviceFactory`.
Check
[all-devices-common/device-factory/DeviceFactory.h](../all-devices-common/device-factory/DeviceFactory.h)
for the full list of supported types.

## Device Attribute Control (Shell Commands)

After the Matter server has started (either via `devtype set` or auto-restored
from NVM), the `app` shell command group lets you drive device-type-specific
cluster attributes directly from the UART console.

> **Note:** These commands are only available after the server has started.
> Running them before `devtype set` will print an error.

| Command | Description |
| ------- | ----------- |
| `app occupancy <0\|1>` | Set OccupancySensing::Occupancy (occupancy-sensor only) |
| `app holdtime <seconds>` | Set OccupancySensing::HoldTime (occupancy-sensor only) |
| `app boolstate <0\|1>` | Set BooleanState::StateValue (contact-sensor / water-leak-detector only) |
| `app onoff <0\|1>` | Set OnOff::OnOff (on-off-light / on-off-plug-in-unit / dimmable-light only) |

### Examples

```
# Simulate occupancy detected
app occupancy 1

# Simulate no occupancy after 30 s hold time
app holdtime 30
app occupancy 0

# Trip a contact sensor
app boolstate 1

# Turn on a light
app onoff 1
```

## Commissioning

Once the Matter server is running, commission the device using chip-tool or
another Matter controller. For example:

```bash
chip-tool pairing ble-wifi <node-id> <ssid> <password> 20202021 3840
```

## Changing Device Type (Factory Reset)

To select a different device type after one has already been set, perform a
factory reset to clear the stored NVM data.

### Using the Matter Shell

```
matterfactoryreset
```

After the factory reset, reboot the device and set a new device type using the
`devtype set` command.
