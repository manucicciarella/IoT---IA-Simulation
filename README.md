# IoT + IA Simulation

Proyecto para la materia **IoT e Inteligencia Artificial** — Universidad de Palermo.

Simula una red de sensores agrícolas con un ESP32 usando Wokwi, y procesa los datos con una red neuronal en Node-RED.

## Architecture

```
[Wokwi ESP32 Simulation]
  DHT22 · LDR · Potentiometer · LCD · LED
        |
      MQTT (broker.hivemq.com)
        |
  [Node-RED (Docker)]
    Neural network (brain.js)
```

- The ESP32 reads temperature, humidity, soil moisture, and light every 5 seconds and publishes to `cursoiot/Agus/sensor` via MQTT.
- Node-RED subscribes to that topic, runs inference through a brain.js neural network, and publishes commands back to `cursoiot/Agus/comando`.
- The ESP32 receives the command and updates the LCD and LED accordingly.

---

## Requirements

### Software to install

| Tool | Purpose | Download |
|------|---------|----------|
| VS Code | Code editor | https://code.visualstudio.com |
| Docker Desktop | Run Node-RED container | https://www.docker.com/products/docker-desktop |

### VS Code extensions to install

Open VS Code, go to the Extensions panel (`Ctrl+Shift+X`), and install:

1. **PlatformIO IDE** — `platformio.platformio-ide`
   Compiles and builds the ESP32 firmware.

2. **Wokwi Simulator** — `wokwi.wokwi-vscode`
   Runs the ESP32 circuit simulation inside VS Code.
   > After installing, you will be prompted to activate a free Wokwi account. Follow the prompt and sign in.

---

## Running the Wokwi Simulation (ESP32)

1. Open the `wokwi/` folder in VS Code:
   `File → Open Folder → select the wokwi folder`

2. Wait for PlatformIO to finish installing dependencies (first run only, takes a few minutes).

3. Build the firmware:
   - Press `Ctrl+Alt+B`, or
   - Click the checkmark icon (✓) in the bottom status bar

4. Once the build says **SUCCESS**, open `diagram.json` from the Explorer panel.

5. Click **Start Simulation** in the Wokwi panel that appears.

6. The Serial Monitor will show MQTT messages and sensor readings.

> The simulation connects to the public HiveMQ broker at `broker.hivemq.com:1883` using the `Wokwi-GUEST` WiFi network (built into the simulator).

---

## Running Node-RED (Docker)

### First time setup

Make sure Docker Desktop is running, then open a terminal in the `nodered/` folder:

```bash
cd nodered
docker compose up --build
```

This builds the image and starts Node-RED. The first run downloads the base image (~300 MB).

### Subsequent runs

```bash
docker compose up
```

### Stopping the container

```bash
docker compose down
```

### Accessing Node-RED

Open your browser and go to:

```
http://localhost:1880
```

The flows from `flows.json` will be loaded automatically. Any changes you deploy from the Node-RED UI are saved back to the local `flows.json` file in real time.

---

## Project Structure

```
IoT + IA Simulation/
├── wokwi/
│   ├── main.cpp          # ESP32 firmware (Arduino framework)
│   ├── diagram.json      # Wokwi circuit diagram
│   ├── wokwi.toml        # Wokwi simulator config
│   └── platformio.ini    # PlatformIO build config
├── nodered/
│   ├── flows.json        # Node-RED flows (neural network logic)
│   ├── Dockerfile        # Docker image definition
│   └── docker-compose.yml
└── README.md
```
