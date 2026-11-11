#!/usr/bin/env python3
# Two-way BLE terminal for the robot. Same commands as serial_monitor.py.
# The robot has one characteristic: write a command, get output as notifications, or read it
# to get the full output of the last command. Type "?" to do a read.
import asyncio, sys
from bleak import BleakScanner, BleakClient

NAME = "M5Dial-Robot"
CMD = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"   # write + read + notify

async def main():
    print(f"Scanning for {NAME}...")
    dev = await BleakScanner.find_device_by_name(NAME, timeout=15)
    if not dev:
        sys.exit(f"{NAME} not found")
    async with BleakClient(dev) as c:
        await c.start_notify(CMD, lambda _, b: print(b.decode("utf-8", errors="replace"), end="", flush=True))
        print(f"Connected to {NAME}. Type commands, Enter to send, ? to read the last reply. Ctrl-C to quit.\n")
        loop = asyncio.get_running_loop()
        while c.is_connected:
            line = await loop.run_in_executor(None, sys.stdin.readline)
            if not line:   # end of input
                await asyncio.sleep(1); break
            cmd = line.strip()
            if cmd == "?":
                print("[read] " + (await c.read_gatt_char(CMD)).decode("utf-8", errors="replace"), end="", flush=True)
            elif cmd:
                await c.write_gatt_char(CMD, cmd.encode(), response=True)

try:
    asyncio.run(main())
except KeyboardInterrupt:
    print("\nDisconnected.")
