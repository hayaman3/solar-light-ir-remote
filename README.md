# solar-light-ir-remote

Control a solar flood light or solar street light from your phone with a cheap ESP32 infrared (IR) board, and find the remote codes your light understands, even if you lost the original remote.

No soldering, no wiring to the light, and no USB-to-serial adapter needed. Everything is done over Wi-Fi from a phone or laptop browser.

**What it does**

- **Reads any IR remote** and shows its protocol, address and command (for example `NEC addr 0x1 cmd 0x9`).
- **Scans codes** on one address, one by one, so you can find buttons and hidden functions your remote doesn't have.
- **Saves buttons** you can tap from your phone to control the light.
- **Updates over Wi-Fi** from its own web page.

> **Why this exists:** many solar lights are sold with generic IR remotes that only partly work, and the original remote is often lost or never included. Most of these lights use the common **NEC** IR format at **38 kHz**, so a small board that can send any NEC code can stand in for the missing remote and find codes no remote of yours has.

## Contents

| Guide | What's in it |
|---|---|
| [Hardware](docs/hardware.md) | The board to buy, search terms, and what you need around it |
| [Setup guide](docs/setup.md) | Step-by-step: power, Wi-Fi, firmware upload, pin detection |
| [Mapping a remote and finding codes](docs/finding-codes.md) | Reading your remotes, the blink test, scanning |
| [Remote code table](docs/remote-codes.md) | Codes recorded from a common solar light remote |
| [Troubleshooting](docs/troubleshooting.md) | Problems we hit and how we solved them |
| [Firmware](firmware/) | Source code, build settings, ready-to-flash `.bin` |

## Quick start

1. Buy the **ESP32 IR transmitter/receiver board** (about ₱330 / US$6). See [Hardware](docs/hardware.md).
2. Power it with a USB-A to USB-C cable from a laptop or wall charger. The board has no power light.
3. Connect your phone or laptop to the board's **`IR_WIFI`** hotspot and open **http://192.168.4.1**.
4. Click **Update** and upload [`firmware/bin/ir-light-remote-v1.0.bin`](firmware/bin/).
5. Connect to `IR_WIFI` again, click **Configure WiFi**, and choose your home 2.4 GHz Wi-Fi.
6. Find the board's address on your network and open it in a browser.
7. Run **Detect receiver** and **Detect transmitter**, then **Save pins & restart**.

The [setup guide](docs/setup.md) explains each step, including what to do when something doesn't work.

## How it works

```
 Phone / laptop  ──Wi-Fi──▶  ESP32 IR board  ──infrared 38 kHz──▶  Solar light
 (browser page)              (this firmware)                        (IR receiver)
                                   ▲
                                   └── reads your old IR remotes
```

The board sits within a few meters of the light's IR receiver, where it can "see" it. Nothing is installed in or wired to the light, and the light's warranty is untouched. You control the board from anywhere your Wi-Fi reaches.

## Status

Working: reading remotes, pin auto-detection, sending and saving codes, code scanning, Wi-Fi firmware updates.

In progress: finding the code that switches a solar light out of automatic dusk-to-dawn mode, and its 12-hour timer. See [Finding codes](docs/finding-codes.md#what-we-learned-about-solar-light-behavior).

## Built with

- [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) for IR decoding and sending
- [WiFiManager](https://github.com/tzapu/WiFiManager) for Wi-Fi setup
- [Arduino core for ESP32](https://github.com/espressif/arduino-esp32)

## License

MIT. See [LICENSE](LICENSE).

---

Keywords: solar light remote control replacement, solar flood light IR remote codes, ESP32 IR blaster, NEC remote code scanner, lost solar light remote, ESP32_IR_TR_WIFI.
