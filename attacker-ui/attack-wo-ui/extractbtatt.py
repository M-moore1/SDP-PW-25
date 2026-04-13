import argparse
import subprocess
import sys
from typing import List, Tuple

TSHARK_PATH = r"C:\Program Files\Wireshark\tshark.exe"


def run_tshark(pcap_file: str, opcode_filter: str) -> List[Tuple[str, str, str]]:
    cmd = [
        TSHARK_PATH,
        "-r",
        pcap_file,
        "-Y",
        opcode_filter,
        "-T",
        "fields",
        "-e",
        "frame.number",
        "-e",
        "btatt.opcode",
        "-e",
        "btatt.value",
    ]

    result = subprocess.run(cmd, capture_output=True, text=True)

    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "tshark failed")

    rows: List[Tuple[str, str, str]] = []
    for line in result.stdout.splitlines():
        parts = line.split("\t")
        while len(parts) < 3:
            parts.append("")
        frame_num, opcode, value = parts[0], parts[1], parts[2]
        if value.strip():
            rows.append((frame_num.strip(), opcode.strip(), value.strip()))
    return rows


def unique_by_value(rows: List[Tuple[str, str, str]]) -> List[Tuple[str, str, str]]:
    seen = set()
    unique_rows = []
    for row in rows:
        value = row[2]
        if value not in seen:
            seen.add(value)
            unique_rows.append(row)
    return unique_rows


def opcode_name(opcode: str) -> str:
    mapping = {
        "0x12": "Write Request",
        "0x1b": "Handle Value Notification",
    }
    return mapping.get(opcode.lower(), opcode)


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Extract unique BT ATT values from a pcap using tshark."
    )
    parser.add_argument("pcap", help="Path to the .pcap/.pcapng file")
    parser.add_argument(
        "--mode",
        choices=["write", "notify", "both"],
        default="both",
        help="Which ATT packet types to extract",
    )
    args = parser.parse_args()

    filters = []
    if args.mode == "write":
        filters.append(("btatt.opcode == 0x12", "Write Request"))
    elif args.mode == "notify":
        filters.append(("btatt.opcode == 0x1b", "Handle Value Notification"))
    else:
        filters.append(("btatt.opcode == 0x12", "Write Request"))
        filters.append(("btatt.opcode == 0x1b", "Handle Value Notification"))

    for filt, title in filters:
        try:
            rows = run_tshark(args.pcap, filt)
            rows = unique_by_value(rows)
        except Exception as e:
            print(f"\n[{title}] Error: {e}")
            continue

        print(f"\n=== {title} ===")
        if not rows:
            print("No values found.")
            continue

        for frame_num, opcode, value in rows:
            print(f"Frame {frame_num} | {opcode_name(opcode)} | {value}")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        sys.exit(1)