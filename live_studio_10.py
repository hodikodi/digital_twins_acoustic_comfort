import matplotlib.pyplot as plt
import matplotlib.image as mpimg
import paho.mqtt.client as mqtt
import matplotlib.patches as patches
import csv
from datetime import datetime
import os

# --- SETTINGS ---
sensor_data = {"m1": 0.0, "m2": 0.0}
IMAGE_PATH = '/home/lucy/image_01 (8).png'
LOG_FILE = 'studio_noise_log.csv'

# Initialize CSV file with headers if it doesn't exist
if not os.path.exists(LOG_FILE):
    with open(LOG_FILE, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['Timestamp', 'Meter', 'Reading_dBA'])

# --- MQTT CALLBACK ---
def on_message(client, userdata, msg):
    global sensor_data
    try:
        val = float(msg.payload.decode())
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        
        meter_label = ""
        if "meter1" in msg.topic: 
            sensor_data["m1"] = val
            meter_label = "M1"
        elif "meter2" in msg.topic: 
            sensor_data["m2"] = val
            meter_label = "M2"

        # LOG TO CSV: This happens every time a new reading arrives (approx every second)
        if meter_label:
            with open(LOG_FILE, 'a', newline='') as f:
                writer = csv.writer(f)
                writer.writerow([timestamp, meter_label, val])
                
    except Exception as e:
        print(f"Logging Error: {e}")

# --- MQTT SETUP ---
client = mqtt.Client()
client.on_message = on_message
client.connect("127.0.0.1", 1883)
client.subscribe("room/noise/#")
client.loop_start()

# --- PLOT SETUP ---
plt.ion()
fig, ax = plt.subplots(figsize=(15, 10), facecolor='black')
img = mpimg.imread(IMAGE_PATH)
h, w, _ = img.shape

print(f"System Live. Logging data to {LOG_FILE}...")

while True:
    ax.clear()
    ax.imshow(img)
    
    # Coordinates (Adjusted for Zone M1 and M2)
    coords = {"m1": (w * 0.30, h * 0.6), "m2": (w * 0.70, h * 0.6)}

    for m_id, (x, y) in coords.items():
        db = sensor_data[m_id]
        color = 'green' if db < 55 else ('orange' if db < 75 else 'red')
        if db == 0.0: color = 'gray' # Sensor hasn't reported yet

        # 1. Zone Overlay
        zone_x = 0 if m_id == "m1" else w/2
        ax.add_patch(patches.Rectangle((zone_x, 0), w/2, h, facecolor=color, alpha=0.15))
        
        # 2. Dynamic Pulse
        pulse = 2000 + (db * 80) if db > 0 else 500
        ax.scatter(x, y, s=pulse, c=color, alpha=0.6, edgecolors='white', linewidth=2)
        
        # 3. Text Label
        label = f"METER {m_id.upper()}\n{db if db > 0 else 'WAITING...'} dBA"
        ax.text(x, y-120, label, color='white', ha='center', weight='bold', 
                bbox=dict(facecolor='black', alpha=0.8, boxstyle='round,pad=0.5'))



    ax.axis('off')
    plt.pause(0.3) # Stops terminal lag