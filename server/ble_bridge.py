import asyncio
import json

import websockets
from bleak import BleakClient, BleakScanner

HOST = "localhost"
PORT = 8080

DEVICE_NAME = "ROBOT_ESP32"
CHAR_UUID = "0000ff01-0000-1000-8000-00805f9b34fb"

connected_clients: set = set()
ble_client: BleakClient | None = None


async def broadcast(message: str) -> None:
    if connected_clients:
        await asyncio.gather(*(ws.send(message) for ws in connected_clients))


async def connect_ble() -> BleakClient | None:
    """Scan for the robot and establish a persistent connection."""
    global ble_client

    if ble_client and ble_client.is_connected:
        return ble_client

    for attempt in range(1, 4):
        print(f"[ble] Scanning for {DEVICE_NAME} (attempt {attempt}/3)...")
        devices = await BleakScanner.discover(timeout=5.0)
        matches = [d for d in devices if d.name and DEVICE_NAME in d.name]

        if not matches:
            print("[ble] Device not found")
            continue

        for device in matches:
            print(f"[ble] Trying device: {device.address}")
            try:
                client = BleakClient(device.address, timeout=10.0)
                await client.connect()
                print(f"[ble] Connected: {client.is_connected}")
                ble_client = client
                return ble_client
            except Exception as e:
                print(f"[ble] Failed on {device.address}: {e}")
                continue

    return None


async def ble_inject(hex_data: str) -> dict:
    clean = hex_data.replace(" ", "")
    if len(clean) % 2 != 0 or not all(c in "0123456789abcdefABCDEF" for c in clean):
        return {"type": "ble_ack", "status": "error", "msg": "Invalid hex string"}

    data = bytes.fromhex(clean)

    client = await connect_ble()
    if not client:
        return {"type": "ble_ack", "status": "error", "msg": "Could not connect to device"}

    try:
        await client.write_gatt_char(CHAR_UUID, data, response=True)
        print(f"[ble] Sent: {data.hex()}")
        return {"type": "ble_ack", "status": "ok", "msg": f"Sent {len(data)} bytes"}
    except Exception as e:
        print(f"[ble] Write failed: {e}, reconnecting...")
        global ble_client
        ble_client = None
        try:
            await client.disconnect()
        except Exception:
            pass
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
    print(f"[ble-bridge] Connecting to {DEVICE_NAME} on startup...")
    client = await connect_ble()
    if client:
        print(f"[ble-bridge] Persistent BLE connection established")
    else:
        print(f"[ble-bridge] WARNING: Could not connect to {DEVICE_NAME}, will retry on first inject")

    async with websockets.serve(ws_handler, HOST, PORT):
        print(f"[ble-bridge] Listening on ws://{HOST}:{PORT}")
        await asyncio.Future()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        if ble_client:
            print("[ble-bridge] Disconnecting BLE...")
            asyncio.run(ble_client.disconnect())
        print("\n[ble-bridge] Shutting down.")
