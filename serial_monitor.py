#!/usr/bin/env python3
import serial, threading, sys

PORT = "/dev/cu.usbmodem1101"
BAUD = 115200

def reader(ser):
    while True:
        try:
            line = ser.readline().decode("utf-8", errors="replace")
            if line:
                print(line, end="", flush=True)
        except Exception:
            break

ser = serial.Serial(PORT, BAUD, timeout=1)
print(f"Connected to {PORT} @ {BAUD}. Type commands, Enter to send. Ctrl-C to quit.\n")
t = threading.Thread(target=reader, args=(ser,), daemon=True)
t.start()
try:
    while True:
        cmd = input()
        ser.write((cmd + "\n").encode())
except KeyboardInterrupt:
    print("\nDisconnected.")
    ser.close()
