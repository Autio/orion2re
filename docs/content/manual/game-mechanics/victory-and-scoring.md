# Manual: Victory and scoring

Your long struggle against hostile forces of the rest of the galaxy can only end in triumph or ignominy. If your last colony is destroyed or conquered, your empire no longer exists. In this case, your personal fate as an ex-emperor is something better left unmentioned.

On the other hand, you might win.


## Orion Spoils

Despite the game's name, defeating the Guardian of Orion does not automatically give you the game, although the benefits acquired from doing so brings ultimate victory close. By default you are rewarded with the ancient Death Ray technology. This prize can be changed to another technology application; for example, `leaders orion_reward_technology application = death_ray;` selects the default reward.


## Win Conditions

There are three ways to win the game: exterminate all opponents; get elected as supreme leader of the galaxy; or lead a successful assault against the Antaran homeworld by sending your fleet through a Dimensional Portal. A Council victory requires at least two-thirds of all votes, rounded up. Each empire receives one vote per ten population, also rounded up.

If you time it right, you can achieve two win conditions: defeat the Antarans and all opponents, provided the battle at Antares happens last; or defeat the Antarans and also win by election.


## Score Calculation

Scores are stored and displayed as signed 16-bit values. At a 100% race score modifier, 32,767 is followed by -32,768 and 65,535 appears as -1. Unspent positive race picks increase the final score modifier; zero or negative remaining picks leave it at 100%. Evolutionary Mutation adds four picks when this modifier is calculated.

The difficulty level has no effect on the score, but the following items matter:


### Time

A starting value is calculated from galaxy size and number of players:

| Galaxy size | Points per player |
| --- | ---: |
| Small | 100 |
| Medium | 120 |
| Large | 140 |
| Huge | 160 |
| Maximum | 180 |

One point is subtracted from the starting time value each turn. Time score can go negative if the number of played turns exceeds the starting value. An eliminated empire with no remaining population receives no time score.


### Population

+1 point for every unit of population.


### Player Elimination

+50 points for every eliminated player.


### Captured Colonists

The game keeps a cumulative count of captured colonists. Their score varies by galaxy size because larger galaxies provide more opportunities to capture population:

| Galaxy size | Points per captured colonist |
| --- | ---: |
| Small | 2 |
| Medium | 1 |
| Large | 2/3, total rounded down |
| Huge | 1/2, total rounded down |
| Maximum | 2/5, total rounded down |


### Technology

Each completed technology field adds 3 points. Under the default starting-field rules, Tech field 0 and Engineering are known from the start, as are the five Pre-warp fields Nuclear Fission, Cold Fusion, Chemistry, Electronics, and Physics. Average Technology therefore starts with 21 technology points. Mods can change the starting fields.

Stolen, received, or found technology does not increase the score unless it completes a field. Each Hyper-Advanced branch with at least one level adds 5 points; additional levels in that branch add no further score.


### The Guardian

Defeating the Guardian adds 100 points. This value can be changed with `score victory_bonus orion = 100;`.


### Antares Win

![Antares victory screen](../../assets/images/manual/image46.png)

A win by defeating the Antaran Homeworld adds 250 points to the score. The value can be changed with `score victory_bonus antares = 250;`.


### Council Win

Council victory adds 100 points. The value can be changed with `score victory_bonus council = 100;`.

## Multiplayer scoring

Hotseat games calculate and display scores normally. Network multiplayer games do not enter the single-player end-game score and Hall of Fame path. Score-related configuration is synchronized by the host along with the other game rules.
