from flask import Flask, jsonify, request
from flask_cors import CORS
import sqlite3
import paho.mqtt.publish as publish

from flask import render_template


app = Flask(__name__)
CORS(app)

DB = "/home/sawyerjones/esp32-zephyr/pi/sensor_data.db"
BROKER = "localhost"
CMD_TOPIC = "cmd/esp32"

def get_db():
    con = sqlite3.connect(DB)
    con.row_factory = sqlite3.Row
    return con

@app.route("/")
def index():
    return render_template("index.html")

@app.route("/api/readings")
def readings():
    limit = request.args.get("limit", 50, type=int)
    con = get_db()
    rows = con.execute(
        "SELECT timestamp, temp, pressure FROM readings ORDER BY timestamp DESC LIMIT ?",
        (limit,)
    ).fetchall()
    con.close()
    return jsonify([dict(r) for r in rows])

@app.route("/api/command", methods=["POST"])
def command():
    body = request.get_json()
    cmd = body.get("command", "").strip().upper()

    valid = {"FAN_ON", "FAN_OFF", "LED_ON", "LED_OFF"}
    if cmd not in valid:
        return jsonify({"error": f"Unknown command: {cmd}"}), 400

    publish.single(CMD_TOPIC, cmd, hostname=BROKER)

    con = get_db()
    con.execute("INSERT INTO commands (command) VALUES (?)", (cmd,))
    con.commit()
    con.close()

    return jsonify({"sent": cmd})

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False)