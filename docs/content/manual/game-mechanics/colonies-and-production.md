# Manual: Colonies and production

## Production timing

New buildings, spies, and freighters begin charging maintenance on the following turn. They do not grant a one-time cash compensation by default. A newly completed freighter or food-producing building becomes available to food distribution on the following turn, and new population and economic buildings affect tax income on the following turn.

`colony freighters_cash_bonus` can grant a configurable one-time freighter completion bonus; its default is zero.

Selling a building recalculates colony output immediately. If the sale invalidates a queued product, that product is removed. If it had been purchased, the purchase is reversed rather than leaving bought production attached to an invalid queue entry.

Food Replicators consume production only when sufficient production is available, so they cannot make colony production negative.

## Build queues and multibuild

A colony queue holds seven products. Technology, existing structures, local targets, population, race and government rules, and ship-design availability are checked when products enter or advance through the queue. Invalid products are skipped or removed safely.

Normally a colony completes at most one queued product per turn. With `colony multibuild` enabled, stored, bought, and current net production are combined and can complete several successive products. Unspent production carries into the first unfinished product. Housing and Trade Goods stop further completion for that turn.

With `colony multibuild_nonbuilding_products_only` also enabled, ships (including colony and transport ships), freighters, androids, and spies can complete multiple times per turn. A colony building or project completes at most once per turn and ends further completions, even if it follows a non-building product in the queue. This flag has no effect without `colony multibuild`.

Queue presets and human-colony Autobuild are described in [Build queue management](../user-facing-changes/build-queue.md). AI and legacy colony selection are described in [AI production rules](ai-production-rules.md).

## Autobuild timing

Queue-driven human Autobuild applies its selected preset when the player submits the next turn, before turn processing begins. Applying a preset does not itself recalculate colony output.

## Population and ghost colonies

The supported population limit is 42. Queue and migration checks enforce both this absolute limit and the lower capacity applicable to the colony's races. See [Population growth](population-growth.md) for the capacity formula.

A colony with no population can remain as a ghost colony while it has buildings. It cannot start new buildings, and each existing building has a 10% chance to be destroyed each turn. Once no buildings remain, the colony is lost immediately unless colonists are inbound. With `colony zero_colony_lost` enabled, it is lost on the next turn even with inbound colonists.

`colony planet_extinction` allows starvation to remove the final colonist for human players. Ghost colonies can be bombarded, and can be invaded while at least one building remains.

## Colony placement restrictions

Each player can build or queue only one Dimensional Portal in a star system. Only one Artemis System Net can be present in a system. A Colony Base can be queued only when the system contains an eligible planet not already covered by another queued base.