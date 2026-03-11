import paho.mqtt.client as mqtt
import sqlite3, json, datetime

DB_PATH     = "/home/sawyerjones/esp32-zephyr/pi/sensor_data.db"
BROKER_HOST = "localhost"
TOPIC       = "sensor/#"

def init_db():
    """Run once at startup to ensure tables exist."""
    conn = sqlite3.connect(DB_PATH)
    conn.execute("""
        CREATE TABLE IF NOT EXISTS readings (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            node      TEXT    NOT NULL,
            timestamp TEXT    NOT NULL,
            temp      REAL,
            humidity  REAL,
            pressure  REAL
        )
    """)
    conn.execute("""
        CREATE TABLE IF NOT EXISTS commands (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT    NOT NULL,
            command   TEXT    NOT NULL
        )
    """)
    conn.commit()
    conn.close()

def on_connect(client, userdata, flags, rc):
    print(f"[PI] connected (rc={rc})")
    client.subscribe(TOPIC)

def on_message(client, userdata, msg):
    """Open a fresh connection per message to avoid SQLite threading issues.

    paho-mqtt's loop_forever() dispatches on_message from its network
    thread.  A long-lived connection opened on the main thread can silently
    fail or deadlock under SQLite's default threading mode.  Opening and
    closing per-message is cheap for the ~0.2 msg/s rate we have here.
    """
    node = msg.topic.split("/")[1]
    try:
        data = json.loads(msg.payload)
        ts   = datetime.datetime.utcnow().isoformat()

        conn = sqlite3.connect(DB_PATH)
        conn.execute(
            "INSERT INTO readings (node, timestamp, temp, humidity, pressure)"
            " VALUES (?, ?, ?, ?, ?)",
            (node, ts, data.get("temp"), data.get("humidity"), data.get("pressure")),
        )
        conn.commit()
        conn.close()

        print(f"[{ts}] {node}: temp={data.get('temp')}  pressure={data.get('pressure')}")
    except Exception as e:
        print(f"[error] {e}")

if __name__ == "__main__":
    init_db()

    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(BROKER_HOST, 1883, keepalive=60)
    print(f"[PI] subscribing to {TOPIC} — broker {BROKER_HOST}:1883")
    client.loop_forever()