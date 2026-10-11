# Manual: Random events

Random events use the saved random-number state described in [Randomness](randomness.md), so reloading and repeating the same actions produces the same event checks.

## Monsters and disasters

A victorious Toxic Amoeba changes its target planet to Toxic and removes its food output. If the colony survives, its output is recalculated for the new climate; if it is wiped out, the colony is destroyed normally.

If a supernova countdown expires, every planet in the system becomes Radiated, planetary alterations and food output are removed, and colonies in the system are destroyed. Research assigned to the disaster can delay or prevent this outcome.

Comets are destroyed as soon as progress reaches 100%. Monster and comet timing respects Hyperspace Flux restrictions. An unstable-wormhole event can affect a fleet with two turns remaining in hyperspace.

## Diplomatic and economic events

Diplomatic Blunder and Marriage events change relations between the affected empires. Pirate raids subtract funds without turning an arithmetic overflow into income for a wealthy empire.