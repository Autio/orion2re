# Manual: Spherical damage

Spherical weapons affect every eligible target around the firing ship. They are useful against groups of missiles and fighters and can damage several nearby ships at once, but their range and damage resolution differ from ordinary beams.

## Spherical Weapons

Range to ships is measured from the firing ship to the target's edge. A ship must be strictly inside the radius, while a missile or fighter at exactly the radius can still be affected. The default ship-targeting ranges are 6 squares for Pulsar and Plasma Flux and 4 squares for Spatial Compressor.


### Pulsar

Listed damage is 2-24 points per size class of affected ship, in a 6-square radius.


### Plasma Flux

The Eel’s weapon is very similar to the Pulsar but has 10-40 points of damage listed.


### Spatial Compressor

Can do 4-32 damage irrespective of size class and does all damage to structure only, ignoring shields and armor, similar to a Gyro or the ESD mod. Range is 4 squares. Contrary to Pulsars, Compressors autofire against incoming missiles and fighters.


### Engine explosion

If a ship's drive is destroyed, it causes an explosion with damage potential equal to 5 times the maximum engine hit points, before Reinforced Hull. A Quantum Detonator triples this value. Explosion damage falls with squared distance rather than using the ordinary beam-dissipation table.


### Satellites

Satellites have no drive, however if an orbital platform self-destructs it will still cause an explosion as if it has an engine of its corresponding ship size class (Star base gets Cruiser engine hp).


## Damage calculation

Actual damage is calculated independently for missiles and fighters, and then again for ships:


### Missiles and Fighters

Each affected fighter stack makes a 50% evasion check against spherical damage. If the stack fails this check, its fighters are then resolved in the same way as missiles.

For each spherical weapon in a slot a damage value is generated:

> `D = (DamageMin + random(max - min + 1) - 1) * ordnance`

Ordnance does not affect an engine explosion. Explosion damage still uses the flyer destruction rolls below after its distance-based damage is calculated.

The sum of all these damage values, `aggD`, is multiplied by 25 and divided by the hit points of one missile or fighter in the stack. The game then generates `random(100)` for each missile or fighter and compares it with the result. The target is destroyed when:

> `aggD * 25 / hit points >= random(100)`

This can also be written as:

> `aggD * 25 / random(100) >= hit points`

clarifying the phrase from the original game manual that “Fighters and missiles take damage as if they were size class one-half.”


### Ships

For ships, the process is repeated and a new damage value (`D`) is generated. As with missiles and fighters, the total damage per slot is calculated. Under the default rules, the resulting value is the structure damage for Spatial Compressor. For other spherical weapons, additional `random(aggD)` rolls follow, where the number of rolls is the target's size class (0 through 5) plus 1. Thus a Frigate gets one roll, a Destroyer two rolls, and so on. For each roll, a first result of 1 contributes 1 damage; any other first result is discarded and replaced by one second roll. The sum of the retained results is the damage against that ship.


### Final notes

Unlike beams, spherical weapons always use all weapons from the slot. As a result, it makes a difference for missiles and shields whether a design has one slot with five weapons or five slots with one weapon. Weapons fired from one slot deal cumulative damage rather than resolving separately.

Every described damage calculation is done per target, so damage values for two different missile stacks or two separate, same-size class ships that get hit from the same spherical weapon shot can differ.

Finally, damages of ship explosions can dissipate to 0 for missiles, but have a minimum damage of 1 against ships.
