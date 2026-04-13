import asyncio
from bleak import BleakClient, BleakScanner

DEVICE_NAME = "ROBOT_ESP32"
CHAR_UUID = "0000ff01-0000-1000-8000-00805f9b34fb"

# 8-byte message
DATA = bytes([0x12, 0x34, 0x56, 0x78, 0x54, 0x23, 0x08, 0x04])


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

        await client.write_gatt_char(CHAR_UUID, DATA, response=True)

        print("Sent:", DATA.hex())


asyncio.run(main())