#!/bin/bash
# Send a face command to the M5Dial over serial.
# Usage: ./face.sh [bot|eyes] <happy|sad|surprised|angry|excited|confused|thinking|winking|love|sleeping|cycle|off>
#        ./face.sh sound <done|success|question|error|alert|hello|bye|wow|sad|thinking|love|sleep|startup|beep|stop>
#        ./face.sh sound typing [seconds] [speed|slow|normal|fast|turbo]   (default 6 s, speed 1)
#        ./face.sh sound vol <0-255>
#        ./face.sh            (lists face names)   ./face.sh sound   (lists sounds)
# Override the port with PORT=/dev/cu.xxx ./face.sh sad
PORT="${PORT:-/dev/cu.usbmodem1101}"

if [ ! -e "$PORT" ]; then
    echo "Serial port $PORT not found (is the M5Dial plugged in?)" >&2
    exit 1
fi

stty -f "$PORT" 115200 raw -echo 2>/dev/null || stty -F "$PORT" 115200 raw -echo
exec 3<>"$PORT"
if [ "$1" = "sound" ]; then
    printf '%s\n' "$*" >&3
else
    printf 'face %s\n' "$*" >&3
fi

# Print the device's reply (stops after 1 s of silence)
reply=""
end=$((SECONDS + 2))
while IFS= read -r -t 1 line <&3; do
    reply+="${line%$'\r'}"$'\n'
    [ $SECONDS -ge $end ] && break
done
exec 3>&-
printf '%s' "$reply"
