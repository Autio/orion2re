# Manual: Tactical combat

## Ship initiative

When ship initiative is enabled, ships from both sides act in one shared round. Initiative is:

```text
tactical attack value + (current combat speed x 10)
```

Higher initiative acts first. Ties are resolved in this order:

1. space monsters before other ships;
2. higher crew level;
3. combat-only objects before ordinary ships;
4. higher combat-object ID between combat-only objects, with the planet acting last;
5. greater crew experience; and
6. lower strategic ship ID between ordinary ships.

The ordinary Wait action remains available. The `H` hotkey waits all eligible ships, but a defending player cannot use it to force the attacking side to exhaust all actions first. `T` marks all ships done.

## Time Warp Facilitator turns

A Time Warp Facilitator grants its ship a second full action phase. During that phase, non-TWF ships do not perform ordinary defensive fire or receive TWF-only refreshes. Missiles still move between action phases, and anti-missile weapons may respond to incoming missiles and fighters.

TWF ships receive their additional shield recharge, repair, weapon and movement refresh, cloak update, and ongoing Plasma Web or Black Hole Generator effects in the TWF sequence. A ship under a continuing Black Hole Generator therefore reaches its destruction point twice as quickly when it takes two turns per round.

With initiative enabled, an Energy Absorber retains stored energy between rounds so a ship can use energy received after its action.

## Defensive fire and missile alerts

Regular-mount beams can perform defensive fire during an opponent's movement. Defensive fire is always active without ship initiative and is disabled by default with initiative; `combat defensive_fire` changes the initiative-mode rule.

Movement and rotation update the ship's current position before range checks. Defensive fire, point defense, Spatial Compressor fire, and missile alerts therefore use the ship's actual intermediate position and facing. A movement cancelled by a missile alert does not move associated interceptor flights.

## Damage and repair

When damage leaves fewer working weapons than remaining shots in a slot, its available shots are reduced immediately. A repaired weapon, shield generator, drive, or special system resumes operation. A repaired shield generator can recharge its arcs, and an immobilized ship receives the minimum post-combat drive repair needed to move if no other repair source is available.

Ground installations and civilian structures are destroyed when damage reaches their hit points. Planetary bombing range is symmetric around planets of every size.

## Boarding and capture

Crew experience affects both raids and captures, including actions made by Assault Shuttles. Ships in Stasis or under an active Phasing Cloak cannot be boarded. A ship's immobilized display follows whether Tractor Beam or Black Hole Generator effects currently make it boardable.

When a telepathic player captures a ship, its Done state is cleared. Its Wait state is also cleared in strict initiative mode so the new owner can choose whether to wait. A ship captured while retreating remains in combat.

Boarding range, black-hole restrictions, marine capacity, raid damage, and the number of components a marine can damage are configurable. See [Raiding and orbital assault](raiding-and-orbital-assault.md).

## Fighters and missiles

Fighters launched at point-blank range strike immediately from every direction. Flights track their actual target chain, continue pursuing stationary missiles, and refill available carrier bays when they return. Returning flights cannot restore bays destroyed by combat damage or raids.

Manual rotation participates in missile-warning checks. Point defense and other on-impact defenses use current coordinates rather than the position where movement began.

## Retreat and combat completion

Running out of movement and ammunition does not automatically mark a ship Done. It remains selectable for actions such as retreating, self-destructing, or boarding when those actions are available.

An immobilized ship cannot disappear by completing a retreat turn. AI ships without usable ammunition normally retreat unless their continuing fleet effect makes remaining valuable; a leader, Warp Dissipator, or Wide Area Jammer can keep them in combat. `combat human_fleet_auto_retreat` applies the same evaluation to human fleets under Auto combat.

During a round, combat can end when one side has won and the defeated side has no remaining missiles or colony. The normal end-of-round check can finish combat even if missiles or a colony remain. This gives the remaining side a chance to act against last-flight missiles or a planet without prolonging an otherwise decided battle.