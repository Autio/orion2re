import json
import http.client
from http.server import ThreadingHTTPServer
from pathlib import Path
import tempfile
import unittest
import threading

from server import Journal, write_control, make_handler


def record(session="one", stardate=35000):
    return {"schema": 1, "session": session, "stardate": stardate,
            "empires": [{"id": 0, "name": "Alkari"}],
            "events": [{"player": 0, "target": 1, "kind": "decision", "text": "Test", "evidence": "Test"}]}


class JournalTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / "journal.jsonl"
        self.journal = Journal(self.path)

    def test_partial_writes_are_not_visible_until_complete(self):
        line = json.dumps(record()).encode()
        self.path.write_bytes(line[:35])
        self.assertIsNone(self.journal.state()["latest"])
        with self.path.open("ab") as file:
            file.write(line[35:] + b"\n")
        self.assertEqual(len(self.journal.state()["journals"][0]), 1)
        self.assertEqual(len(self.journal.state()["journals"][0]), 1)

    def test_sessions_reset_and_history_is_bounded(self):
        for i in range(100):
            self.journal.consume(record(stardate=35000 + i))
        self.assertEqual(len(self.journal.entries[0]), 60)
        self.journal.consume(record(session="two"))
        self.assertEqual(len(self.journal.entries[0]), 1)

    def test_bad_lines_do_not_prevent_recovery(self):
        self.path.write_text('invalid\n[]\n' + json.dumps(record()) + '\n')
        state = self.journal.state()
        self.assertEqual(state["errors"], 2)
        self.assertEqual(state["latest"]["session"], "one")

    def test_truncation_restarts_timeline(self):
        self.path.write_text(json.dumps(record()) + '\n' + json.dumps(record(stardate=35001)) + '\n')
        self.journal.state()
        self.path.write_text(json.dumps(record(session="two")) + '\n')
        self.assertEqual(self.journal.state()["latest"]["session"], "two")
        self.assertEqual(len(self.journal.entries[0]), 1)

    def test_unknown_empire_and_schema_rejected(self):
        bad = record()
        bad["events"][0]["player"] = 99
        with self.assertRaises(ValueError):
            self.journal.consume(bad)
        bad = record()
        bad["schema"] = 2
        with self.assertRaises(ValueError):
            self.journal.consume(bad)

    def test_control_is_validated_before_write(self):
        write_control(self.path, 3000)
        self.assertEqual(self.path.read_text(), "3000\n")
        for value in (1, -1, "3000", True, 10000000):
            with self.assertRaises(ValueError):
                write_control(self.path, value)
        self.assertEqual(self.path.read_text(), "3000\n")
        write_control(self.path, 0)
        self.assertEqual(self.path.read_text(), "0\n")

    def test_http_control_requires_same_origin_and_valid_interval(self):
        control = Path(self.temp.name) / "control.txt"
        write_control(control, 0)
        server = ThreadingHTTPServer(("127.0.0.1", 0), make_handler(self.journal, control))
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        try:
            origin = f"http://127.0.0.1:{server.server_port}"
            for request_origin, interval, expected in [("https://example.com", 3000, 403), (origin, 2, 400), (origin, 3000, 200), (origin, 0, 200)]:
                connection = http.client.HTTPConnection("127.0.0.1", server.server_port, timeout=2)
                connection.request("POST", "/api/control", json.dumps({"interval": interval}),
                                   {"Origin": request_origin, "Content-Type": "application/json"})
                response = connection.getresponse()
                self.assertEqual(response.status, expected)
                response.read()
                connection.close()
            self.assertEqual(control.read_text(), "0\n")
        finally:
            server.shutdown()
            server.server_close()
            worker.join()


if __name__ == "__main__":
    unittest.main()
