# Hardware

## The board

**ESP32 IR transmitter and receiver board**, sold as "WiFi Transparent Transmission ESP Development Module ESP8266/ESP32 IR Transmitter&Receiver For Smart Home Control".

| Detail | Value |
|---|---|
| Chip | ESP32-32E N4 (4 MB flash) |
| Silkscreen on the back | `ESP32_IR_TR_WIFI_V1.1` |
| Size | 33 × 33 mm |
| Power | 5 V through USB-C (**power only**, no data) |
| IR receiver | 38 kHz (VS1838-type), on **GPIO14** |
| IR LED | 940 nm, on **GPIO4** |
| Programming header | 6 holes: `GND 5V RX TX IO0 GND` (unsoldered) |
| Stated IR range | about 5 m |
| In the bag | the board, a 6-pin header, a jumper cap |
| Price paid | about ₱330 (Philippines, Shopee) |

> The listing offers an **ESP32** and an **ESP8266** variant. This project uses the **ESP32** one. The seller's photos and instructions mostly show the ESP8266 version; the ESP32 version behaves the same way.

**Search terms:** `ESP32 IR transmitter receiver WiFi module`, `ESP32 IR TR WIFI`, `WiFi transparent transmission ESP32 IR`.

### What it ships with

The board comes with stock firmware that:

- Opens a Wi-Fi hotspot called **`IR_WIFI`** (no password) with a setup page at **http://192.168.4.1** (WiFiManager v2.0.17).
- Forwards received NEC codes over UDP and sends NEC codes it receives over UDP.
- Has an **Update** button on the setup page, which is how we install this project's firmware without any cable.

## What else you need

- A **USB-A to USB-C cable** and a USB-A port (laptop, wall charger). See the note on power banks below.
- A **phone or laptop** with a web browser.
- **2.4 GHz Wi-Fi**. The ESP32 can't use 5 GHz networks.
- Optional: one of your old IR remotes, for pin detection and for reading codes.

### Power notes

- The board has **no power LED**, so it looks dead even when it works. Check for the `IR_WIFI` network instead.
- **Power banks may switch off** because the board draws very little current.
- **USB-C to USB-C chargers may not power it at all.** Use USB-A to USB-C.

## Range and placement

- The board has to point at the light's IR receiver, usually a small dark window on the underside of the light.
- The stated range is about 5 m. For a light two floors up, put the board below the light pointing up, or close to it.
- IR works best at night and with nothing in between.

## Alternatives

If you need more range or want to build your own:

- **ESP32 DevKit** (with built-in USB programming) + **KY-022** IR receiver + **940 nm IR LED** driven by a **2N2222** transistor. Reaches about 8–10 m. Needs a breadboard but no soldering.
- **Flipper Zero**: reads and sends IR without any coding, but costs much more.

Learning-only IR blasters (Broadlink, Tuya/Smart Life Wi-Fi IR remotes) can copy buttons from a remote you have, but **can't scan for codes you don't have**.
