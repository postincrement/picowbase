# Pico W Blink Example

This is a simple example project for the Raspberry Pi Pico W that demonstrates:
- Basic GPIO control
- WiFi connectivity
- LED blinking
- USB serial output
- Interactive CLI interface

## Prerequisites

- Docker or Podman installed on your system
- ARM GCC toolchain installed on your system
- picotool installed on your host system (not in container)

## Building the Project

1. Build the container (only needed first time):
   ```bash
   podman build -t pico-w-builder .
   ```

2. Create and start the container (only needed first time):
   ```bash
   podman create --name pico-w-builder -v /absolute/path/to/your/project:/build localhost/pico-w-builder
   podman start pico-w-builder
   ```
   Replace `/absolute/path/to/your/project` with the full path to your project directory.

3. To access the container's shell:
   ```bash
   podman exec -it pico-w-builder /bin/bash
   ```

4. Inside the container, run the build script:
   ```bash
   cd /build
   ./build.sh
   ```

5. After the build completes, you can exit the shell:
   ```bash
   exit
   ```

## Managing the Container

- To stop the container:
  ```bash
  podman stop pico-w-builder
  ```

- To start the container again:
  ```bash
  podman start pico-w-builder
  ```

- To remove the container (if you need to recreate it):
  ```bash
  podman rm pico-w-builder
  ```

## Programming the Pico W

The build process creates a UF2 file in the `build` directory. To program your Pico W:

1. Make sure picotool is installed on your host system (not in the container)
2. Connect your Pico W to your computer via USB
3. Run the following command from your project directory:
   ```bash
   picotool load build/pico_w_blink.uf2
   ```

If you need to reboot the Pico W into bootloader mode:
```bash
picotool reboot -f -u
```

## Using the CLI Interface

After programming, the Pico W will provide a command-line interface over USB serial. To access it:

### On macOS:

1. Find the correct serial port:
   ```bash
   ls /dev/tty.usbmodem*
   ls /dev/cu.usbmodem*
   ```

2. Connect using one of these methods:

   a. Using `screen`:
   ```bash
   screen /dev/tty.usbmodem* 115200
   ```
   To exit screen: Press `Ctrl-A` followed by `Ctrl-\` and confirm with `y`

   b. Using Serial (Mac App Store):
   - Open Serial
   - Select the port from the dropdown
   - Set baud rate to 115200
   - Click "Open"

   c. Using CoolTerm:
   - Download from http://freeware.the-meiers.org/
   - Open CoolTerm
   - Click "Options" and select the port
   - Set baud rate to 115200
   - Click "Connect"

### On Linux:

1. Connect using screen:
   ```bash
   screen /dev/ttyACM0 115200
   ```
   To exit screen: Press `Ctrl-A` followed by `Ctrl-\` and confirm with `y`

### On Windows:

1. Use a terminal program like PuTTY or MobaXterm
2. Select the COM port assigned to the Pico W
3. Set baud rate to 115200

Available commands:
- `help` - Show available commands
- `led on` - Turn the LED on
- `led off` - Turn the LED off
- `status` - Show system status
- `clear` - Clear the screen
- `exit` - Enter bootloader mode for programming

## Project Structure

- `main.cpp` - Main application code
- `CMakeLists.txt` - CMake build configuration
- `build.sh` - Build script
- `lwipopts.h` - lwIP configuration for WiFi
- `Dockerfile` - Container build configuration

## Features

- Blinks the onboard LED
- Connects to WiFi
- Outputs status messages via USB serial
- Uses C++20 standard
- Supports programming without disconnecting USB cable
- Interactive CLI interface over USB serial

## Troubleshooting

If you encounter any issues:

1. Make sure your Pico W is properly connected
2. Verify that picotool is installed and accessible from your host system
3. Check that the container has access to your project directory
4. Ensure you have the correct permissions to access the USB device
5. If the container isn't working properly, try removing and recreating it:
   ```bash
   podman rm pico-w-builder
   # Then follow the "Building the Project" steps again
   ```
6. If you can't access the CLI:
   - Check that the USB serial port is available
   - Try a different USB cable
   - Verify the baud rate is set to 115200
   - Try resetting the Pico W
   - On macOS, make sure you're using the correct port name (tty.usbmodem*)
   - Try a different terminal program if screen doesn't work

## License

This project is licensed under the MIT License - see the LICENSE file for details. 