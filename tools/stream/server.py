"""Local stream overlay. Uses only the Python standard library."""
import argparse
from collections import defaultdict, deque
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import tempfile
import threading
import time
from urllib.parse import urlsplit


class Journal:
    def __init__(self, path):
        self.path = Path(path)
        self.offset = 0
        self.identity = None
        self.pending = b""
        self.latest = None
        self.entries = defaultdict(lambda: deque(maxlen=60))
        self.updated = None
        self.errors = 0
        self.lock = threading.Lock()

    def consume(self, record):
        if record.get("schema") != 1:
            raise ValueError("Unsupported journal schema")
        if not isinstance(record.get("session"), str) or not isinstance(record.get("stardate"), int):
            raise ValueError("Missing session or stardate")
        empires = record.get("empires")
        events = record.get("events")
        if not isinstance(empires, list) or len(empires) > 8 or not isinstance(events, list) or len(events) > 256:
            raise ValueError("Invalid journal size")
        ids = set()
        for empire in empires:
            if not isinstance(empire, dict) or not isinstance(empire.get("id"), int) or not 0 <= empire["id"] < 8:
                raise ValueError("Invalid empire")
            if empire["id"] in ids or not isinstance(empire.get("name"), str):
                raise ValueError("Invalid empire name or duplicate id")
            ids.add(empire["id"])
        for event in events:
            if not isinstance(event, dict) or event.get("player") not in ids:
                raise ValueError("Invalid event player")
            if any(not isinstance(event.get(key), str) for key in ("text", "kind", "evidence")):
                raise ValueError("Invalid event text")
        if self.latest is None or record["session"] != self.latest["session"] or record["stardate"] < self.latest["stardate"]:
            self.entries.clear()
        self.latest = record
        for event in events:
            self.entries[event["player"]].append({**event, "stardate": record["stardate"]})
        self.updated = time.time()

    def refresh(self):
        try:
            stat = self.path.stat()
            identity = (stat.st_dev, stat.st_ino)
            if identity != self.identity or stat.st_size < self.offset:
                self.offset = 0
                self.pending = b""
                self.latest = None
                self.entries.clear()
                self.identity = identity
            with self.path.open("rb") as file:
                file.seek(self.offset)
                # Bound work per poll; archival logs may be arbitrarily long.
                data = file.read(2 * 1024 * 1024)
            self.offset += len(data)
        except OSError:
            return
        lines = (self.pending + data).split(b"\n")
        self.pending = lines.pop()
        if len(self.pending) > 1024 * 1024:
            self.pending = b""
            self.errors += 1
        for line in lines:
            if not line.strip():
                continue
            try:
                record = json.loads(line)
                if not isinstance(record, dict):
                    raise ValueError("Expected object")
                self.consume(record)
            except (ValueError, TypeError):
                self.errors += 1

    def state(self):
        with self.lock:
            self.refresh()
            return {"latest": self.latest, "journals": {key: list(value) for key, value in self.entries.items()},
                    "updated": self.updated, "errors": self.errors}


def write_control(path, interval):
    if type(interval) is not int or interval not in (0, 1000, 3000, 5000, 10000, 30000, 60000):
        raise ValueError("Unsupported turn interval")
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(mode="w", dir=path.parent, delete=False) as file:
        file.write(str(interval) + "\n")
        temporary = Path(file.name)
    try:
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)


def make_handler(journal, control, demo=False):
    assets = Path(__file__).parent

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass

        def respond(self, code, body, content_type="application/json"):
            self.send_response(code)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.send_header("X-Content-Type-Options", "nosniff")
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self):
            path = urlsplit(self.path).path
            if path == "/api/state":
                state = journal.state()
                state["demo"] = demo
                try:
                    state["interval"] = int(control.read_text().strip())
                except (OSError, ValueError):
                    state["interval"] = 0
                self.respond(200, json.dumps(state, default=list).encode())
            elif path in ("/", "/overlay.js", "/overlay.css"):
                name, mime = {"/": ("index.html", "text/html; charset=utf-8"),
                              "/overlay.js": ("overlay.js", "text/javascript; charset=utf-8"),
                              "/overlay.css": ("overlay.css", "text/css; charset=utf-8")}[path]
                self.respond(200, (assets / name).read_bytes(), mime)
            else:
                self.respond(404, b'{}')

        def do_POST(self):
            # Only same-origin dashboard requests may drive the local game.
            expected_origin = f"http://{self.headers.get('Host', '')}"
            allowed_hosts = {f"127.0.0.1:{self.server.server_port}", f"localhost:{self.server.server_port}"}
            if (self.headers.get("Host") not in allowed_hosts or
                    self.headers.get("Origin") != expected_origin or
                    self.headers.get("Content-Type") != "application/json"):
                self.respond(403, b'{}')
                return
            if urlsplit(self.path).path != "/api/control" or demo:
                self.respond(404, b'{}')
                return
            try:
                size = int(self.headers.get("Content-Length", "0"))
                if not 0 < size <= 256:
                    raise ValueError("Invalid body length")
                body = json.loads(self.rfile.read(size))
                write_control(control, body["interval"])
            except (ValueError, TypeError, KeyError):
                self.respond(400, b'{}')
                return
            except OSError:
                self.respond(500, b'{}')
                return
            self.respond(200, b'{"ok":true}')

    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--journal", type=Path)
    parser.add_argument("--control", type=Path, default=Path("out/stream/control.txt"))
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument("--demo", action="store_true")
    args = parser.parse_args()
    if not args.demo and args.journal is None:
        parser.error("--journal is required unless --demo is selected")
    journal = Journal(Path(__file__).with_name("demo.jsonl") if args.demo else args.journal)
    write_control(args.control, 0)
    server = ThreadingHTTPServer(("127.0.0.1", args.port), make_handler(journal, args.control, args.demo))
    print(f"Journal dashboard: http://127.0.0.1:{args.port}/", flush=True)
    print(f"OBS browser source: http://127.0.0.1:{args.port}/?overlay=1&empire=0", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        write_control(args.control, 0)
        server.server_close()


if __name__ == "__main__":
    main()
