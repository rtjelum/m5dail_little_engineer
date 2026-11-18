# M5Dial desk robot

Firmware for an M5Stack Dial desk robot (face, sounds, arm servos). See README.md.

## Build

Always use the Makefile: `make all` (compile + flash), `make compile`, `make flash`. Board FQBN is `m5stack:esp32:m5stack_dial`, port `/dev/cu.usbmodem1101`.

## Dial shows Claude's state

Hooks in `.claude/settings.json` call `claude_dial.sh`, which sends a mood to the dial with `face.sh` (skipped when the dial isn't plugged in):

| Claude is...                                   | Dial shows |
|------------------------------------------------|------------|
| working on a prompt / running tools            | `thinking` (thinking face with short detours to other faces + typing arms) |
| asking a question or waiting for a permission  | `question` |
| finished a reply                               | `done`     |
| idle for a while, or the session ended         | `sleep`    |

The faces use the style saved on the dial (`./face.sh eyes`, `./face.sh bot` or `./face.sh toon` changes it).

The hooks do this automatically, so don't run `face.sh` for these states yourself. While you have the serial port open (e.g. `arduino-cli monitor`), the hooks can't send. Flashing works as normal.
