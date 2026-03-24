# AP_HAL_RP

ArduPilot Hardware Abstraction Layer (HAL) for Raspberry Pi RP series MCUs.

## Overview

This HAL allows ArduPilot flight control software to run on RP2350-based flight controllers, utilizing the Raspberry Pi Pico C/C++ SDK and its built-in FreeRTOS support.

## Features

- Uses an approach similar to AP_HAL_ESP32, fully isolated from the ChibiOS infrastructure.
- Integration with the Pico SDK for low-level peripheral access.
- Multitasking support via FreeRTOS.
- Waf-based build system using custom rp_hwdef.py scripts.

## Supported Boards

- [List of boards will be added here as they are integrated]
- Default development board: [Kolibri-FC](https://github.com/bastian2001/Kolibri-FC-Hardware)

## Building the Firmware

- [Building on Linux/Ubuntu](#building-on-linuxubuntu).
- [Building on macOS](#building-on-macos).
- [Building on Windows](#building-on-windows).

### Building on Linux/Ubuntu

*Note*: Build was tested on Ubuntu 24.04 LTS

1. Run the following script from the cloned ardupilot directory to install required packages:

```bash
Tools/environment_install/install-prereqs-ubuntu.sh -y
```

2. Reload the path (log-out and log-in to make it permanent):

```bash
. ~/.profile
```

3. Install Raspberry Pi Pico C/C++ SDK and FreeRTOS kernel:

```bash
Tools/scripts/get_pico_sdk.sh -d <path_to_install>
```

You can install Pico SDK and FreeRTOS kernel into modules/pico, just run the installation script without any parameters:

```bash
Tools/scripts/get_pico_sdk.sh
```

4. Configure and build the firmware:

```bash
./waf configure --board Kolibri
./waf copter
```

*Note*: The first configure command should be called only once or when you want to change a
configuration option. One configuration often used is the `--board` option to
switch from one board to another one.

### Building on macOS

*Note*: Build was tested on MacOs 15.0.1 (24A348)

1. Run the following script from the cloned ardupilot directory to install required packages:

```bash
Tools/environment_install/install-prereqs-mac.sh -y
```

2. Reload the path (restart your terminal or source the appropriate shell configuration file):

```bash
source ~/.zshrc
```

or if using bash:

```bash
source ~/.bash_profile
```

3. Install Raspberry Pi Pico C/C++ SDK and FreeRTOS kernel:

```bash
Tools/scripts/get_pico_sdk.sh -d <path_to_install>
```

You can install Pico SDK and FreeRTOS kernel into modules/pico, just run the installation script without any parameters:

```bash
Tools/scripts/get_pico_sdk.sh
```

4. Configure and build the firmware:

```bash
./waf configure --board Kolibri
./waf copter
```

*Note*: The first configure command should be called only once or when you want to change a
configuration option. One configuration often used is the `--board` option to
switch from one board to another one.

### Building on Windows

*Note*: Build was tested on **Windows 11**

1. Install Windows Subsystem for Linux (**WSL**):

   ```sh
   wsl --set-default-version 2
   wsl --install
   ```

2. Set memory config for the virtual environment. Open (or create, if doesn't exist) file `C:\Users\<YourUsername>\.wslconfig`, and set

   ```sh
   [wsl2]
   memory=12GB
   swap=16GB
   ```

   *Note:* Numbers *may not be optimal* in the example, you can set what is best for you system.

3. Login to **Linux** environment:

   1. Press the Windows Key.
   2. Type "Ubuntu".
   3. Click on the Ubuntu icon

4. Follow all the steps described in [Building the Firmware](#building-the-firmware) section

5. Map usb ports (required to see controller from virtual environment)

   Because WSL 2 runs in a utility VM, it doesn't "see" the physical USB hardware plugged into your Windows machine by default. To access a USB device, you need to "attach" the hardware manually.

   1. Install **usbipd-win** on Windows:

      Download and run the .msi installer from the **usbipd-win** GitHub releases page: <https://github.com/dorssel/usbipd-win/releases>
   2. Install the USB Tools in Ubuntu:

      ```bash
      sudo apt update
      sudo apt install linux-tools-virtual hwdata
      sudo update-alternatives --install /usr/local/bin/usbip usbip `ls /usr/lib/linux-tools/*/usbip | tail -n1` 20
      ```

   3. Attaching a Device

      Once the software is installed, follow these steps in **PowerShell** (with **Administrator** privileges):

      ```sh
      usbipd list
      ```

      Find the **BUSID** of the device you want to share (e.g., `2-1`).
   4. Bind the device:

      This tells Windows to allow the device to be shared.

      ```sh
      usbipd bind --busid <BUSID>
      ```

   5. Attach to WSL:

      ```sh
      usbipd attach --wsl --busid <BUSID>
      ```

   6. Verify in Ubuntu

      Go back to your Ubuntu terminal and run:

      ```bash
      lsusb
      ```

      You should now see your USB device listed exactly as if it were plugged into a native Linux machine.

## Building the Examples

This is the easiest way to create firmware that focuses only on testing a specific HAL module or library on a real flight controller.

1. Configure the required board:

```bash
./waf configure --board Kolibri
```

2. Build the example:

```bash
./waf build --target examples/RCOutput
```

HAL examples are located in the libraries/AP_HAL/examples folder.

## Uploading the Firmware

### Flashing the RP2350 firmware is a simple drag-and-drop process

- Disconnect the flight controller from power.
- Press and hold the BOOT button and connect the USB cable.
- A new USB mass storage device named RPI-RP2 will appear on your computer.
- Copy the generated arducopter.uf2 file to this drive.
- The drive will automatically unmount, and the board will reboot with the new ArduPilot firmware running.

## Hardware Definition (hwdef)

The configuration for Input/Output (GPIO) pins, SPI/I2C/UART buses, and sensors is defined in the hwdef.dat files for each board, located in the libraries/AP_HAL_RP/hwdef/ subdirectory.

## Contributing

Bug fixes, improvements, and support for new RP2350-based flight controllers are welcome. Please refer to the ArduPilot Developer Guidelines before submitting pull requests.
