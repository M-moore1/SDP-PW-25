import asyncio
import json
import os
import shutil
import subprocess
import sys

import websockets

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "ubertooth"))
from script import opcode_name

HOST = "localhost"
PORT = 8765

connected_clients: set = set()
seen_frames: set = set()


def find_tool(name: str) -> str:
    path = shutil.which(name)
    if not path:
        sys.exit(f"Error: '{name}' not found. Make sure it is installed and on your PATH.")
    return path


def parse_line(line: str) -> dict | None:
    parts = line.strip().split("\t")
    while len(parts) < 3:
        parts.append("")

    frame_num, opcode, value = parts[0].strip(), parts[1].strip(), parts[2].strip()
    if not value:
        return None

    return {
        "type": "sniffed_packet",
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

    print("[capture] Starting ubertooth-btle...")

    ubertooth_proc = subprocess.Popen(
        ["sudo", ubertooth, "-f", "-c", pcap_file],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )

    await asyncio.sleep(2)
    print("[capture] Pipeline running, polling for packets...")

    connect_ind_seen = False
    loop = asyncio.get_event_loop()
    try:
        while True:
            if not os.path.exists(pcap_file):
                await asyncio.sleep(1)
                continue
            # if not connect_ind_seen:
            #     conn_result = await loop.run_in_executor(None, lambda: subprocess.run(
            #         [
            #             tshark, "-r", pcap_file,
            #             "-Y", '_ws.col.Info contains "CONNECT_IND"',
            #             "-T", "fields",
            #             "-e", "frame.number",
            #             "-c", "1",
            #         ],
            #         capture_output=True,
            #         text=True,
            #     ))
            #     if conn_result.stdout.strip():
            #         print("Connected")
            #         connect_ind_seen = True

            result = await loop.run_in_executor(None, lambda: subprocess.run(
                [
                    tshark, "-r", pcap_file,
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
                ],
                capture_output=True,
                text=True,
            ))

            
            for line in result.stdout.splitlines():
                line = line.strip()
                if not line:
                    continue
                fields = line.split("\t")
                while len(fields) < 8:
                    fields.append("")
                frame, time, src, dst, proto, length, info, value = (
                    f.strip() for f in fields
                )
                if frame in seen_frames:
                    continue
                seen_frames.add(frame)

                val_display = f" | Value: {value}" if value else ""
                print(f"[sniff] [{proto}] {frame} {time} {src} → {dst} {proto} {length} {info}{val_display}")

                pkt = {
                    "type": "sniffed_packet",
                    "frame": frame,
                    "protocol": proto,
                    "info": info,
                    "value": value,
                }
                await broadcast(json.dumps(pkt))

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
