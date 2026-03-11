import time
import json
import sqlite3

from flask import Flask, jsonify, request, render_template, Response
from flask_cors import CORS
import paho.mqtt.publish as publish

app = Flask(__name__)
CORS(app)

DB        = "/home/sawyerjones/esp32-zephyr/pi/sensor_data.db"
BROKER    = "localhost"
CMD_TOPIC = "cmd/esp32"

def get_db():
    con = sqlite3.connect(DB)
    con.row_factory = sqlite3.Row
    return con

# ── Pages ────────────────────────────────────────────────────────────────

@app.route("/")
def index():
    return render_template("index.html")

# ── REST endpoints ───────────────────────────────────────────────────────

@app.route("/api/readings")
def readings():
    limit = request.args.get("limit", 50, type=int)
    con = get_db()
    rows = con.execute(
        "SELECT id, timestamp, temp, pressure FROM readings "
        "ORDER BY id DESC LIMIT ?",
        (limit,),
    ).fetchall()
    con.close()
    return jsonify([dict(r) for r in rows])

@app.route("/api/command", methods=["POST"])
def command():
    body = request.get_json()
    cmd  = body.get("command", "").strip().upper()

    valid = {"FAN_ON", "FAN_OFF", "LED_ON", "LED_OFF"}
    if cmd not in valid:
        return jsonify({"error": f"Unknown command: {cmd}"}), 400

    publish.single(CMD_TOPIC, cmd, hostname=BROKER)

    con = get_db()
    con.execute(
        "INSERT INTO commands (timestamp, command) VALUES (datetime('now'), ?)",
        (cmd,),
    )
    con.commit()
    con.close()

    return jsonify({"sent": cmd})

# ── SSE stream ───────────────────────────────────────────────────────────

@app.route("/api/stream")
def stream():
    """Server-Sent Events endpoint.

    Polls SQLite every ~1 s for rows with id > last_seen.  Each new row
    is pushed as a JSON SSE event.  The initial last_seen is seeded from
    the query param so the client can tell us where it left off after the
    initial REST fetch.
    """
    last_id = request.args.get("last_id", 0, type=int)

    def generate():
        nonlocal last_id
        while True:
            con = get_db()
            rows = con.execute(
                "SELECT id, timestamp, temp, pressure FROM readings "
                "WHERE id > ? ORDER BY id ASC",
                (last_id,),
            ).fetchall()
            con.close()

            for row in rows:
                r = dict(row)
                last_id = r["id"]
                yield f"data: {json.dumps(r)}\n\n"

            time.sleep(1)

    return Response(generate(), mimetype="text/event-stream")

# ── Main ─────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    # threaded=True so the SSE generator doesn't block other requests
    app.run(host="0.0.0.0", port=5000, debug=False, threaded=True)