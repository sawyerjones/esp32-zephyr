import paho.mqtt.client as mqtt
import sqlite3, json, datetime

DB_PATH = "sensor_data.db"
BROKER_HOST = "localhost"
TOPIC = "sensor/#"

def init_db(conn):
    conn.execute("""
        CREATE TABLE IF NOT EXISTS readings (
            id        INTEGER PRIMARY KEY AUTOINCREMENT,
            node      TEXT    NOT NULL,
            timestamp TEXT    NOT NULL,
            temp      REAL,
            pressure  REAL
        )
    """)
    conn.commit()

def on_connect(client, userdata, flags, rc):
    print(f"[PI] connected (rc={rc})")
    client.subscribe(TOPIC)

def on_message(client, userdata, msg):
    conn = userdata["db"]
    node = msg.topic.split("/")[1]
    try:
        data = json.loads(msg.payload)
        ts   = datetime.datetime.utcnow().isoformat()
        conn.execute(
            "INSERT INTO readings (node, timestamp, temp, pressure)"
            " VALUES (?, ?, ?, ?)",
            (node, ts, data["temp"], data["pressure"])
        )
        conn.commit()
        print(f"[{ts}] {node}: {data}")
    except Exception as e:
        print(f"[error] {e}")

if __name__ == "__main__":
    conn = sqlite3.connect(DB_PATH)
    init_db(conn)

    client = mqtt.Client(userdata={"db": conn})
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(BROKER_HOST, 1883, keepalive=60)
    client.loop_forever()