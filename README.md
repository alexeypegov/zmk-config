# ZMK config: Corne Wireless View

ZMK config for the Typeractive Corne Wireless View (nice!nano v2 + nice!view). The keymap is a port of the QMK Cantor keymap in `~/dev/qmk_userspace` and is meant to be used with the macOS `Russian Cantor 42` input source from that repo.

Firmware is built by GitHub Actions. Download the `firmware` artifact and flash `corne_left` / `corne_right` `.uf2` files to each half.

## Layers

- **Base**: QWERTY with one-shot Option/Shift/Ctrl, `Cmd`/`Enter`, `Lower`/`Tab`, `Raise`/`Bksp`, `Space`. `AltGr`/`;` and `Func`/`` ` `` on the right outer column.
- **Lower** (hold Tab thumb): numbers, shifted symbols, brackets, colon.
- **Raise** (hold Bksp thumb): navigation, editing, brightness, media, Mouseless triggers (`F13`/`F14`).
- **Func** (hold `` ` ``): F-keys, `F15`/`F16`, Caps Lock, copy/paste shortcuts, lock screen.
- **Mouse**: mouse movement and buttons; no key activates it yet.
- **Adjust** (hold Tab thumb + Bksp thumb):
  - `Q`–`T`: select Bluetooth profile 1–5
  - `A`: clear the current profile's pairing
  - `S`: toggle USB/BLE output
  - Bottom outer corners: bootloader (each half)

## Combos

| Keys | Output |
| --- | --- |
| `D`+`F` (or `3`+`4` on Lower) | `-` |
| `J`+`K` (after a typing pause) | `Esc` |
| `,`+`.` | `Shift+Option+/` → `?` |
| `M`+`,` | `Option+M` → `ъ` |
| `K`+`L` | `Cmd+Enter` |

## Differences from QMK

- Hold-taps use ZMK's `hold-preferred`, which is the closest match to QMK's `HOLD_ON_OTHER_KEY_PRESS`. Tapping terms are 200 ms, and 125 ms for the Lower/Raise thumbs.
- One-shot mods time out after 2 s. ZMK has no tap-toggle, so tapping a one-shot mod 3 times does not lock it.
- `J`+`K` → `Esc` uses `require-prior-idle-ms` instead of QMK's must-hold.
- The QMK RGB toggle is unbound, because there's no RGB.
