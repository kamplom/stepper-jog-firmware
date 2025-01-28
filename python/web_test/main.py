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
app.mount("/static", StaticFiles(directory="static"), name="statifc")
serial_handler = SerialHandler()
cached_position = None  # Store last known position
offset_value = 0.0  # Store offset value
connected_clients = []
is_metric = True  # Add global metric preference

def round_to_half(value):
    """Round to nearest 0.5"""
    return round(value * 2) / 2

@app.get("/")
async def root():
    return FileResponse('static/index.html')

@app.get("/distance")
async def get_position():
    return FileResponse('static/distance.html')

@app.get("/distanceN")
async def distanceN():
    return FileResponse('static/distanceN.html')

@app.get("/raw")
async def return_raw():
    return FileResponse('static/raw.html')

@app.get("/serial")
async def serial_monitor():
    return FileResponse('static/serial.html')

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    global cached_position, offset_value, is_metric
    await websocket.accept()
    connected_clients.append(websocket)
    
    try:
        if not serial_handler.connect():
            await websocket.close(code=1001)
            return
            
        # Send initial states in this specific order
        if cached_position is not None:
            await websocket.send_json({
                "type": "position",
                "value": cached_position
            })
        await websocket.send_json({
            "type": "unit",
            "isMetric": is_metric
        })
        await websocket.send_json({
            "type": "offset",
            "value": offset_value
        })
        
        while True:
            try:
                cmd = await asyncio.wait_for(websocket.receive_text(), timeout=0.001)
                if cmd:
                    if cmd.startswith("offset:"):
                        offset_value = float(cmd.split(":")[1])
                        if offset_value == 0.0:
                            offset_value = 0.0
                        await broadcast({"type": "offset", "value": offset_value})
                    elif cmd.startswith("unit:"):
                        is_metric = cmd.split(":")[1].lower() == "true"
                        logger.info(f"Unit change: isMetric = {is_metric}")
                        await broadcast({"type": "unit", "isMetric": is_metric})
                    else:
                        serial_handler.send_command(cmd)
            except asyncio.TimeoutError:
                pass
            except WebSocketDisconnect:
                break
                
            data = serial_handler.read_data()
            if data:
                if data["type"] == "position":
                    data["value"] = round_to_half(data["value"]) 
                    cached_position = data["value"]
                await broadcast(data)
                
            await asyncio.sleep(0.001)
            
    except WebSocketDisconnect:
        pass
    finally:
        connected_clients.remove(websocket)

@app.websocket("/ws/serial")  # New endpoint for raw serial communication
async def websocket_serial_endpoint(websocket: WebSocket):
    await websocket.accept()
    
    try:
        if not serial_handler.connect():
            await websocket.send_text("Failed to connect to serial port")
            await websocket.close(code=1001)
            return
            
        await websocket.send_text("Connected to serial port\n")
        
        while True:
            try:
                # Check for incoming commands
                cmd = await asyncio.wait_for(websocket.receive_text(), timeout=0.001)
                if cmd:
                    serial_handler.send_command(cmd)
            except asyncio.TimeoutError:
                pass
            except WebSocketDisconnect:
                break
                
            # Read raw serial data
            raw_data = serial_handler.read_raw()
            if raw_data:
                await websocket.send_text(raw_data.decode('utf-8', errors='replace'))
                
            await asyncio.sleep(0.001)
            
    except WebSocketDisconnect:
        pass
    finally:
        pass
async def broadcast(data):
    for client in connected_clients:
        try:
            await client.send_json(data)
        except WebSocketDisconnect:
            connected_clients.remove(client)