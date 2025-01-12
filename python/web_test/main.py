from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
import asyncio
from serial_handler import SerialHandler
import os
import logging

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

app = FastAPI()
app.mount("/static", StaticFiles(directory="static"), name="static")
serial_handler = SerialHandler()

def round_to_half(value):
    """Round to nearest 0.5"""
    return round(value * 2) / 2

@app.get("/")
async def root():
    return FileResponse('static/index.html')

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    if not serial_handler.connect():
        await websocket.close(1001, "Unable to connect to device")
        return
    
    async def read_serial():
        while True:
            data = serial_handler.read_data()
            if data:
                try:
                    if data["type"] == "position":
                        data["value"] = round_to_half(data["value"])
                    await websocket.send_json(data)
                except:
                    break
            await asyncio.sleep(0.001)  # 1ms interval for serial reading
    
    async def handle_commands():
        while True:
            try:
                cmd = await websocket.receive_text()
                if cmd:
                    serial_handler.send_command(cmd)
            except:
                break
    
    try:
        # Run both tasks concurrently
        await asyncio.gather(
            read_serial(),
            handle_commands()
        )
    except WebSocketDisconnect:
        logger.info("Client disconnected")
    except Exception as e:
        logger.error(f"Error in websocket: {e}")
    finally:
        if websocket.client_state.CONNECTED:
            await websocket.close()