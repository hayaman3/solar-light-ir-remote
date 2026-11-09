# Remote code table

Codes recorded with this firmware's **Received from your remotes** list.

## White solar light remote (blue buttons)

A common generic remote for solar flood lights, with a red button at the top and ON, OFF, a round brightness pad with M in the middle, AUTO, 3H, 5H and 8H.

**Protocol:** NEC · **Address:** `0x01` for every button

| Button | Command (hex) | Command (decimal) |
|---|---|---|
| Red button (top) | `0x00` | 0 |
| ON | `0x01` | 1 |
| Plus (+), left of pad | `0x02` | 2 |
| Brightness, half sun (bottom of pad) | `0x04` | 4 |
| Brightness, full sun (top of pad) | `0x05` | 5 |
| M (mode), center of pad | `0x06` | 6 |
| AUTO | `0x07` | 7 |
| OFF | `0x09` | 9 |
| Minus (−), right of pad | `0x0A` | 10 |
| 3H | `0x40` | 64 |
| 8H | `0x44` | 68 |
| 5H | `0x48` | 72 |

**Unused so far:** 3, 8, 11–63, 65–67, 69–71, 73 and up.

To send any of these from the board, use **Send one NEC code** with Address `0x01` and the command, or the scanner.

## Adding your remote

If you map another remote, please share it: open an issue or a pull request with a table like the one above, a short description or photo of the remote, and which light it came with.
