# Manual: History graphs

History graphs help compare long-term trends between known empires. They are most useful for seeing whether an empire is expanding, building up its fleet, or pulling ahead technologically; they are not exact inventories.

You can select one or more of Population, Buildings, Fleet, and Technology. Selected values are combined in the displayed graph. The game selects a scale that fits the largest values and retains up to 350 turns of history.

## Graph smoothing

Smoothing is enabled by default. It makes abrupt changes easier to read, especially in the fleet graph, but can make early population growth appear flat and can delay visible changes. Disable it with `interface history_graph_smoothing = false;` when you prefer a more direct view of the recorded values.

## Graph units

### Population

Each unit of population counts as 1. Population being transported between colonies is included, so moving colonists does not create a temporary drop in the empire's total.

### Buildings

Each completed building contributes its production cost. A high Buildings line therefore indicates accumulated colony infrastructure rather than current production output.

### Fleet

| Hull | Frigate | Destroyer | Cruiser | Battleship | Titan | Doom Star |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Graph value | 20 | 40 | 80 | 160 | 320 | 640 |

Only hull size matters. Weapons, armor, and special systems do not change fleet graph value, so the graph compares fleet size more reliably than fleet quality.

### Technology

Technology value is based on the research cost of each field and how much of that field is known. Race-specific technology deletions do not reduce the number of applications used as the divisor.

Gaining an entire field contributes its research cost.

For a field with several applications, gaining one application without completing the field contributes:

> `technology value = research cost / (1 + number of applications in the field)`

Completing a field through normal research usually records both the completed field and its selected application, producing:

> `technology value = 2 * research cost / (1 + number of applications in the field)`

Thus, in a field with two applications, a separately acquired application normally contributes one-third of the field cost, while completing normal research contributes two-thirds. For three applications the corresponding values are one-quarter and two-quarters; for four they are one-fifth and two-fifths.

The calculation uses the resulting field and application status rather than recording how an application was acquired. Diplomacy, spying, leaders, or discoveries can therefore affect the graph indirectly by adding an application or completing a field.
