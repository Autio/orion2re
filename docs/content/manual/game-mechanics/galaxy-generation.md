# Manual: Galaxy generation

## Home systems

Every home system is generated with at least three planets. The homeworld and other protected home-system objects are retained if the map must reduce its total number of satellites.

For each non-Lithovore player, galaxy generation ensures that at least one actual food-producing planet is available within six parsecs of the home system. Gas giants and asteroid belts do not satisfy this requirement, and the qualifying system is not assigned a space monster.

Galaxy generation also ensures a nearby star with at least one colonizable planet. If a generated system must be replaced to meet this requirement, it is regenerated using the active map tables rather than receiving fixed star and planet properties.

## Satellite limit

On galaxies with up to 72 stars, generation retains at most 250 planets and asteroid belts; gas giants do not count toward this limit. If initial generation exceeds the limit, removable objects are trimmed while preserving homeworlds, Orion, required food worlds, and valid system links. Extended galaxies use their larger planet capacity instead of the 250-object limit.

## Nebulae and black holes

A system's nebula status follows the visible nebula mask. Small dark pockets in the artwork can therefore count as being outside the nebula.

Black holes are not retained inside nebulae. A conflicting black hole is replaced with a normal star generated from the active map tables.

## Special links

Generated wormholes do not connect to Orion.

Galaxy tables, distances, map sizes, home-system requirements, and special-object placement can be changed by mods. See the [galaxy and events configuration reference](../../generated/configuration/galaxy-and-events.md).