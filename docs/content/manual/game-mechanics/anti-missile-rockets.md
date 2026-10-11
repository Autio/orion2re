# Manual: Anti-missile rockets

Anti-Missile Rockets are a short-range defense against incoming missiles. They are especially useful against advanced missiles because missile type, hit points, and modifications do not affect their chance to intercept.

## Using Anti-Missile Rockets

Anti-Missile Rockets (AMR) have a maximum range of 15 squares and can target only missiles, not torpedoes or fighters. They can be fired manually and can also fire automatically when missiles approach their ship.

Unlike most special weapons, which fire every weapon in a stack together, AMR launch one rocket at a time. Each rocket makes a separate hit roll. A successful rocket destroys one missile in the targeted stack.

## Range and chance to hit

AMR use a ceiling-style distance conversion that shifts the usual squares-to-range calculation by one square. This treats the firing ship's center as the theoretical range-0 case and the next three-square group as AMR range 1.

| Range value | Standard range (squares) | AMR distance case |
| ---: | --- | --- |
| 0 | 0 | Identical center coordinates |
| 1 | 1-3 | 0-2 |
| 2 | 4-6 | 3-5 |
| 3 | 7-9 | 6-8 |
| 4 | 10-12 | 9-11 |
| 5 | 13-15 | 12-14 |
| 6 | 16-18 | 15-17 |

The 15-17 conversion bucket extends beyond the weapon's legal range; AMR cannot fire beyond 15 squares.

The AMR hit threshold is:

> `AMR hit threshold = 70 - trunc((Range + 2) * 10 / 3) + 1`

The game rolls from 1 through 100. The rocket hits only when the roll is lower than the threshold, so the actual probability is one percentage point below the threshold.

| AMR range (squares) | Theoretical 0 | 0-2 | 3-5 | 6-8 | 9-11 | 12-14 | 15 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Range value | 0 | 1 | 2 | 3 | 4 | 5 | 6 |
| Hit threshold | 65 | 61 | 58 | 55 | 51 | 48 | 45 |
| Actual chance to hit | 64% | 60% | 57% | 54% | 50% | 47% | 44% |

Within 0-2 squares of the ship's center, each AMR therefore has a 60% chance to destroy one missile.

The exact distance to a missile is not displayed during combat, so these ranges normally have to be judged from its position.

The theoretical center case occurs only if the missile and firing ship have identical center coordinates.
