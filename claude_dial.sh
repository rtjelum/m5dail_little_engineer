#!/bin/bash
# Show what Claude Code is doing on the M5Dial. Called from the hooks in .claude/settings.json.
# Usage: ./claude_dial.sh <thinking|question|done|sleep>
# Does nothing if the dial isn't plugged in, and skips repeats of the state already shown.
DIR="$(cd "$(dirname "$0")" && pwd)"
PORT="${PORT:-/dev/cu.usbmodem1101}"
STATE_FILE="${TMPDIR:-/tmp}/m5dial_claude_state"
LOCK="${TMPDIR:-/tmp}/m5dial_claude.lock"

state="$1"
case "$state" in thinking|question|done|sleep) ;; *) exit 0 ;; esac
cat >/dev/null                                  # drain the hook's JSON on stdin
[ -e "$PORT" ] || exit 0                        # dial not connected
[ "$(cat "$STATE_FILE" 2>/dev/null)" = "$state" ] && exit 0

# One sender at a time on the serial port (face.sh holds it for ~1-2 s).
for _ in $(seq 1 30); do mkdir "$LOCK" 2>/dev/null && break; sleep 0.1; done
trap 'rmdir "$LOCK" 2>/dev/null' EXIT
echo "$state" > "$STATE_FILE"
"$DIR/face.sh" "$state" >/dev/null 2>&1 || rm -f "$STATE_FILE"
exit 0
