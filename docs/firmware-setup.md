# Firmware Setup

How to install the toolchain and flash the XIAO ESP32-C3.

The XIAO does **not** appear in the Arduino IDE out of the box. It ships
inside Espressif's ESP32 board package, which has to be added manually.

---

## 1. Install the Arduino IDE

Download the current version from
[arduino.cc/en/software](https://www.arduino.cc/en/software) and install
it normally. Version 2.x is fine.

## 2. Add the ESP32 board manager URL
s
Arduino IDE → **Settings** (macOS: Arduino IDE → Settings; Windows:
File → Preferences).

In **Additional Boards Manager URLs**, paste:

```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

Click OK.

> If that URL ever fails, this mirror is identical:
> `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`

## 3. Install the ESP32 package

**Tools → Board → Boards Manager**, search `esp32`, and install the
**esp32 by Espressif Systems** package.

It is a large download and takes a few minutes.

Version **2.0.8 or later** is required for the XIAO boards. Take the
latest.

## 4. Select the board

**Tools → Board → ESP32 Arduino → XIAO_ESP32C3**

That list is long and alphabetical-ish; the XIAO entries sit near the
end. If you cannot find it, the package did not install — go back to
step 3.

## 5. Set the board options

| Setting | Value | Why |
|---|---|---|
| Board | XIAO_ESP32C3 | not ESP32C3 Dev Module, not S3 |
| USB CDC On Boot | **Enabled** | required, see below |
| Upload Speed | 921600 | drop to 115200 if uploads fail partway |
| Port | the one with `usbmodem` in the name | |

**USB CDC On Boot must be Enabled.** The C3 talks to your computer over
native USB rather than a separate serial chip. With this disabled,
`Serial` output goes nowhere and the Serial Monitor stays blank, which
looks exactly like a firmware problem.

## 6. Flash it

Open `firmware/syringe_cycler_wifi_volatile.ino`.

**Program the XIAO out of its socket**, plugged straight into USB. The
control board does not need to be powered, and keeping the two separate
means a failed upload is a firmware problem rather than a wiring one.

Click Upload. Then open the Serial Monitor at **115200** and press the
board's reset button — you should see the startup banner.

---

## If something goes wrong

**"This chip is ESP32, not ESP32-S3"** (or similar mismatch)
Wrong board selected. The list contains several similarly named
entries. You want **XIAO_ESP32C3**, no suffix.

**Serial Monitor is blank**
USB CDC On Boot is Disabled. Change it and re-upload — the setting is
compiled into the sketch, so changing it alone does nothing.

**Garbled characters in Serial Monitor**
Wrong baud rate. Set the dropdown to 115200.

**Upload fails at "Connecting..."**
Hold the **BOOT** button on the XIAO, click Upload, and release once
the dots start moving. Some boards do not auto-reset into bootloader
mode reliably.

**Port does not appear**
Try a different USB-C cable. Charge-only cables carry no data and are
indistinguishable by eye. This is a more common cause than it should
be.

**XIAO_ESP32C3 is not in the board list**
The package did not install. Check the URL in Settings is exactly
right, then reopen Boards Manager and confirm esp32 shows as installed.

---

## After flashing

The fixture broadcasts a WiFi network called **`walrus`**, password
**`walrus12`**. Join it and browse to **http://192.168.4.1**.

Serial commands also work at 115200: `E60` extend seconds, `R10`
retract seconds, `C20` cycle count, `S` start, `X` stop, `D` restore
defaults.

See the main [README](../README.md) for operating instructions.