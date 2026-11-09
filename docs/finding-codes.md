# Mapping a remote and finding codes

## How IR remote codes work

Almost every IR remote uses the same **38 kHz** carrier, so frequency isn't what tells remotes apart. The **code** is. With the common NEC format, a code has three parts:

| Part | Meaning | Example |
|---|---|---|
| **Protocol** | The signal's "language" | NEC |
| **Address** | Which device the remote is for, like a name tag | `0x01` |
| **Command** | Which button was pressed | `0x09` |

- **Same protocol, address and command** = the exact same signal. The light can't tell two remotes apart if they send the same code.
- **Same address, different command** = same device family, a different button. Hidden functions live here.
- **Different address** = meant for another device. The light most likely ignores it.

## 1. Map your remote

1. Open the board's page and scroll to **Received from your remotes**.
2. Hold the remote about 30 cm from the board's black sensor.
3. Press **one button once** and wait a second. A line appears, for example `NEC addr 0x1 cmd 0x9 (9)`.
4. Click **Save** on that line and name it after the button, for example `OFF`.
5. Repeat for every button, one at a time.

Tips:

- Holding a button down logs it only once.
- The list keeps the last 20 presses, so save as you go.
- If two buttons show the same code, press them again separately. We once recorded 8H wrongly this way.

Write the codes down. See our [remote code table](remote-codes.md) for an example.

## 2. Check that the board reaches the light

1. Put the board where it can see the light's IR receiver, within a few meters.
2. In **Send one NEC code**, enter the remote's address and the code for a button that works, then click **Send**.

Many solar lights **blink once** when they accept a command, then go back to whatever their light sensor says. That blink is your feedback. If you see it, the board reaches the light.

## 3. The blink test

The light can't send anything back to the board, so all feedback is visual. A blink means **the light understood the code**, even in daytime.

Send each mapped code and note which ones blink. Codes that don't blink are probably not used by your light, even if the button exists on the remote.

## 4. Scan for unknown codes

In **Code scanner**:

1. Set **Address** to your remote's address (ours is `0x01`).
2. Set **From** and **To**. Start with the gaps near known codes. A full scan is `0` to `255`.
3. Set **Delay ms**. 2000 ms gives you time to react.
4. Click **Start** and watch the light.

When the light reacts:

- Click **Pause**, then use **◀ Prev** and **Resend** to find which command did it. It's usually the last or second-to-last one sent.
- Click **Save last** and name it.

### Recording a video instead of watching

For a light that's hard to watch (for example two floors up):

1. Set a known delay, for example 2000 ms.
2. Start recording the light on your phone, then click **Start**.
3. Afterward, scrub through the video. For each blink:

   **command = From + (seconds after Start ÷ delay in seconds)**

   With From `0` and a 2-second delay, a blink 18 seconds in is command 9.

A full 0–255 scan at 2000 ms takes about 8.5 minutes.

### Where to look first

Cheap remote chips number buttons by their position on the circuit board, so codes come in groups with gaps. On our remote:

- The main buttons are **0–10**, with **3** and **8** unused.
- The timers are **64, 68, 72**, going up in steps of 4, so **76** is the next spot in that pattern.

Scan the gaps first: **0–15** and **64–79** on the remote's address.

## What we learned about solar light behavior

These are observations from one light, not rules for every model:

- **The light's own dusk-to-dawn sensor wins.** After a command blinks, the light returns to on or off based on daylight. OFF doesn't keep it off at night.
- **Runtime depends on the battery.** After a few cloudy days, the light runs only about 6 hours. It ran about 12 hours on some nights, which may have been AUTO with a fuller battery rather than a specific button.
- **Brightness buttons seemed to do nothing.** Low battery or automatic mode can lock brightness.
- **3H, 5H and 8H didn't blink**, so this light probably doesn't use those codes. Its timer, or a manual-mode switch, is likely on codes our remotes don't have.

Things worth trying, at night with the light on: **M** then **OFF**, pressing **M** repeatedly, **AUTO** then **OFF**, and the blink scan above. Test after sunny days so a low battery doesn't confuse the results.

> **Battery warning:** don't force a light back on when its controller turns it off because the battery is low. Deep discharge shortens battery life. Clean the solar panel or move it somewhere sunnier instead.
