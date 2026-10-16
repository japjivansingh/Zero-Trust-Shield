import asyncio
import json
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.middleware.cors import CORSMiddleware

app = FastAPI()

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

RULES_FILE = "/app/config/rules.txt"
connected_clients = set()

# --- REST API for Rules ---
@app.get("/api/rules")
def get_rules():
    try:
        with open(RULES_FILE, "r") as f:
            rules = [line.strip() for line in f.readlines() if line.strip()]
        return {"rules": rules}
    except Exception as e:
        return {"error": str(e)}

@app.post("/api/rules")
def add_rule(rule: dict):
    with open(RULES_FILE, "a") as f:
        f.write(f"\n{rule['keyword'].upper()}")
    return {"status": "success"}

@app.delete("/api/rules/{keyword}")
def delete_rule(keyword: str):
    with open(RULES_FILE, "r") as f:
        rules = [line.strip() for line in f.readlines() if line.strip()]
    
    rules = [r for r in rules if r != keyword.upper()]
    
    with open(RULES_FILE, "w") as f:
        f.write("\n".join(rules))
    return {"status": "success"}

# --- WebSockets for Live UI Updates ---
@app.websocket("/ws/logs")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    connected_clients.add(websocket)
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        connected_clients.remove(websocket)

# --- UDP Server to receive logs from C-Proxy ---
class UDPLogProtocol(asyncio.DatagramProtocol):
    def datagram_received(self, data, addr):
        log_entry = data.decode('utf-8')
        # Broadcast to all connected web UI clients
        for client in list(connected_clients):
            asyncio.create_task(client.send_text(log_entry))

@app.on_event("startup")
async def startup_udp_server():
    loop = asyncio.get_running_loop()
    await loop.create_datagram_endpoint(
        lambda: UDPLogProtocol(),
        local_addr=("0.0.0.0", 5005)
    )
