import asyncio
import websockets
import json

async def connect():
    url = "wss://stream.binance.com:9443/ws/btcusdt@depth"
    async with websockets.connect(url) as ws:
        while True:
            message = await ws.recv()
            data = json.loads(message)
            for price, qty in data['a']:
                # qty == 0  → means "remove this price level"
                price_int = int(float(price) * 100)
                qty_int = int(float(qty) * 100)
                if qty_int != 0:
                    print("SELL", price_int, qty_int)

            for price, qty in data['b']:
                price_int = int(float(price) * 100)
                qty_int = int(float(qty) * 100)
                if qty_int != 0:
                    print("BUY", price_int, qty_int)

asyncio.run(connect())
