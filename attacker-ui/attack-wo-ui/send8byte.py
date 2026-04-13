import asyncio
from bleak import BleakClient

ADDRESS = "44:1D:64:F1:1A:86"
CHAR_UUID = "0000ff01-0000-1000-8000-00805f9b34fb"

# 8-byte message
DATA = bytes([0x12, 0x34, 0x56, 0x78, 0x54, 0x23, 0x08, 0x04])

async def main():
    async with BleakClient(ADDRESS) as client:
        print("Connected:", client.is_connected)
        print("Sending:", DATA.hex())

        await asyncio.sleep(1)

        await client.write_gatt_char(CHAR_UUID, DATA, response=True)

        print("Write complete")

asyncio.run(main())