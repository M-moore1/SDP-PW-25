import asyncio
import json
import os
import shutil
import sys

import websockets

HOST = "localhost"
PORT = 8765

connected_clients: set = set()


def find_tool(name: str) -> str:
    path = shutil.which(name)
    if not path:
        sys.exit(f"Error: '{name}' not found. Make sure it is installed and on your PATH.")
    return path


async def broadcast(message: str) -> None:
    if connected_clients:
        await asyncio.gather(*(ws.send(message) for ws in connected_clients))


async def capture_loop() -> None:
    ubertooth = find_tool("ubertooth-btle")
    tshark = find_tool("tshark")

    fifo_path = "/tmp/ubertooth_live.pcap"
    if os.path.exists(fifo_path):
        os.remove(fifo_path)
    os.mkfifo(fifo_path)

    print("[capture] Starting ubertooth-btle...")

    ubertooth_proc = await asyncio.create_subprocess_exec(
        "sudo", ubertooth, "-f", "-c", fifo_path,
        stdout=asyncio.subprocess.DEVNULL,
        stderr=asyncio.subprocess.DEVNULL,
    )

    print("[capture] Starting tshark stream...")

    tshark_proc = await asyncio.create_subprocess_exec(
        tshark, "-i", fifo_path,
        "-l",
        "-Y", "btatt.handle == 0x002a",
        "-T", "fields",
        "-e", "frame.number",
        "-e", "frame.time_relative",
        "-e", "_ws.col.Source",
        "-e", "_ws.col.Destination",
        "-e", "_ws.col.Protocol",
        "-e", "frame.len",
        "-e", "_ws.col.Info",
        "-e", "btatt.value",
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.DEVNULL,
    )

    print("[capture] Pipeline running, streaming packets...")

    try:
        async for raw_line in tshark_proc.stdout:
            line = raw_line.decode().strip()
            if not line:
                continue
            fields = line.split("\t")
            while len(fields) < 8:
                fields.append("")
            frame, time_rel, src, dst, proto, length, info, value = (
                f.strip() for f in fields
            )

            val_display = f" | Value: {value}" if value else ""
            print(f"[sniff] [{proto}] {frame} {time_rel} {src} → {dst} {proto} {length} {info}{val_display}")

            pkt = {
                "type": "sniffed_packet",
                "frame": frame,
                "protocol": proto,
                "info": info,
                "value": value,
            }
            await broadcast(json.dumps(pkt))

    finally:
        tshark_proc.terminate()
        ubertooth_proc.terminate()
        try:
            os.remove(fifo_path)
        except OSError:
            pass
        print("[capture] Pipeline stopped.")


async def ws_handler(websocket) -> None:
    connected_clients.add(websocket)
    print(f"[ws] Client connected ({len(connected_clients)} total)")
    try:
        async for _ in websocket:
            pass
    finally:
        connected_clients.discard(websocket)
        print(f"[ws] Client disconnected ({len(connected_clients)} total)")


async def main() -> None:
    if not shutil.which("ubertooth-btle") or not shutil.which("tshark"):
        sys.exit("[sniffer] ubertooth-btle and tshark are required. Install them and try again.")

    async with websockets.serve(ws_handler, HOST, PORT):
        print(f"[sniffer] Listening on ws://{HOST}:{PORT}")
        await capture_loop()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n[sniffer] Shutting down.")
