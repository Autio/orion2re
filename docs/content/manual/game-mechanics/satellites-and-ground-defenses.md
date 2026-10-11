# Manual: Satellites and ground defenses

## Combat satellites

Star Bases, Battlestations, and Star Fortresses use their own hull-table rows for computer and shield subsystem durability. Their special-system size calculations continue to use the equivalent Cruiser, Battleship, and Titan classes.

Satellite beams can use allowed weapon modifications. Point-defense beams exclude armor-piercing and shield-piercing modifications by default, while missile modifications and their miniaturization levels follow the configured defense rules. A 360-degree beam costs 50% more space than a standard arc.

Available weapon space is divided among heavy beams, regular beams, point-defense beams, and missiles or torpedoes according to `combat satellite_space_allocation`. Satellite strength estimates use the same allocation.

Missiles are spread across the satellite's three missile slots so separate volleys can engage separate targets. One through three missiles use one slot each, four are split two and two, and larger loads are distributed as evenly as possible. Each slot can hold up to 99 missiles.

## Ground batteries and installations

Ground Battery beams pay the same 50% cost for a 360-degree arc; their total space includes the corresponding default compensation. Ground installations are destroyed when incoming damage reaches their listed hit points, rather than requiring an additional point.

## Fighter Garrisons and returning flights

A Fighter Garrison has two banks. By default, each bank carries five Interceptors, three Bombers, or two Heavy Fighters according to the craft used. Returning flights fill every compatible vacant bay, including partially filled slots, rather than replacing a slot's capacity. A returning flight restores the bank's ammunition but leaves no shot available to relaunch it that turn, and cannot refill bays that were destroyed by damage or raids.

Completing or destroying a Fighter Garrison, combat satellite, ground defense, or Artemis System Net updates hostile fleets waiting in the system so the next combat decision reflects the current defenses.

## Ground-unit limits

Colonies enforce their maximum number of marines and armor units consistently. Excess troops left after an invasion are removed over subsequent turns, including at colonies without Marine Barracks and after capacity-affecting buildings are sold.