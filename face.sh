#!/bin/bash
# Send a face command to the M5Dial over serial.
# Usage: ./face.sh [bot|eyes] <happy|sad|surprised|angry|excited|confused|thinking|winking|love|sleeping|cycle|off>
#        ./face.sh sound <done|success|question|error|alert|hello|bye|wow|sad|thinking|love|sleep|startup|beep|stop>
#        ./face.sh sound typing [seconds] [speed|slow|normal|fast|turbo]   (default 6 s, speed 1)
#        ./face.sh sound vol <0-255>
#        ./face.sh servo <1-3> <0-180> [deg/s]   (no speed = jump straight there)
#        ./face.sh arms thinking [deg/s] [min s] [max s] | arms off   (random arm, random pause; default 50 deg/s, 0.1-1 s)
#        ./face.sh pwm <40-330>        (servo PWM frequency in Hz, default 50)
#        ./face.sh reset               (all servos back to default)
#        ./face.sh rotate <0-3>        (0 = normal, 1 = 90° cw, 2 = 180°, 3 = 270°)
#        ./face.sh            (lists face names)   ./face.sh sound   (lists sounds)
# Override the port with PORT=/dev/cu.xxx ./face.sh sad
PORT="${PORT:-/dev/cu.usbmodem1101}"

if [ ! -e "$PORT" ]; then
    echo "Serial port $PORT not found (is the M5Dial plugged in?)" >&2
    exit 1
fi

exec 3<>"$PORT"
stty 115200 raw -echo <&3   # set while open, so the Mac never echoes the dial's replies back to it
if [ "$1" = "sound" ] || [ "$1" = "rotate" ] || [ "$1" = "servo" ] || [ "$1" = "arms" ] || [ "$1" = "reset" ] || [ "$1" = "pwm" ]; then
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
