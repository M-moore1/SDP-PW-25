import asyncio
import json
import shutil
import subprocess
import sys
import os

import websockets
from bleak import BleakClient, BleakScanner

# Allow importing from ubertooth/script.py
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "ubertooth"))
from script import opcode_name

HOST = "localhost"
PORT = 8080

OPCODE_FILTER = None

DEVICE_NAME = "ROBOT_ESP32"
CHAR_UUID = "0000ff01-0000-1000-8000-00805f9b34fb"

connected_clients: set = set()


def find_tool(name: str) -> str:
    path = shutil.which(name)
    if not path:
        sys.exit(f"Error: '{name}' not found. Make sure it is installed and on your PATH.")
    return path


def parse_line(line: str) -> dict | None:
    """Parse a tshark tab-separated line into a packet dict, or None if invalid."""
    parts = line.strip().split("\t")
    while len(parts) < 3:
        parts.append("")

    frame_num, opcode, value = parts[0].strip(), parts[1].strip(), parts[2].strip()

    if not value:
        return None

    return {
        "frame": frame_num,
        "opcode": opcode_name(opcode),
        "value": value,
    }
    

async def broadcast(message: str) -> None:
    if connected_clients:
        await asyncio.gather(*(ws.send(message) for ws in connected_clients))


async def capture_loop() -> None:
    ubertooth = find_tool("ubertooth-btle")
    tshark = find_tool("tshark")

    pcap_file = "/tmp/ubertooth_live.pcap"
    if os.path.exists(pcap_file):
        os.remove(pcap_file)

    print("[capture] Starting ubertooth-btle | tshark pipeline...")

    ubertooth_proc = subprocess.Popen(
        ["sudo", ubertooth, "-f", "-c", pcap_file],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    # Wait for ubertooth to create the file
    await asyncio.sleep(2)
    print("[capture] Pipeline running, polling for packets...")

    loop = asyncio.get_event_loop()
    try:
        while True:
            # Run tshark on the current file, skipping already-seen frames
            result = await loop.run_in_executor(None, lambda: subprocess.run(
                [tshark, "-r", pcap_file, "-V"],
                capture_output=True,
                text=True,
            ))

            for line in result.stdout.splitlines():
                print(line)
                await broadcast(line)

            await asyncio.sleep(1)

    finally:
        ubertooth_proc.terminate()
        if os.path.exists(pcap_file):
            os.remove(pcap_file)
        print("[capture] Pipeline stopped.")


async def ble_inject(hex_data: str) -> dict:
    """Scan for the robot, connect, and write raw bytes to its GATT characteristic."""
    clean = hex_data.replace(" ", "")
    if len(clean) % 2 != 0 or not all(c in "0123456789abcdefABCDEF" for c in clean):
        return {"type": "ble_ack", "status": "error", "msg": "Invalid hex string"}

    data = bytes.fromhex(clean)
    print(f"[ble] Scanning for {DEVICE_NAME}...")

    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: d.name and DEVICE_NAME in d.name,
        timeout=5.0,
    )

    if not device:
        print("[ble] Device not found")
        return {"type": "ble_ack", "status": "error", "msg": "Device not found"}

    print(f"[ble] Found device: {device.address}")

    try:
        async with BleakClient(device.address) as client:
            print(f"[ble] Connected: {client.is_connected}")
            await client.write_gatt_char(CHAR_UUID, data, response=True)
            print(f"[ble] Sent: {data.hex()}")
            return {"type": "ble_ack", "status": "ok", "msg": f"Sent {len(data)} bytes"}
    except Exception as e:
        print(f"[ble] Write failed: {e}")
        return {"type": "ble_ack", "status": "error", "msg": str(e)}


async def ws_handler(websocket) -> None:
    connected_clients.add(websocket)
    print(f"[ws] Client connected ({len(connected_clients)} total)")
    try:
        async for raw in websocket:
            try:
                msg = json.loads(raw)
            except json.JSONDecodeError:
                continue

            if msg.get("type") == "ble_inject":
                hex_data = msg.get("data", "")
                print(f"[ws] BLE inject request: {hex_data}")
                result = await ble_inject(hex_data)
                await broadcast(json.dumps(result))
    finally:
        connected_clients.discard(websocket)
        print(f"[ws] Client disconnected ({len(connected_clients)} total)")


async def main() -> None:
    capture_available = shutil.which("ubertooth-btle") and shutil.which("tshark")

    async with websockets.serve(ws_handler, HOST, PORT):
        print(f"[ws] Server listening on ws://{HOST}:{PORT}")

        if capture_available:
            await capture_loop()
        else:
            print("[capture] ubertooth-btle or tshark not found — sniffing disabled, BLE injection still active")
            await asyncio.Future()  # run forever


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[server] Shutting down.")
