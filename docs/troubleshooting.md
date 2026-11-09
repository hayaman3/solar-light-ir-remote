# Troubleshooting

## The board looks dead

- It has **no power LED**. Check for the `IR_WIFI` network, or for the board on your Wi-Fi, instead.
- Use a **USB-A to USB-C** cable from a laptop or wall charger. USB-C to USB-C chargers may not power it, and power banks may turn off because it draws so little.

## The board doesn't appear in Device Manager

Expected. Its USB-C port is **power only**. Firmware is uploaded over Wi-Fi instead. Programming over a cable would need a USB-to-TTL adapter on the 6-pin header.

## The update page won't open

- Make sure you're still connected to **`IR_WIFI`**. Windows sometimes switches away because the network has no internet.
- Type **`http://192.168.4.1`** with `http://`, then click the **Update** button. Going straight to `/u` didn't work for us.
- On iPhone or iPad, close the automatic pop-up and use Safari; the pop-up often can't pick files.

## IR_WIFI disappeared and the old firmware is still on

Once the stock firmware connects to your home Wi-Fi, it turns off `IR_WIFI` **and** its web pages, so you can't reach the update page on your home network either.

What worked for us: **make it unable to find your Wi-Fi.**

1. Download the `.bin` to your laptop first.
2. Unplug the board.
3. Turn off your **main router** (unplug it).
4. Plug the board back in and wait 1–2 minutes. `IR_WIFI` comes back.
5. Connect to `IR_WIFI`, open **http://192.168.4.1**, click **Update**, and upload the firmware.
6. Turn the router back on. The board rejoins your Wi-Fi with the new firmware.

The seller's instructions say that **shorting TX and RX while powering on** clears the saved Wi-Fi. A paperclip across those holes didn't work for us on the ESP32 version.

## I can't find the board's address

- Try **http://irremote.local**.
- Check your router's connected devices list.
- On Windows, run the ping sweep and `arp -a` commands from the [setup guide](setup.md#5-find-the-board-on-your-network). Run them **one at a time**; if they get pasted onto one line, the second command never runs.
- The address can change after a restart. Set a DHCP reservation in your router to keep it fixed.

## `ping esp32-…` says it can't find the host

Common on home routers, which don't always publish device names. Use the `arp` method instead.

## Detect receiver finds nothing

- Press buttons **during** the 6-second "NOW" window, not before.
- Hold the remote close (about 30 cm) and point it at the black sensor.
- Check the remote works: look at its LED through your phone's **front** camera while pressing a button. A purple flicker means it's sending.

## Detect transmitter finds nothing

Use the manual **Blink test**: enter a pin, click **Blink**, and watch the clear IR LED through your phone's front camera. A purple flicker means that's the transmitter pin. On `ESP32_IR_TR_WIFI_V1.1` it's **GPIO4**.

## The light doesn't react

- Move the board closer and point it straight at the light's IR receiver.
- Check the address. A light ignores codes for a different address.
- Watch for a **blink** rather than a lasting change. Many solar lights blink to accept a command and then follow their own light sensor.
- Test at night, after sunny days. A low battery can make the light ignore commands.

## The light's original remotes stopped working

Before blaming the remote:

- Check the remote with your phone's front camera, or point it at this board and see if codes appear.
- Check whether the light still turns on by itself at night. If not, the battery or panel is the problem.
- Clean the light's IR receiver window.
