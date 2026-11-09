# Firmware

| File | What it is |
|---|---|
| [`bin/ir-light-remote-v1.0.bin`](bin/ir-light-remote-v1.0.bin) | Ready to upload over Wi-Fi (app image, about 970 KB) |
| [`irremote/irremote.ino`](irremote/irremote.ino) | Source code (Arduino sketch) |

SHA-256 of `ir-light-remote-v1.0.bin`:
`1c621d722560a05eb4c3b736e954495446fbb7da27e10254a3cb6d5b587f5eb0`

## Installing

Upload the `.bin` from the stock firmware's WiFiManager **Update** page, or later from this firmware's **Settings → Firmware update**. See the [setup guide](../docs/setup.md).

The `.bin` is an **app image** for over-the-air updates. It isn't a full flash image, so don't write it to address 0x0 with a serial flasher.

## Features

- WiFiManager setup hotspot `IR_WIFI` (3-minute timeout), then fallback hotspot `IR_REMOTE` at 192.168.4.1
- mDNS name `irremote.local`
- IR pin auto-detection (receiver by counting signal edges while you press a remote, transmitter by listening with the board's own receiver)
- Decodes any protocol IRremoteESP8266 supports; keeps the last 20 received codes
- NEC scanner with start, pause, resume, stop, previous, next, resend and save-last
- Saved buttons stored in flash (kept across updates and restarts)
- Web firmware update at `/update`

## Building from source

Tested with:

| Item | Version |
|---|---|
| Arduino core for ESP32 | 2.0.14 |
| Board setting | **ESP32 Dev Module**, default partition scheme (needs two OTA slots) |
| [WiFiManager](https://github.com/tzapu/WiFiManager) | 2.0.17 |
| [IRremoteESP8266](https://github.com/crankyoldgit/IRremoteESP8266) | 2.8.6 |

### Arduino IDE

1. Add the ESP32 boards URL in **Preferences → Additional boards manager URLs**:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
2. Install **esp32 by Espressif Systems** version 2.0.14 in the Boards Manager.
3. Install **WiFiManager** (tzapu) and **IRremoteESP8266** in the Library Manager.
4. Open `irremote/irremote.ino`, select **ESP32 Dev Module**.
5. **Sketch → Export Compiled Binary.** Upload the resulting `irremote.ino.bin` from the board's update page.

### arduino-cli

```
arduino-cli core install esp32:esp32@2.0.14 \
  --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli lib install "WiFiManager@2.0.17" "IRremoteESP8266@2.8.6"
arduino-cli compile -b esp32:esp32:esp32 --output-dir build irremote
```

## Web API

The page uses these endpoints, which you can also call from scripts on the same network:

| Endpoint | Does |
|---|---|
| `GET /api/status` | JSON: version, pins, scanner state, received log, saved button names |
| `GET /api/sendnec?addr=0x01&cmd=0x09&rep=1` | Send one NEC code |
| `GET /api/scan?action=start&addr=0x01&from=0&to=255&ms=2000&rep=1` | Start a scan (`pause`, `resume`, `stop`, `step&d=-1|0|1` also work) |
| `GET /api/sendsaved?i=0` | Send saved button number 0 |
| `GET /api/savenec?name=OFF&addr=0x01&cmd=0x09` | Save an NEC button |
| `GET /api/pins?rx=14&tx=4` | Set pins and restart |
