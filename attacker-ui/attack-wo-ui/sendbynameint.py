import asyncio
from bleak import BleakClient, BleakScanner

DEVICE_NAME = "ROBOT_ESP32"
CHAR_UUID = "0000ff01-0000-1000-8000-00805f9b34fb"

DEFAULT_HEX = "1234567854230804"


def parse_hex(user_input: str) -> bytes:
    cleaned = user_input.replace(" ", "").strip()

    if cleaned == "":
        cleaned = DEFAULT_HEX

    if len(cleaned) != 16:
        raise ValueError("Must be exactly 16 hex characters (8 bytes)")

    return bytes.fromhex(cleaned)


async def main():
    print("Scanning for device...")
    device = await BleakScanner.find_device_by_filter(
        lambda d, ad: d.name and DEVICE_NAME in d.name
    )

    if not device:
        print("Device not found")
        return

    print(f"Found device: {device.address}")

    async with BleakClient(device.address) as client:
        print("Connected:", client.is_connected)

        print("\nReady to send.")
        print("Enter 16 hex chars (8 bytes)")
        print(f"Press Enter for default: {DEFAULT_HEX}")
        print("Type 'q' to quit\n")

        while True:
            user_input = input("Hex> ").strip()

            if user_input.lower() in ["q", "quit"]:
                print("Exiting...")
                break

            try:
                data = parse_hex(user_input)
            except Exception as e:
                print("Error:", e)
                continue

            print("Sending:", data.hex())
            await client.write_gatt_char(CHAR_UUID, data, response=True)
            print("Sent\n")


asyncio.run(main())