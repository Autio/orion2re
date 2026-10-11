# Manual: Gameplay differences

Orion2 Redone keeps the original game's overall rules and balance while retaining supported corrections and optional extensions. This page summarizes the differences most likely to affect play. The [game mechanics](../game-mechanics/index.md) pages describe the current rules without patch history.

## Galaxy generation

Galaxy generation protects home systems and nearby expansion opportunities more consistently. Home systems contain at least three planets, non-Lithovore players receive a reachable food world, and map cleanup preserves required worlds while enforcing the map's satellite limit. Star systems use the visible nebula mask consistently.

See [Galaxy generation](../game-mechanics/galaxy-generation.md).

## Combat

Initiative combat uses every point of beam attack and combat speed, with deterministic tie-breakers. Time Warp Facilitator turns, defensive fire, missile movement, repairs, retreat, and end-of-combat processing use a consistent turn sequence. Defensive fire remains available in non-initiative combat but is disabled by default when initiative is enabled.

Boarding, fighter recovery, missile warnings, planetary bombing, damage to weapons and special systems, and the effects of Stasis Field, Black Hole Generator, Tractor Beam, cloaks, and defensive devices have been corrected so displayed state and actual combat state agree.

See [Tactical combat](../game-mechanics/tactical-combat.md), [Strategic combat](../game-mechanics/strategic-combat.md), [Special systems](../game-mechanics/special-systems.md), and [Raiding and orbital assault](../game-mechanics/raiding-and-orbital-assault.md).

## Ships, fleets, and defenses

Navigator effects follow the leader's current fleet immediately. Automatic ship designs use the same space, cost, and combat-stat rules as manual designs. Landing colony and outpost ships updates command use immediately, and queued landers use the best available armor.

Combat satellites support configured weapon modifications and space allocations, spread missiles across available slots, and use their own size-class durability. Fighter Garrison flights and returning fighters fill available bays without destroying usable capacity.

See [Ships and fleets](../game-mechanics/ships-and-fleets.md), [Satellites and ground defenses](../game-mechanics/satellites-and-ground-defenses.md), [Ship refitting](../game-mechanics/ship-refitting.md), and [Ship limits](../game-mechanics/ship-limits.md).

## Colonies and production

Colony population is capped at 42. Production and maintenance changes take effect consistently, unavailable queue products are discarded safely, and removing a building cannot leave purchased production attached to an invalid product. Optional multibuild can complete several queued products from one turn's production, while configurable queue presets provide a separate human-colony autobuild system.

Zero-population colonies remain only while buildings or incoming colonists keep them viable. They can be bombarded, and can be invaded while at least one building remains.

See [Colonies and production](../game-mechanics/colonies-and-production.md), [Population growth](../game-mechanics/population-growth.md), [Build queue management](build-queue.md), and [AI production rules](../game-mechanics/ai-production-rules.md).

## Research, diplomacy, leaders, and events

Research generated before a turn-zero technology choice is retained. Technology transfers no longer omit valid technologies, surrender transfers the defeated empire's technology and map knowledge, and human technology exchanges are not filtered by AI trade preferences.

Spy overflow no longer provides an exploitable full-price refund. Leader combat and espionage skills apply to their intended actions, and leader recruitment order is randomized among players. Diplomatic events affect relations, while monster, comet, nova, pirate, and unstable-wormhole events apply their reported outcomes without the original overflow and timing errors.

See [Research and technology](../game-mechanics/research-and-technology.md), [Spying](../game-mechanics/spying.md), [Leaders](../game-mechanics/leaders.md), and [Random events](../game-mechanics/random-events.md).

## Configurable differences

Many retained rules can be changed by a mod or in `data/orion2re.cfg`, including initiative, defensive fire, boarding ranges and damage, strategic crew bonuses, colony extinction, multibuild, and human autobuild presets. The [configuration reference](../../generated/configuration/index.md) gives the authoritative defaults and accepted values.