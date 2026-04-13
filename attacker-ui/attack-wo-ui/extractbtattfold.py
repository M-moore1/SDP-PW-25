import os
import subprocess

TSHARK_PATH = r"C:\Program Files\Wireshark\tshark.exe"
PCAP_FOLDER = "captures"


def run_tshark(pcap_file, opcode_filter):
    cmd = [
        TSHARK_PATH,
        "-r",
        pcap_file,
        "-Y",
        opcode_filter,
        "-T",
        "fields",
        "-e",
        "btatt.value",
    ]

    result = subprocess.run(cmd, capture_output=True, text=True)

    if result.returncode != 0:
        print(f"Error processing {pcap_file}")
        return []

    values = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    return values


def main():
    write_values = set()
    notify_values = set()

    print(f"Scanning folder: {PCAP_FOLDER}\n")

    for file in os.listdir(PCAP_FOLDER):
        if file.endswith(".pcap") or file.endswith(".pcapng"):
            full_path = os.path.join(PCAP_FOLDER, file)
            print(f"Processing: {file}")

            writes = run_tshark(full_path, "btatt.opcode == 0x12")
            write_values.update(writes)

            notifies = run_tshark(full_path, "btatt.opcode == 0x1b")
            notify_values.update(notifies)

    print("\n=== UNIQUE WRITE REQUEST VALUES ===")
    if write_values:
        for v in write_values:
            print(v)
    else:
        print("None found")

    print("\n=== UNIQUE NOTIFICATION VALUES ===")
    if notify_values:
        for v in notify_values:
            print(v)
    else:
        print("None found")


if __name__ == "__main__":
    main()