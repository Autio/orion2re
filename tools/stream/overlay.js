"use strict";
const params = new URLSearchParams(location.search);
const overlay = params.get("overlay") === "1";
document.body.classList.toggle("overlay", overlay);
const $ = id => document.getElementById(id);
const colors = ["#8bd3dd", "#edae83", "#b4a0e4", "#a6d890", "#e294ab", "#e2cf8b", "#93b9ed", "#c4c9ce"];
const personalities = ["Xenophobic", "Ruthless", "Aggressive", "Erratic", "Honorable", "Pacifist", "Dishonored"];
let selected = params.get("empire") || "all";
let lastKey = "";
let rosterKey = "";
let cached;
function node(tag, cls, text) { const e = document.createElement(tag); if(cls) e.className = cls; if(text !== undefined) e.textContent = text; return e; }
function render(state) {
  cached = state;
  const record = state.latest;
  const age = state.updated ? Math.floor(Date.now()/1000 - state.updated) : null;
  $("status").textContent = state.demo ? "Illustrative demo" : !record ? "Waiting for engine" : record.finished ? "Match ended" : state.interval === 0 ? "Paused / manual turns" : age > 30 ? "Waiting for next turn" : "Journal connected";
  $("lamp").classList.toggle("live", !!record && !state.demo && age < 30);
  $("date").textContent = record ? `STARDATE ${(record.stardate/10).toFixed(1)}` : "—";
  $("play").disabled = state.demo || !record || record.finished;
  $("pause").disabled = state.demo;
  $("notice").textContent = state.demo ? "ILLUSTRATIVE DEMO · Sample journal data, not a running game." : !record ? "Waiting for a journal. Start a single-player game and activate ALLAI." : record.finished ? "Match complete. Automatic turn advancement has stopped." : `Last journal received ${age}s ago. ${state.errors ? `${state.errors} malformed records skipped. ` : ""}${record.dropped_events ? `${record.dropped_events} events omitted at the per-turn limit. ` : ""}Spectator statistics are omniscient; journal entries describe recorded decisions.`;
  if (!record) return;
  const roster = JSON.stringify(record.empires.map(e => [e.id, e.name]));
  if (roster !== rosterKey) {
    rosterKey = roster;
    $("empire").replaceChildren(new Option("All empires", "all"), ...record.empires.map(e => new Option(e.name, String(e.id))));
    $("empire").value = selected;
  }
  const key = JSON.stringify([record, state.journals, selected, $("evidence").checked]);
  if (key === lastKey) return;
  lastKey = key;
  const cards = [];
  for (const empire of record.empires) {
    if (selected !== "all" && String(empire.id) !== selected) continue;
    const card = node("article", `empire${empire.eliminated ? " eliminated" : ""}`);
    card.style.setProperty("--accent", colors[empire.id % colors.length]);
    const head = node("div", "empire-head");
    const title = node("div", "empire-title");
    title.append(node("h2", "", empire.name), node("span", "personality", empire.eliminated ? "Eliminated" : personalities[empire.personality] || "Unknown"));
    const stats = node("div", "stats");
    for (const [label, value] of [["colonies",empire.colonies],["population",empire.population],["ships",empire.ships]]) {
      const stat = node("div", "stat"); stat.append(node("strong", "", value), node("span", "", label)); stats.append(stat);
    }
    head.append(title, stats);
    const entries = node("div", "entries");
    const history = [...(state.journals[empire.id] || [])].reverse().slice(0, overlay ? 5 : 20);
    if (!history.length) entries.append(node("div", "empty", "No recorded diplomatic decisions yet. The empire's journal will appear here as decisions are made."));
    for (const event of history) {
      const entry = node("div", "entry"), meta = node("div", "entry-meta");
      meta.append(node("span", "kind", event.kind), node("span", "", (event.stardate/10).toFixed(1)));
      entry.append(meta, node("p", "", event.text));
      if ($("evidence").checked) { const detail = node("details"); detail.open = true; detail.append(node("summary", "", "Engine evidence"), node("div", "", event.evidence)); entry.append(detail); }
      entries.append(entry);
    }
    card.append(head, entries); cards.push(card);
  }
  if (!cards.length) cards.push(node("p", "empty", "This empire is not present in the current match."));
  $("journals").replaceChildren(...cards);
}
$("empire").addEventListener("change", () => { selected = $("empire").value; if(cached) render(cached); });
$("evidence").addEventListener("change", () => { if(cached) render(cached); });
async function control(interval) {
  try { const response = await fetch("/api/control", {method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({interval})}); if(!response.ok) throw new Error(); }
  catch { $("notice").textContent = "Could not update playback controls. Check the local stream server."; }
}
$("play").addEventListener("click", () => control(Number($("speed").value)));
$("pause").addEventListener("click", () => control(0));
async function poll() {
  try { const response = await fetch("/api/state", {cache:"no-store"}); if(!response.ok) throw new Error(); render(await response.json()); }
  catch { $("status").textContent = "Connection lost"; $("lamp").classList.remove("live"); $("notice").textContent = "Stream server disconnected. Displaying the last received journal."; }
  setTimeout(poll, 1000);
}
poll();
