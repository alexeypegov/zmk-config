# ZMK config: Corne Wireless View

ZMK config for the Typeractive Corne Wireless View (nice!nano v2 + nice!view). The keymap is a port of the QMK Cantor keymap in `~/dev/qmk_userspace` and is meant to be used with the macOS `Russian Cantor 42` input source from that repo.

Firmware is built by GitHub Actions. Download the `firmware` artifact and flash `corne_left` / `corne_right` `.uf2` files to each half.

## Layers

- **Base**: QWERTY with one-shot Option/Shift/Ctrl, `Cmd`/`Enter`, `Lower`/`Tab`, `Raise`/`Bksp`, `Space`. `AltGr`/`;` and `Func`/`` ` `` on the right outer column.
- **Lower** (hold Tab thumb): numbers, shifted symbols, brackets, colon.
- **Raise** (hold Bksp thumb): navigation, editing, brightness, media, Mouseless triggers (`F13`/`F14`).
- **Func** (hold `` ` ``): F-keys, `F15`/`F16`, Caps Lock (`D`), lock screen, and Bluetooth:
  - `Z`–`B`: select Bluetooth profile 1–5
  - `F`: toggle USB/BLE output
  - `G`: clear the current profile's pairing
  - Top outer corners (`Tab` / `[`): bootloader (each half)
- **Mouse** (hold top-left `Tab`): move with `H` `J` `K` `L` (like the arrows on Raise), click with `U` (left) and `I` (right), scroll with `M` (up) and `,` (down), `App` on `Y`.

## Combos

| Keys | Output |
| --- | --- |
| `D`+`F` (or `3`+`4` on Lower) | `-` |
| `J`+`K` (after a typing pause) | `Esc` |
| `,`+`.` | `Shift+Option+/` → `?` |
| `M`+`,` | `Option+M` → `ъ` |
| `K`+`L` | `Cmd+Enter` |
| Both one-shot Shifts (bottom-left outer + right outer thumb) | Caps Word (English only: ж э б ю х aren't capitalized) |

## Differences from QMK

- Hold-taps use ZMK's `hold-preferred`, which is the closest match to QMK's `HOLD_ON_OTHER_KEY_PRESS`. Tapping terms are 200 ms, and 125 ms for the Lower/Raise thumbs.
- One-shot mods time out after 2 s. ZMK has no tap-toggle, so tapping a one-shot mod 3 times does not lock it.
- `J`+`K` → `Esc` uses `require-prior-idle-ms` instead of QMK's must-hold.
- The QMK RGB toggle is unbound, because there's no RGB.
- Func's Windows-style `Shift+Del` / `Ctrl+Ins` / `Shift+Ins` were dropped (macOS ignores them); Caps Lock moved from `Z` to `D` to make room for Bluetooth.
