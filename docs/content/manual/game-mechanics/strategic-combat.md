# Manual: Strategic combat

Strategic combat uses the strategic damage values from the weapons data for both automatic weapon selection and combat resolution. Automatic designs prefer the available weapon with the greatest maximum strategic damage; ties favor the lower weapon-table ID.

This is especially important for special weapons. The default strategic values include working damage for Bombers and Heavy Fighters, and distinct values for weapons such as Pulsar, Plasma Web, Spatial Compressor, Black Hole Generator, and Stellar Converter. A mod can replace these values through the weapon data.

## Crew bonuses

`combat strategic_xp_bonus` controls whether strategic combat includes the crew-level attack and defense bonuses in beam attack, beam defense, and missile defense. When enabled, crew training, Space Academy, and Warlord affect those values. Racial beam defense also applies, while Trans-Dimensional combat speed does not provide a separate bonus. Leader Weaponry and Helmsman skills contribute their attack and beam-defense bonuses independently of the crew-level option.

## Planetary defenses

Pre-combat damage from Artemis System Net is resolved before the surviving forces determine the battle winner. Destroying an attacker does not grant the defender victory if the attacking fleet subsequently destroys all defending forces.

Technologies with no strategic-combat function are normally excluded from the strategic technology tree. Strategic games include Warp Interdictor, while Anti-Missile Rockets, Energy Absorber, Security Stations, and Warp Dissipator are omitted unless a mod changes the tree.