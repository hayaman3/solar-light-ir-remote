# Setup guide

This takes about 20 minutes. You need the board, a USB-A to USB-C cable, and a phone or laptop.

## 1. Power the board

1. Plug the board into a **laptop USB-A port** or a **wall charger**, using a USB-A to USB-C cable.
2. Wait 15–20 seconds.

The board has no power light. To check that it's on, look for a new Wi-Fi network called **`IR_WIFI`** on your phone or laptop.

> Don't expect the board to appear in Windows Device Manager. Its USB-C port only supplies power.

## 2. Open the stock setup page

1. Connect your phone or laptop to **`IR_WIFI`** (no password).
2. A setup page called **WiFiManager** usually opens by itself. If it doesn't, open a browser and go to **http://192.168.4.1**. Type `http://` so the browser doesn't try `https`.

You'll see **Configure WiFi**, **Info**, **Exit** and **Update**.

## 3. Upload this project's firmware

Do this **before** connecting the board to your home Wi-Fi. Once the stock firmware joins your Wi-Fi, it turns off `IR_WIFI` and its web pages. If that already happened, see [Getting IR_WIFI back](troubleshooting.md#ir_wifi-disappeared-and-the-old-firmware-is-still-on).

1. Download [`ir-light-remote-v1.0.bin`](../firmware/bin/ir-light-remote-v1.0.bin) to your device **before** connecting to `IR_WIFI`, since that network has no internet.
2. On the WiFiManager menu at **http://192.168.4.1**, click **Update**.
   - On iPhone or iPad, the automatic pop-up window often can't pick files. Close it and use Safari instead.
3. Choose the `.bin` file and click **Update**.
4. Wait about 30 seconds without unplugging. You'll see **Update successful. Device rebooting now…**

## 4. Connect the board to your home Wi-Fi

After the update, the board restarts.

- If it was already set up with your Wi-Fi, it rejoins it on its own. Skip to step 5.
- Otherwise it opens **`IR_WIFI`** again. Connect to it, open **http://192.168.4.1**, click **Configure WiFi**, choose your **2.4 GHz** home network, enter the password and save.

When it connects, `IR_WIFI` disappears. That's expected.

> If the board can't reach your Wi-Fi for 3 minutes, it opens a fallback hotspot called **`IR_REMOTE`**. Connect to it and open **http://192.168.4.1** to use the remote page without home Wi-Fi.

## 5. Find the board on your network

With your phone or laptop back on home Wi-Fi, try these in order:

1. **http://irremote.local** (works on most iPhones, Macs and recent Windows).
2. Your router's app or admin page: look for a device called `irremote` or `esp32-…` in the connected devices list.
3. On Windows, find it from Command Prompt. Run these **one at a time**:

   ```
   ipconfig
   ```
   Note the first three parts of your **IPv4 Address**, for example `192.168.1`. Then:

   ```
   for /L %i in (1,1,254) do @ping -n 1 -w 100 192.168.1.%i >nul
   ```
   Replace `192.168.1` with yours. This takes 1–2 minutes and shows nothing while it runs. Then:

   ```
   arp -a | findstr 20-50-0d
   ```
   `20-50-0d` is the start of this board's hardware address. Yours may differ; if nothing shows, run `arp -a` and look for a new entry.

Open **http://** plus the address you found, for example `http://192.168.1.7`.

> The address can change after a restart. To keep it fixed, set an **address reservation** (DHCP reservation) for the board in your router.

## 6. Find the IR pins (one time)

The page opens on **Step 1 · Find the IR pins**. Have an old IR remote ready.

1. Click **Detect receiver**. When the page says **NOW press remote buttons repeatedly!**, point the remote at the board's black sensor from close up and keep pressing buttons for about 6 seconds.
   - On our board: **Receiver found on GPIO14**.
2. Click **Detect transmitter**. You don't need to do anything; the board flashes its own IR LED and listens with its own receiver.
   - On our board: **Transmitter found on GPIO4**.
3. Check that the **RX** and **TX** boxes are filled in, then click **Save pins & restart**.

No remote at all? Type `14` and `4` into the manual boxes. Those are the pins on the `ESP32_IR_TR_WIFI_V1.1` board.

## 7. You're set up

The page now shows:

- **Saved buttons**: your own buttons, sent with one tap.
- **Code scanner**: sends commands one by one on an address.
- **Send one NEC code**: sends a single address and command.
- **Received from your remotes**: everything the board hears.
- **Settings**: redo pin setup, firmware update, forget Wi-Fi.

Next: [Mapping a remote and finding codes](finding-codes.md).

## Updating firmware later

Open the board's page, then **Settings → Firmware update**, or go to `http://<board address>/update`. Choose the new `.bin` and click **Upload**. Wi-Fi settings, pins and saved buttons are kept.
