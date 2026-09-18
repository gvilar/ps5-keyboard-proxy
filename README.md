# PS5 Keyboard Proxy — Raspberry Pi + RP2040

Fork and adaptation of [Nothka/ps5-keyboard-proxy](https://github.com/Nothka/ps5-keyboard-proxy).

This version adapts the original project to use a **Raspberry Pi Zero 2 W** as a bridge between a **Logitech G915 TKL** keyboard and an **RP2040**, which then presents itself to the PS5 as a USB HID keyboard.

## Architecture

```text
Logitech G915 TKL
        │
        │ USB
        ▼
Raspberry Pi Zero 2 W
        │
        │ evdev
        ▼
kb_passthrough.py
        │
        │ UART /dev/serial0
        │ 115200 baud
        ▼
RP2040
        │
        │ USB HID Keyboard
        ▼
       PS5
```

### Raspberry Pi role

The Raspberry Pi receives keyboard events from Linux through `evdev`.

The `kb_passthrough.py` script:

* detects the Logitech G915 TKL;
* reads keyboard events;
* converts Linux key codes to HID key codes;
* handles modifiers (`Ctrl`, `Shift`, `Alt`, `Meta`);
* builds 8-byte keyboard HID reports;
* sends these reports to the RP2040 through `/dev/serial0` at **115200 baud**.

The Raspberry Pi **does not directly present itself as a USB HID keyboard to the PS5**.

### RP2040 role

The RP2040 receives the 8-byte reports through UART and forwards them to the PS5 using **TinyUSB** as a USB HID Keyboard device.

The firmware is located in:

```text
rp2040/
├── CMakeLists.txt
├── main.c
├── tusb_config.h
└── usb_descriptors.c
```

## Differences from the original project

The original project uses a Linux HID Gadget device:

```text
/dev/hidg0
```

This adaptation replaces that output with a serial connection:

```text
Raspberry Pi
    │
    │ UART
    ▼
RP2040
    │
    │ USB HID
    ▼
PS5
```

The original keyboard passthrough logic, including the `keymap`, `modmap`, keyboard detection, pressed-key handling and modifier handling, has been kept and adapted to use the RP2040 UART output.

## Hardware

### Raspberry Pi

* Raspberry Pi Zero 2 W
* Raspberry Pi OS / Linux
* Python 3
* `python3-evdev`
* `pyserial`

### Keyboard

* Logitech G915 TKL

### RP2040

* RP2040 compatible with TinyUSB
* Tested with a **Waveshare RP2040 Zero**
* UART connection at 115200 baud

### UART connection

The tested wiring is:

```text
Raspberry Pi TX  ──────► RP2040 RX / GPIO1
Raspberry Pi GND ─────► RP2040 GND
```

The RP2040 firmware uses:

```text
UART0
115200 baud
TX GPIO0
RX GPIO1
```

The Raspberry Pi uses:

```text
/dev/serial0
```

## UART Report Format

Each report sent by the Raspberry Pi contains exactly **8 bytes**, following the USB HID Boot Keyboard report format:

```text
Byte 0 : modifiers
Byte 1 : reserved
Byte 2 : key 1
Byte 3 : key 2
Byte 4 : key 3
Byte 5 : key 4
Byte 6 : key 5
Byte 7 : key 6
```

Example:

```text
02 00 04 00 00 00 00 00
```

This represents:

```text
Left Shift + A
```

A null report:

```text
00 00 00 00 00 00 00 00
```

is used to report that keys have been released.

## Installation

Clone the repository:

```bash
git clone <URL_OF_YOUR_FORK>
cd ps5-keyboard-proxy
```

Install the Python dependencies:

```bash
sudo apt update
sudo apt install python3-evdev python3-serial
```

Check the input devices:

```bash
ls -l /dev/input/
```

The script automatically searches for a keyboard whose name contains:

```text
G915
```

and:

```text
Keyboard
```

## Manual execution

Run:

```bash
sudo python3 kb_passthrough.py
```

You should see output similar to:

```text
Using keyboard: ...
Keyboard grabbed
Using UART: /dev/serial0 @ 115200
```

UART reports are printed to the console in hexadecimal format.

## systemd service

The repository contains systemd service files in:

```text
services/
```

The keyboard passthrough service is:

```text
services/kbpassthrough.service
```

Install it with:

```bash
sudo cp services/kbpassthrough.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable kbpassthrough.service
sudo systemctl start kbpassthrough.service
```

Check its status:

```bash
systemctl status kbpassthrough.service
```

Follow the logs:

```bash
journalctl -u kbpassthrough.service -f
```

## RP2040 Firmware

The RP2040 firmware is located in:

```text
rp2040/
```

The firmware uses the **Raspberry Pi Pico SDK** and **TinyUSB**.

Set the Pico SDK path:

```bash
export PICO_SDK_PATH=/home/viciado/pico-sdk
```

Build the firmware:

```bash
cd rp2040
mkdir -p build
cd build
cmake ..
make -j2
```

The generated firmware, including the `.uf2` file, is placed in:

```text
rp2040/build/
```

The `build/` directory contains generated files and should not be committed to Git.

## UART Test

A HID report can be sent directly from the Raspberry Pi to test the complete UART → RP2040 → USB HID path:

```bash
python3 - <<'PY'
import serial
import time

s = serial.Serial('/dev/serial0', 115200, timeout=1)

# Right Arrow
s.write(bytes([0, 0, 0x4F, 0, 0, 0, 0, 0]))
s.flush()

time.sleep(0.2)

# Release
s.write(bytes(8))
s.flush()

s.close()

print("Right arrow sent")
PY
```

This allows the following path to be tested independently:

```text
Raspberry Pi
    ↓ UART
RP2040
    ↓ USB HID
PS5
```

## Troubleshooting

### Check the serial device

```bash
ls -l /dev/serial0
```

### Check the keyboard service

```bash
systemctl status kbpassthrough.service
```

### View service logs

```bash
journalctl -u kbpassthrough.service -f
```

### Check input devices

```bash
python3 - <<'PY'
from evdev import InputDevice, list_devices

for path in list_devices():
    dev = InputDevice(path)
    print(path, dev.name)
PY
```

### Run the passthrough manually

```bash
python3 kb_passthrough.py
```

UART HID reports should appear in the console.

## Limitations

This implementation primarily uses the standard **USB HID Boot Keyboard** format.

Therefore, Logitech-specific features may not be supported depending on the events exposed by Linux and the current key mapping, including:

* Logitech macros;
* proprietary Logitech functions;
* multimedia keys not present in the current `keymap`;
* RGB lighting;
* Logitech G HUB-specific features.

Support also depends on the input events exposed by `evdev`.

## Repository Structure

```text
ps5-keyboard-proxy/
├── README.md
├── LICENSE
├── .gitignore
│
├── kb_passthrough.py
├── install.sh
├── ps5kbd.sh
├── ps5kbd-gadget.sh
│
├── docs/
│   ├── setup.md
│   └── troubleshooting.md
│
├── services/
│   ├── btautoconnect.service
│   ├── kbpassthrough.service
│   └── ps5kbd.service
│
└── rp2040/
    ├── CMakeLists.txt
    ├── main.c
    ├── tusb_config.h
    └── usb_descriptors.c
```

Generated build files such as `build/`, `.uf2`, `.elf`, and other compilation artifacts are not required in the source repository.

## Attribution

This project is a fork/adaptation of:

**Nothka/ps5-keyboard-proxy**

Original project:

https://github.com/Nothka/ps5-keyboard-proxy

The original keyboard passthrough logic has been adapted to replace the `/dev/hidg0` output with UART communication to an RP2040.

