import asyncio
from bleak import BleakClient

ADDRESS = "44:1D:64:F1:1A:86"

async def main():
    async with BleakClient(ADDRESS) as client:
        print("Connected:", client.is_connected)

        services = client.services
        for service in services:
            print(f"\nService: {service.uuid}")
            for char in service.characteristics:
                print(f"  Characteristic: {char.uuid}")
                print(f"    Properties: {char.properties}")

asyncio.run(main())