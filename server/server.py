import asyncio
import json
import shutil
import subprocess
import sys
import os

import websockets

# Allow importing from ubertooth/script.py
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "ubertooth"))
from script import opcode_name

HOST = "localhost"
PORT = 8080

OPCODE_FILTER = None

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


async def ws_handler(websocket) -> None:
    connected_clients.add(websocket)
    print(f"[ws] Client connected ({len(connected_clients)} total)")
    try:
        await websocket.wait_closed()
    finally:
        connected_clients.discard(websocket)
        print(f"[ws] Client disconnected ({len(connected_clients)} total)")


async def main() -> None:
    async with websockets.serve(ws_handler, HOST, PORT):
        print(f"[ws] Server listening on ws://{HOST}:{PORT}")
        await capture_loop()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[server] Shutting down.")
