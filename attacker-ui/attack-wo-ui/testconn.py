import asyncio
from bleak import BleakClient

ADDRESS = "44:1D:64:F1:1A:86"

async def main():
    async with BleakClient(ADDRESS) as client:
        print("Connected:", client.is_connected)

asyncio.run(main())