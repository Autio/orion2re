# Manual: Missile defenses

Missile defense happens in stages. Defensive weapons can destroy missiles before impact; Lightning Field can destroy surviving ordnance; then cloaking, displacement, and missile evasion can prevent individual warheads from hitting. Torpedoes bypass weapon interception but still face the later defensive systems.

## Intercepting missiles

Missiles can be shot down by regular and PD mount beam weapons, Interceptors, Anti-Missile Rockets, Pulsars, Plasma Flux, and Spatial Compressors. Spatial Compressors can also fire automatically against incoming fighters.

Torpedoes cannot be targeted or damaged by these interception weapons.

## Special defensive systems

Three special systems can defend against both missiles and torpedoes:

- Lightning Field: has a 50% chance to destroy each missile, torpedo or fighter attempting to strike the equipped ship.
- Cloaking Device: Under the default rules, missile warheads and torpedoes have an independent 50% chance to miss, but only while the device is active and the ship remains cloaked. Mods can replace this fallback with a configured cloak-evasion value.
- Displacement Device: Missile warheads and torpedoes have a 30% chance of missing completely, regardless of any other equipment or considerations.

Note that Lightning Field acts before MIRV missile warheads separate but Cloak and Displacer effects happen afterwards.


## Missile evasion

Missiles and torpedoes that survive interception and reach a ship have a 100% base chance to hit unless one of the ship's later defenses stops them. Missile evasion can come from the following sources:

| Source | Evasion bonus | Notes |
| --- | ---: | --- |
| ECM Jammer | +70 | - |
| Multi-Wave Jammer | +100 | - |
| Wide Area Jammer | +130 | Also gives +70 to the rest of the fleet.* |
| Inertial Stabilizer | +25 | - |
| Inertial Nullifier | +50 | - |
| Ship Defense -20 racial trait | -20 | - |
| Ship Defense +25 racial trait | +25 | - |
| Ship Defense +50 racial trait | +50 | - |
| Green crew | 0 | - |
| Regular crew | +7 | - |
| Veteran crew | +15 | - |
| Elite crew | +25 | - |
| Ultra Elite crew | +37 | - |
| Helmsman fleet leader | Half the Helmsman value | Uses the applicable active fleet-leader bonus. |

*The Wide Area Jammer fleet bonus does not stack with other jammers; only the best jammer bonus applies.

The probability of a single missile warhead getting jammed is:

> `jamming chance = defender missile evasion - attacker scanner bonus`

This probability is halved if the missile has ECCM (Electronic Counter Countermeasures).

Scanner values reduce the target's missile evasion:

| Scanner | Evasion adjustment |
| --- | ---: |
| Space Scanner | 0 |
| Tachyon Scanner | -20 |
| Neutron Scanner | -40 |
| Sensors | -70 |

The jamming chance is calculated for each warhead, so it may happen that not all warheads of a MIRV missile hit, but some get jammed.

Note that all monsters, the Guardian and Antarans have a Neutron Scanner onboard in tactical combat, and Sensors in strategic combat.

Dragon Breath is a special torpedo with standard damage of 300 and dissipation of 15 damage per square. Scanner-based missile evasion does not stop it, although other applicable defenses such as Lightning Field, an active Cloaking Device, and Displacement Device still act normally.


### Example

The probability that an ECCM warhead guided by a Tachyon Scanner gets jammed by a target with the Wide Area Jammer fleet bonus (+70), Inertial Stabilizer (+25), a racial penalty (-20), Regular crew (+7), and a Helmsman value of 10 is:

> `P = [(70 + 25 - 20 + 7 + (10 / 2)) - 20] / 2 = 33%`
