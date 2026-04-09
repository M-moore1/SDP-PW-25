import asyncio
import json

import websockets
from bleak import BleakClient, BleakScanner

HOST = "localhost"
PORT = 8080

DEVICE_NAME = "ROBOT_ESP32"
CHAR_UUID = "0000ff01-0000-1000-8000-00805f9b34fb"

connected_clients: set = set()


async def broadcast(message: str) -> None:
    if connected_clients:
        await asyncio.gather(*(ws.send(message) for ws in connected_clients))


MAX_RETRIES = 3


PAYLOAD_BYTES = 156
PACKET_BYTES = 160


def frame_payload(raw: bytes) -> bytes:
    """Wrap raw bytes in the robot's frame protocol: 0x0A 0xD0 [156 bytes padded] 0xDA 0x0D."""
    if len(raw) == PACKET_BYTES:
        return raw
    payload = raw[:PAYLOAD_BYTES].ljust(PAYLOAD_BYTES, b"\x00")
    return b"\x0a\xd0" + payload + b"\xda\x0d"


async def ble_inject(hex_data: str) -> dict:
    clean = hex_data.replace(" ", "")
    if len(clean) % 2 != 0 or not all(c in "0123456789abcdefABCDEF" for c in clean):
        return {"type": "ble_ack", "status": "error", "msg": "Invalid hex string"}

    data = frame_payload(bytes.fromhex(clean))

    for attempt in range(1, MAX_RETRIES + 1):
        print(f"[ble] Scanning for {DEVICE_NAME} (attempt {attempt}/{MAX_RETRIES})...")

        devices = await BleakScanner.discover(timeout=5.0)
        matches = [d for d in devices if d.name and DEVICE_NAME in d.name]

        if not matches:
            print("[ble] Device not found")
            if attempt < MAX_RETRIES:
                continue
            return {"type": "ble_ack", "status": "error", "msg": "Device not found"}

        for device in matches:
            print(f"[ble] Trying device: {device.address}")
            try:
                async with BleakClient(device.address, timeout=10.0) as client:
                    print(f"[ble] Connected: {client.is_connected}")
                    await client.write_gatt_char(CHAR_UUID, data, response=True)
                    print(f"[ble] Sent: {data.hex()}")
                    return {"type": "ble_ack", "status": "ok", "msg": f"Sent {len(data)} bytes"}
            except Exception as e:
                print(f"[ble] Failed on {device.address}: {e}")
                continue

        print(f"[ble] All addresses failed on attempt {attempt}")

    return {"type": "ble_ack", "status": "error", "msg": "All connection attempts failed"}


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
    async with websockets.serve(ws_handler, HOST, PORT):
        print(f"[ble-bridge] Listening on ws://{HOST}:{PORT}")
        print(f"[ble-bridge] Ready to inject into {DEVICE_NAME}")
        await asyncio.Future()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[ble-bridge] Shutting down.")
