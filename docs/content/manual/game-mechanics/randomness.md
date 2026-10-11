# Manual: Randomness

The game uses a shared sequence of pseudo-random results for events, success checks, damage, and other uncertain outcomes.

## What happens after reloading

The current random state is stored in every save. If you reload and repeat exactly the same actions in the same order, the same checks normally produce the same results. For example, reloading immediately before ending the turn will not by itself prevent an Antaran attack that occurred on that turn.

Taking a different action can consume one or more random results and change what happens afterward. A diplomatic action, combat, or another check before ending the turn may therefore change a later event. This does not guarantee a more favorable outcome; it only changes the point reached in the sequence.

## Random function

The generator uses a game-wide unsigned 32-bit state. The source calls this value the random seed, although after initialization it is more precisely the current generator state.

For a positive limit `n`, `Random(n)` returns an integer from 1 through `n`, inclusive. Each value in that range has the same chance. The generator advances its state before producing a result; this is true even for `Random(1)`, which can only return 1.

Calling `Random(0)` also returns 1, but does not advance the state. The limit is an unsigned 32-bit value, so the game does not give negative limits a separate meaning.

For ordinary positive limits, the generator repeatedly advances its 32-bit state until the result falls within an evenly divisible range. It then divides that range into `n` equal parts and returns the corresponding number from 1 through `n`. Discarding the extra states prevents some results from being slightly more likely than others.

## Multiplayer expectations

Multiplayer games rely on every participant performing the same synchronized checks. Players should use matching game data, settings, and mods; a rule-changing mod that performs different checks can cause players to receive different results.
