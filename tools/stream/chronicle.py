"""Deterministic dramatic prose grounded in the journal, never inferred battle causes."""
from collections import defaultdict, deque
import re


VOICES = {
    0: "Beyond our borders, every promise comes from a stranger.",
    1: "An empire is measured by what it can keep.",
    2: "Our banners were not made to hang forever in safe harbors.",
    3: "The stars offer no promise that tomorrow will resemble today.",
    4: "Our word must outlast the fortunes of a single campaign.",
    5: "Every quiet border is a chance to build something that will last.",
}


def date(value):
    return f"{value / 10:.1f}"


def has_statistics(empire):
    return all(type(empire.get(key)) is int and empire[key] >= 0
               for key in ("colonies", "population", "ships"))


class Chronicle:
    def __init__(self):
        self.entries = defaultdict(lambda: deque(maxlen=60))
        self.memory = {}
        self.summaries = {}

    def _remember(self, player, stardate, text):
        self.memory[player]["milestones"].append({"stardate": stardate, "text": text})

    def _context(self, player, stardate):
        milestones = self.memory[player]["milestones"]
        prior = [item for item in milestones if item["stardate"] < stardate]
        if not prior:
            return ""
        last = prior[-1]
        return f"The earlier entry at {date(last['stardate'])} still frames this moment: {last['text']}"

    def _add(self, event, stardate, narrative, context=""):
        self.entries[event["player"]].append({**event, "stardate": stardate,
            "narrative": narrative, "history": context, "dramatized": True})

    def _war_context(self, player, stardate):
        wars = self.memory[player]["wars"]
        if not wars:
            return ""
        return " ".join(f"Our recorded war with {war['name']} began at {date(war['start'])}, "
                        f"{stardate - war['start']} turns ago." for war in wars.values())

    def consume(self, record):
        stardate = record["stardate"]
        roster = {empire["id"]: empire for empire in record["empires"]}
        for player, empire in roster.items():
            if player not in self.memory:
                self.memory[player] = {"start": stardate, "initial": dict(empire),
                    "previous": dict(empire), "previous_date": stardate, "peak_colonies": empire.get("colonies", 0),
                    "peak_population": empire.get("population", 0),
                    "milestones": deque(maxlen=12), "wars": {}}
                if has_statistics(empire):
                    self._add({"player": player, "target": -1, "kind": "chronicle",
                        "text": "Opening empire report.", "source": "snapshot",
                        "evidence": f"Initial snapshot: colonies={empire['colonies']}; population={empire['population']}; ships={empire['ships']}."},
                        stardate, f"We are the {empire['name']}. Our story opens with {empire['colonies']} "
                        f"reported colonies, a population of {empire['population']}, and {empire['ships']} ships. "
                        + VOICES.get(empire.get("personality"), "The next chapter has not yet been written."))

        for event in record["events"]:
            player = event["player"]
            target = event.get("target", -1)
            memory = self.memory[player]
            name = roster.get(target, {}).get("name", "the other empire")
            context = self._context(player, stardate)
            evidence = event["evidence"]
            transition = re.search(r"Treaty (\d+) -> (\d+)", evidence)
            # Only recognized exporter evidence gives an event a more specific meaning.
            if event["kind"] == "decision" and evidence.startswith("NPC_Declarations_Of_War:"):
                reason = re.search(r"reason=(\d+)", evidence)
                explanations = {
                    "20": "A personality-driven aggression check has carried the decision.",
                    "22": "The decision record points to colony losses as the reason for retaliation.",
                    "23": "The fleet comparison cleared a threshold shaped by temperament, treaties, difficulty, and chance. It is a wager, not a forecast of victory.",
                    "68": "Poor relations and a favorable fleet comparison opened the door to this decision.",
                    "113": "Expansion or food pressure lies behind the decision; this record does not distinguish the two.",
                }
                explanation = explanations.get(reason.group(1) if reason else "", "The strategic checks selected this target; their full reasoning was not recorded.")
                narrative = f"Our banners turn toward {name}. {explanation} The order selects an enemy; it does not yet tell the story of a successful campaign."
            elif event["kind"] == "outcome" and transition:
                before, after = map(int, transition.groups())
                if before < 4 <= after:
                    narrative = f"The peace with {name} is over. At {date(stardate)}, war becomes part of our history. The opening calculations are behind us; the next entries will have to account for what this conflict actually costs."
                    memory["wars"][target] = {"name": name, "start": stardate,
                        "colonies": roster[player].get("colonies")}
                    self._remember(player, stardate, f"war began with {name}.")
                elif before >= 4 > after:
                    war = memory["wars"].pop(target, None)
                    duration = f" after {stardate - war['start']} recorded turns" if war else ""
                    narrative = f"Our war with {name} has ended{duration}. A boundary in the ledger has changed; whether it becomes a lasting peace belongs to the chapters ahead."
                    self._remember(player, stardate, f"war ended with {name}.")
                else:
                    narrative = event["text"] + " A new arrangement joins the history between our empires. Its terms are recorded below; its durability will be measured in later turns."
                    self._remember(player, stardate, event["text"])
            elif event["kind"] == "evaluation":
                narrative = event["text"] + " This is the moment of deliberation, before any agreement is secured. A favorable calculation is still only an opening; the outcome must earn its own entry."
            else:
                narrative = event["text"]
            self._add({**event, "source": "engine"}, stardate, narrative, context)

        for player, empire in roster.items():
            memory = self.memory[player]
            previous = memory["previous"]
            if has_statistics(empire) and has_statistics(previous):
                changes = {key: empire[key] - previous[key] for key in ("colonies", "population", "ships")}
                lost_population = changes["population"] <= -max(3, previous["population"] // 4)
                eliminated = empire.get("eliminated", False) and not previous.get("eliminated", False)
                if changes["colonies"] or lost_population or changes["ships"] <= -2 or eliminated:
                    context = self._war_context(player, stardate) or self._context(player, stardate)
                    if eliminated:
                        opening = "The empire's independent story has ended: the game now marks us eliminated."
                    elif changes["colonies"] < 0 or lost_population:
                        opening = "The ledger has become a record of loss."
                    elif changes["colonies"] > 0:
                        opening = "Our place among the stars has grown."
                    else:
                        opening = "The fleet register has grown shorter."
                    facts = []
                    for key, label in (("colonies", "reported colonies"), ("population", "population"), ("ships", "ships")):
                        if changes[key]:
                            facts.append(f"{label} changed from {previous[key]} to {empire[key]}")
                    factual = "; ".join(facts) + "."
                    narrative = opening + " " + factual[0].upper() + factual[1:]
                    if eliminated or any(value < 0 for value in changes.values()):
                        narrative += " These totals do not name an attacker, a planet, or a cause. The losses belong in our history; an explanation must wait for a fuller record."
                    else:
                        narrative += " The totals mark a larger domain, but do not say whether its new holdings were settled, captured, or transferred."
                    self._add({"player": player, "target": -1, "kind": "chronicle", "source": "snapshot",
                        "text": factual[0].upper() + factual[1:],
                        "evidence": f"Snapshot comparison: {date(memory['previous_date'])} -> {date(stardate)}. " + factual + " Attribution unavailable."},
                        stardate, narrative, context)
                    self._remember(player, stardate, factual)
                memory["peak_colonies"] = max(memory["peak_colonies"], empire["colonies"])
                memory["peak_population"] = max(memory["peak_population"], empire["population"])
                self.summaries[player] = self._summary(player, empire, stardate)
            memory["previous"] = dict(empire)
            memory["previous_date"] = stardate

    def _summary(self, player, empire, stardate):
        memory = self.memory[player]
        title = "The story so far"
        paragraphs = [f"Our chronicle began at {date(memory['start'])}. The latest register lists "
            f"{empire['colonies']} colonies, population {empire['population']}, and {empire['ships']} ships."]
        if empire.get("eliminated"):
            title = "The last chapter"
            paragraphs.append(f"We once held {memory['peak_colonies']} colonies and our population reached "
                f"{memory['peak_population']}. Those earlier heights remain part of our story, even as the "
                "game records our elimination.")
        elif empire["colonies"] < memory["peak_colonies"]:
            title = "An empire diminished"
            paragraphs.append(f"At our recorded high-water mark we held {memory['peak_colonies']} colonies. "
                f"Now the register lists {empire['colonies']}. Our population once reached {memory['peak_population']}; "
                f"it now stands at {empire['population']}. Those figures measure the reversal without identifying its cause.")
        elif empire["colonies"] > memory["initial"].get("colonies", empire["colonies"]):
            title = "A wider horizon"
        wars = memory["wars"]
        for war in wars.values():
            elapsed = stardate - war["start"]
            paragraph = f"The war with {war['name']} entered our record at {date(war['start'])}, {elapsed} turns ago."
            if type(war["colonies"]) is int and elapsed > 0:
                paragraph += f" Our colony count was {war['colonies']} then and is {empire['colonies']} now."
                if elapsed >= 20 and empire["colonies"] <= war["colonies"]:
                    paragraph += " No net territorial gain appears in that comparison; it does not measure every success or loss on the front."
            paragraphs.append(paragraph)
        return {"title": title, "text": " ".join(paragraphs), "dramatized": True,
            "milestones": list(memory["milestones"])[-4:]}
