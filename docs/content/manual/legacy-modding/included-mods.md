# Manual: Included game mods

Version 2.00 includes the optional packages below. They are installed with the game but are not enabled automatically. Enable a package by adding its line to `data/orion2re.cfg`, then restart the game.

Dependencies must also be installed. The game reports missing dependencies and conflicts during startup. See [Using mods](index.md) for installation and troubleshooting.

## Rule and balance packages

| Package | Purpose | Requirements and compatibility | Enable line |
| --- | --- | --- | --- |
| Community Patch 1.50 (`v150`) | Applies the standard 1.50 patch rules and data retained by version 2.00. | Base package for several other included mods. | `enable v150;` |
| Community Patch 1.50 Improved (`v150i`) | Adds rebalanced race picks, improved AI ship designs, and revised tactical-combat rules to the standard 1.50 package. | Requires `v150`. | `enable v150i;` |
| Community Patch 1.50 Improved Multiplayer (`v150i-mp`) | Adds multiplayer race-pick, map, diplomacy, and interface changes for 1.50 Improved. | Requires `v150i` and the script-bearing `mirror_hw` package. All players need compatible copies of the complete dependency set. | `enable v150i-mp;` |
| ICE (`ice`) | Applies ICE 35 balance, difficulty, map, resource, and new-game behavior. | Requires `v150`. | `enable ice;` |
| ICE-X (`ice-x`) | Adds harder AI and Antaran settings to ICE. | Requires and loads after `ice`. | `enable ice-x;` |
| VDC-GM3 (`vdc`) | Applies the VDC-GM3 balance, map, interface, resource, and new-game technology changes. | Requires `v150`, `mirror_exhw`, and `noreport`. Conflicts with `ice`, `ice-x`, `mirror`, `mirror_hw`, and `v150i`. | `enable vdc;` |
| VDC-GM3 Map (`vdc_gm3_map`) | Applies the canonical VDC-GM3 galaxy-generation table. | Requires `vdc`. | `enable vdc_gm3_map;` |
| Version 1.2 mod (`v120`) | Approximates version 1.2 rules, including its Creative cost, Psilon design, selected ship designs, and selected weapon values. | Requires `v150`. Conflicts with `ice`, `melee`, `v150i`, and `vdc`. | `enable v120;` |
| Melee (`melee`) | Provides the 1.50 two-player Melee mode and its compact tournament map. | Intended for two-player tactical competition rather than a normal campaign. | `enable melee;` |

## Galaxy and setup packages

| Package | Purpose | Requirements and compatibility | Enable line |
| --- | --- | --- | --- |
| Full Mirror (`mirror`) | Splits the galaxy into identical mirrored sectors so every player receives the same start. | Do not combine with a profile that declares a conflict with it. | `enable mirror;` |
| Full Mirror except home systems (`mirror_exhw`) | Mirrors galaxy sectors while preserving each player's generated home system. | Required by `vdc`. | `enable mirror_exhw;` |
| Only Home Systems Mirrored (`mirror_hw`) | Copies one selected player's unoccupied home-system orbits to every other home system. | Required by `v150i-mp`; conflicts with `vdc`. | `enable mirror_hw;` |
| Random Technology Tree (`randtech`) | Randomizes the global technology tree before player technologies are assigned. | Uses a script; all multiplayer participants must use a compatible copy. | `enable randtech;` |

## Optional rule packages

| Package | Purpose | Requirements and compatibility | Enable line |
| --- | --- | --- | --- |
| No Diplomacy Reports (`noreport`) | Disables diplomacy reports and technology exchange, demand, and offer actions. | Required by `vdc`. | `enable noreport;` |
| Hidden Tech Tree (`treeless`) | Prevents browsing future technologies in the Research screen. | Can change planning information available to players. | `enable treeless;` |

## Important rule changes

The package table gives a short description. The following larger changes are useful to know before starting a game.

### Community Patch 1.50 Improved

The `v150i` package changes more than AI ship designs and race-pick costs. Toxic colonies can become Barren by building a Recyclotron. Excess research beyond twice the current field cost is retained, and accumulated research is not discarded when the current technology is acquired from spying, capture, or an artifact world.

Tactical combat uses strict ship initiative: each ship can Wait once, and ships that waited act in reverse initiative order. Ship statistics update during the combat turn as damage, cloaking, tractor beams, captures, and other effects occur. Missiles and torpedoes move at the end of each turn, volleys launched in different turns do not merge, and long-range beam attacks use the simplified accuracy formula.

### Melee

The `melee` package is intended for a two-player tactical contest rather than a normal campaign. Use a Small galaxy. Players begin without the normal starting ships, command costs are heavily weighted by hull size, and colonies can complete multiple ships in one turn when enough production is available.

### ICE

The `ice` package changes race picks, starting conditions, map generation, AI difficulty, resources, and other balance rules together. It uses at least four planets in each home system, starts with ships but no starting outpost ship, and should be treated as a complete game profile rather than a small optional adjustment.

### VDC-GM3

The `vdc` package is also a complete game profile. It uses at least five planets in each home system, gives an outpost ship with the applicable starting fleet, removes the random part of research breakthroughs, forces ship initiative, and limits tactical combat to 25 turns. The optional `vdc_gm3_map` add-on supplies its canonical galaxy-generation table and requires `vdc`; enabling `vdc` alone does not enable the map add-on.

### Version 1.2 mod

The `v120` package approximates older version 1.2 rules. Home systems may contain only one planet and ship initiative is disabled. Use it when those older rules are wanted, not as a general compatibility package for version 1.2 saved games.

## Enabling a package

Enable the package you want. Its installed dependencies are activated automatically. For example:

```text
enable v150i;
```

For Improved Multiplayer:

```text
enable v150i-mp;
```

When enabling more than one independent package, list the highest-priority package first. If the game reports a conflict or required order, correct the enabled package set as directed and restart.