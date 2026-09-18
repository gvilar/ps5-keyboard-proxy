# PS5 Keyboard Proxy

A Raspberry Pi Zero 2 W + RP2040 based keyboard proxy that allows a Logitech G915 TKL keyboard to be used as a USB HID keyboard on a PlayStation 5.

This project is based on the original [`Nothka/ps5-keyboard-proxy`](https://github.com/Nothka/ps5-keyboard-proxy) project and is maintained in this fork:

https://github.com/gvilar/ps5-keyboard-proxy

## Architecture

```text
Logitech G915 TKL
        │
        │ evdev
        ▼
Raspberry Pi Zero 2 W
        │
        │ kb_passthrough.py
        │ UART /dev/serial0 @ 115200
        ▼
RP2040
        │
        │ USB HID Keyboard
        ▼
PlayStation 5
```

The Raspberry Pi handles the Logitech keyboard through Linux `evdev`.

The RP2040 receives HID keyboard reports over UART and presents itself to the PS5 as a standard USB HID keyboard.

The Pi does **not** emulate `/dev/hidg0`; the USB HID device connected to the PS5 is the RP2040.

## Hardware

* Raspberry Pi Zero 2 W
* RP2040 board
* Logitech G915 TKL
* PlayStation 5
* USB cable between the RP2040 and the PS5
* UART connection between the Raspberry Pi and RP2040

### UART wiring

The current firmware uses UART0:

| Raspberry Pi | RP2040        |
| ------------ | ------------- |
| TX           | GP1 / UART RX |
| RX           | GP0 / UART TX |
| GND          | GND           |

Use the appropriate UART pins for the specific RP2040 board.

The configured UART speed is:

```text
115200 baud
```

## UART Protocol

The Pi sends one USB HID keyboard report preceded by a two-byte synchronization sequence.

Each packet is:

```text
AA 55 + 8-byte HID keyboard report
```

Example:

```text
AA 55 02 00 04 00 00 00 00 00
```

The HID report is the standard 8-byte boot keyboard report:

```text
Byte 0: Modifier
Byte 1: Reserved
Byte 2: Key 1
Byte 3: Key 2
Byte 4: Key 3
Byte 5: Key 4
Byte 6: Key 5
Byte 7: Key 6
```

### Modifier byte

```text
Bit 0: Left Ctrl
Bit 1: Left Shift
Bit 2: Left Alt
Bit 3: Left GUI
Bit 4: Right Ctrl
Bit 5: Right Shift
Bit 6: Right Alt
Bit 7: Right GUI
```

For example:

```text
02 00 04 00 00 00 00 00
```

means:

```text
Left Shift + A
```

### Why `AA 55` is used

UART is a byte stream and does not preserve packet boundaries.

The original implementation assumed that every group of eight received bytes was exactly one HID report. If an extra byte or a lost byte caused the stream to become offset, all following reports could be misaligned.

The RP2040 firmware now searches for:

```text
AA 55
```

before reading the following eight bytes as a HID report.

This allows the receiver to resynchronize with the stream.

## Raspberry Pi Software

The Pi runs:

```text
kb_passthrough.py
```

The script:

1. Finds the Logitech G915 TKL through `evdev`.
2. Grabs the keyboard exclusively.
3. Converts Linux key codes to USB HID usage codes.
4. Maintains the current modifier state.
5. Builds an 8-byte HID keyboard report.
6. Adds the `AA 55` synchronization header.
7. Sends the packet through `/dev/serial0`.

Example packet:

```text
AA 55 02 00 04 00 00 00 00 00
```

## Installation

Install the required Python packages:

```bash
sudo apt update
sudo apt install python3-evdev python3-serial
```

Clone the repository:

```bash
git clone git@github.com:gvilar/ps5-keyboard-proxy.git
cd ps5-keyboard-proxy
```

Make sure the Logitech keyboard is connected and visible:

```bash
ls /dev/input/event*
```

You can inspect available input devices with:

```bash
python3 - <<'PY'
from evdev import InputDevice, list_devices

for path in list_devices():
    dev = InputDevice(path)
    print(path, dev.name)
PY
```

## Manual Execution

Run:

```bash
sudo python3 kb_passthrough.py
```

The script should report something similar to:

```text
Using keyboard: Logitech G915 TKL ... at /dev/input/event2
Keyboard grabbed
Using UART: /dev/serial0 @ 115200
```

When a key is pressed, the script prints the packet sent to the RP2040:

```text
UART: aa 55 02 00 04 00 00 00 00 00
```

## systemd Service

The project can run automatically at boot using:

```text
/etc/systemd/system/ps5-keyboard-proxy.service
```

Example:

```ini
[Unit]
Description=PS5 Keyboard Proxy - G915 to RP2040 UART
After=network.target
Wants=dev-serial0.device
After=dev-serial0.device

[Service]
Type=simple
User=root
WorkingDirectory=/home/viciado/ps5-keyboard-proxy
ExecStart=/usr/bin/python3 /home/viciado/ps5-keyboard-proxy/kb_passthrough.py
Restart=always
RestartSec=2

[Install]
WantedBy=multi-user.target
```

Enable and start:

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now ps5-keyboard-proxy.service
```

Check status:

```bash
systemctl status ps5-keyboard-proxy.service
```

View logs:

```bash
journalctl -u ps5-keyboard-proxy.service -f
```

## RP2040 Firmware

The RP2040 firmware is located in:

```text
rp2040/
```

The firmware uses:

* Raspberry Pi Pico SDK
* TinyUSB
* UART0
* USB HID Boot Keyboard

The current board configuration is:

```cmake
set(PICO_BOARD waveshare_rp2040_zero)
```

Set the Pico SDK path:

```bash
export PICO_SDK_PATH=/home/viciado/pico-sdk
```

Build:

```bash
cd rp2040/build
make -j2
```

The resulting UF2 file is:

```text
rp2040_uart_test.uf2
```

## Flashing the RP2040

Put the RP2040 into BOOTSEL mode and load:

```bash
picotool load rp2040_uart_test.uf2
```

Then reboot:

```bash
picotool reboot
```

The running USB device should appear as:

```text
2e8a:000a
```

Check with:

```bash
lsusb -d 2e8a:000a
```

## Testing the USB HID Interface

The RP2040 is exposed by Linux as a HID keyboard.

Find the corresponding input device:

```bash
for d in /sys/class/input/event*; do
    printf '%s: ' "$d"
    cat "$d/device/name" 2>/dev/null
done
```

The RP2040 should appear similar to:

```text
HID 2e8a:000a
```

The raw HID interface can also be found with:

```bash
for d in /sys/class/hidraw/hidraw*; do
    echo "===== $d ====="
    readlink -f "$d/device"
done
```

Look for:

```text
0003:2E8A:000A
```

## Testing the UART Protocol

A direct test can send a Shift+A HID report:

```bash
python3 - <<'PY'
import serial
import time

s = serial.Serial('/dev/serial0', 115200, timeout=1)

packet = bytes([
    0xAA, 0x55,
    0x02, 0x00, 0x04, 0x00,
    0x00, 0x00, 0x00, 0x00
])

print("TX:", packet.hex(' '))

s.write(packet)
s.flush()

time.sleep(1)
s.close()
PY
```

The RP2040 should expose the following HID report:

```text
02 00 04 00 00 00 00 00
```

This corresponds to:

```text
Left Shift + A
```

The same mechanism is used for Ctrl, Shift, Alt, GUI and normal keyboard keys.

## Troubleshooting

### Keyboard not detected

Check input devices:

```bash
python3 - <<'PY'
from evdev import InputDevice, list_devices

for path in list_devices():
    dev = InputDevice(path)
    print(path, dev.name)
PY
```

Make sure the G915 is present.

### Keyboard is detected but the script cannot grab it

Check which processes have the input device open:

```bash
fuser -v /dev/input/eventX
```

The Python process should have the device grabbed exclusively.

### UART unavailable

Check:

```bash
ls -l /dev/serial0
```

The expected UART is:

```text
/dev/serial0 -> /dev/ttyAMA0
```

Check the service logs:

```bash
journalctl -u ps5-keyboard-proxy.service -n 50 --no-pager
```

### RP2040 not detected by USB

Check:

```bash
lsusb -d 2e8a:000a
```

If the board is in BOOTSEL mode instead, it normally appears as:

```text
2e8a:0003
```

### HID reports appear corrupted

Verify that both sides use the current protocol:

```text
AA 55 + 8-byte HID report
```

The RP2040 firmware must be using the `AA 55` synchronization parser, and the Pi must send:

```python
uart.write(b"\xAA\x55" + report)
```

Do not send raw 8-byte reports to the current firmware.

### Check the raw HID report

Find the RP2040 `hidraw` device:

```bash
for d in /sys/class/hidraw/hidraw*; do
    echo "===== $d ====="
    readlink -f "$d/device"
done
```

Then read eight bytes:

```bash
python3 - <<'PY'
import os

fd = os.open('/dev/hidrawX', os.O_RDONLY)

data = os.read(fd, 8)
print(data.hex(' '))

os.close(fd)
PY
```

Replace `hidrawX` with the RP2040 device.

A Shift+A report should be:

```text
02 00 04 00 00 00 00 00
```

## Current Status

The current implementation has been tested successfully with:

* Logitech G915 TKL
* Raspberry Pi Zero 2 W
* RP2040
* UART at 115200 baud
* USB HID keyboard emulation
* PlayStation 5

Verified functionality includes:

* Normal keyboard keys
* Left Ctrl
* Left Shift
* Modifier combinations
* USB HID enumeration
* Automatic startup through systemd
* UART synchronization using `AA 55`

The PS5 sees the RP2040 as the USB keyboard device.

## Project Structure

```text
ps5-keyboard-proxy/
├── kb_passthrough.py
├── README.md
├── rp2040/
│   ├── CMakeLists.txt
│   ├── main.c
│   ├── tusb_config.h
│   ├── usb_descriptors.c
│   └── build/
└── ...
```

## Limitations

This is a relatively simple HID keyboard proxy.

The current implementation focuses on standard keyboard input and does not attempt to reproduce Logitech-specific functionality such as:

* G HUB profiles
* RGB lighting control
* Logitech macro processing
* Media-specific Logitech features
* Keyboard display features

The Pi converts Linux keyboard events to standard USB HID keyboard usage codes.

## Credits

Original project:

[`Nothka/ps5-keyboard-proxy`](https://github.com/Nothka/ps5-keyboard-proxy)

This repository is a fork with modifications for the Raspberry Pi Zero 2 W + RP2040 architecture and the current UART/HID implementation.

## License

See the repository's license file for the applicable license and attribution requirements.

