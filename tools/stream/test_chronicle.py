import copy
import unittest
from pathlib import Path
import tempfile

from server import Journal


def snapshot(stardate, colonies=2, population=23, ships=7, events=None, session="one"):
    return {"schema": 1, "session": session, "stardate": stardate,
            "empires": [dict(id=0, name="Alkari", personality=4, colonies=colonies,
                             population=population, ships=ships, eliminated=False),
                        dict(id=1, name="Meklar", personality=3, colonies=3,
                             population=30, ships=8, eliminated=False)],
            "events": events or []}


def war_event(before=0, after=4):
    return dict(player=0, target=1, kind="outcome", text="Diplomatic state changed.",
                evidence=f"Treaty {before} -> {after}; trade 0 -> 0; research 0 -> 0")


class ChronicleTests(unittest.TestCase):
    def test_final_population_loss_is_not_described_as_expansion(self):
        self.journal.consume(snapshot(35100))
        self.journal.consume(snapshot(35180, colonies=0, population=1, ships=0))
        ending = snapshot(35271, colonies=0, population=0, ships=0)
        ending["empires"][0]["eliminated"] = True
        self.journal.consume(ending)
        entry = self.journal.chronicle.entries[0][-1]
        self.assertNotIn("larger domain", entry["narrative"])
        self.assertIn("eliminated", entry["narrative"])
        self.assertIn("once held 2 colonies", self.journal.chronicle.summaries[0]["text"])

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.journal = Journal(Path(self.temp.name) / "not-created.jsonl")

    def test_war_and_collapse_have_history_without_invented_attacker(self):
        self.journal.consume(snapshot(35102))
        self.journal.consume(snapshot(35167, events=[war_event()]))
        self.journal.consume(snapshot(35180, colonies=0, population=1, ships=6))
        chronicle = self.journal.chronicle
        entry = chronicle.entries[0][-1]
        self.assertIn("population changed from 23 to 1", entry["narrative"])
        self.assertIn("do not name an attacker", entry["narrative"])
        self.assertNotIn("Antaran", entry["narrative"])
        self.assertNotIn("homeworld", entry["narrative"])
        self.assertIn("13 turns ago", entry["history"])
        self.assertEqual(chronicle.summaries[0]["title"], "An empire diminished")
        self.assertIn("held 2 colonies", chronicle.summaries[0]["text"])
        self.assertIn("3516.7 -> 3518.0", entry["evidence"])

    def test_engine_log_is_unchanged_and_has_no_inferred_events(self):
        source = snapshot(35167, events=[war_event()])
        original = copy.deepcopy(source)
        self.journal.consume(source)
        self.journal.consume(snapshot(35180, colonies=0, population=1))
        self.assertEqual(source, original)
        self.assertEqual(len(self.journal.entries[0]), 1)
        self.assertNotIn("narrative", self.journal.entries[0][0])
        self.assertGreater(len(self.journal.chronicle.entries[0]), 1)

    def test_earlier_entries_do_not_learn_future_outcomes(self):
        self.journal.consume(snapshot(35100))
        self.journal.consume(snapshot(35167, events=[war_event()]))
        earlier = copy.deepcopy(list(self.journal.chronicle.entries[0]))
        self.journal.consume(snapshot(35180, colonies=0, population=1))
        self.assertEqual(list(self.journal.chronicle.entries[0])[:len(earlier)], earlier)

    def test_peace_closes_war_memory(self):
        self.journal.consume(snapshot(35100, events=[war_event()]))
        self.journal.consume(snapshot(35130))
        self.assertIn("No net territorial gain", self.journal.chronicle.summaries[0]["text"])
        self.journal.consume(snapshot(35140, events=[war_event(4, 3)]))
        self.assertEqual(self.journal.chronicle.memory[0]["wars"], {})
        self.assertIn("40 recorded turns", self.journal.chronicle.entries[0][-1]["narrative"])
        self.assertNotIn("The war with", self.journal.chronicle.summaries[0]["text"])

    def test_session_change_and_rollback_clear_history(self):
        for new in [snapshot(35000), snapshot(35200, session="two")]:
            self.journal.consume(snapshot(35167, events=[war_event()]))
            self.journal.consume(new)
            self.assertEqual(self.journal.chronicle.memory[0]["wars"], {})
            self.assertEqual(len(self.journal.chronicle.entries[0]), 1)

    def test_replay_is_deterministic_and_memory_bounded(self):
        rows = [snapshot(35000 + i, colonies=2 + i % 2) for i in range(100)]
        second = Journal(Path(self.temp.name) / "other.jsonl")
        for record in rows:
            self.journal.consume(record)
            second.consume(record)
        self.assertEqual(list(self.journal.chronicle.entries[0]), list(second.chronicle.entries[0]))
        self.assertEqual(len(self.journal.chronicle.entries[0]), 60)
        self.assertEqual(len(self.journal.chronicle.memory[0]["milestones"]), 12)

    def test_invalid_record_does_not_change_story(self):
        self.journal.consume(snapshot(35100))
        original = copy.deepcopy(self.journal.chronicle.summaries)
        bad = snapshot(35101)
        bad["empires"][0]["population"] = "many"
        with self.assertRaises(ValueError):
            self.journal.consume(bad)
        self.assertEqual(self.journal.chronicle.summaries, original)

    def test_calculated_attack_is_not_presented_as_a_forecast(self):
        event = dict(player=0, target=1, kind="decision", text="War selected.",
                     evidence="NPC_Declarations_Of_War: reason=23; relations=0")
        self.journal.consume(snapshot(35167, events=[event]))
        self.assertIn("not a forecast of victory", self.journal.chronicle.entries[0][-1]["narrative"])


if __name__ == "__main__":
    unittest.main()
