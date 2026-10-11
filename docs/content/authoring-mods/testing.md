# Authoring Mods: Testing and Troubleshooting

Test a mod as a package, not only as a set of loose files. Configuration priority, VFS priority, source identity, saves, and multiplayer can expose problems that do not appear when one file is tested in isolation.

## Development cycle

1. Use the directory form `data/mods/<id>/`.
2. Begin with a valid manifest and empty CFG entry point.
3. Enable only the package and confirm startup.
4. Add one configuration, asset, or Lua feature at a time.
5. Test with its required dependencies.
6. Test with packages it is intended to override or compose with.
7. Create a new game for setup-sensitive changes.
8. Save, exit, restart, and load.
9. Test every supported old package/storage version.
10. Build and test the release ZIP separately from the development directory.

## Startup failures

Check these package errors first:

- the package is installed but has no `enable` statement;
- directory, ZIP, manifest, dependency, and `enable` IDs differ;
- both directory and ZIP forms exist;
- `mod.json5` is nested inside another directory in the ZIP;
- the manifest has an unknown property, unsupported version syntax, or incompatible engine range;
- the required `config` path is absent or names a missing or invalid file;
- a dependency is absent, cyclic, or version-incompatible;
- an active package is listed as a conflict;
- a `load_after` assertion disagrees with enable priority; or
- an archive is malformed, encrypted, unsafe, or contains duplicate normalized paths.

Activation is staged. A manifest, dependency, CFG, VFS, Lua setup, prototype, registration, migration, or load-lifecycle error rejects the candidate package set instead of intentionally committing a partial activation.

## Configuration failures

For a CFG diagnostic:

1. Check the reported file, line, and column.
2. Confirm the statement ends with `;`.
3. Compare the complete path with the matching generated reference.
4. Check index base and symbolic names.
5. Replace a compact record assignment with explicit fields.
6. Check that a scalar-start assignment stays inside its record.
7. Check included files and the include stack.
8. Reduce the entry point until the first failing statement is isolated.

A statement either validates completely or applies none of its values. A complete startup or reload failure leaves the previously active configuration and package layers in place where the lifecycle supports retaining them.

## Asset failures

When an asset does not appear:

- confirm it is below package `data/`, not package root;
- remove a hiding file from `data/overrides/`;
- check package priority;
- verify the virtual path and case-insensitive spelling;
- check for duplicate normalized ZIP entries;
- for an LBX entry, use a three-digit entry name and payload-only content; and
- test the final ZIP after removing the directory package.

When an asset fails only when first used, test every changed screen, sound, animation, and video rather than assuming successful startup validated the payload format.

## Lua activation failures

Common causes include:

- `main.lua` without a valid manifest `lua` object;
- `lua` metadata without package-root `main.lua`;
- unsupported listener, callback, service, or script-path manifest properties;
- unknown event, rule, or service names;
- registration attempted after setup sealed;
- a module name containing uppercase letters, separators, or traversal;
- a module cycle or module loaded for the first time after sealing;
- floating-point source, `/`, `^`, or bytecode;
- prototype validation failure;
- duplicate exclusive service providers; or
- memory, instruction, source, stack, recursion, registration, or result limits.

Use the package ID, callback name, source line, traceback, and API operation from the diagnostic. Do not catch an error and continue with guessed state after a deterministic callback fails.

## Lua behavior tests

For each handler, rule, or service, test:

- no-op and normal branches;
- minimum and maximum accepted integer values;
- absent optional objects;
- expired references;
- deterministic collection order;
- interaction with an earlier and later callback;
- callback error and budget failure;
- save/load without rerunning new-game behavior;
- storage serialization limits and unsupported types; and
- whether the hook is accepted or rejected in multiplayer.

For RNG behavior, compare repeatable runs from the same authoritative starting state. Adding or reordering a callback can legitimately change later native draws because Lua uses the shared engine stream.

## Save and migration tests

Keep test saves for every package version that `compatible_versions` claims to support. Verify:

1. Exact-version loading.
2. Every declared migration edge.
3. A multi-edge migration to the current storage version.
4. Shared and cyclic table preservation when used.
5. Valid and expired stored references.
6. Rebuilding transient globals in `game.loaded`.
7. Rejection of a missing edge or incompatible package version.
8. Rejection when source changed without a compatible release identity.

Do not publish `compatible_versions` based only on successful startup. Continue the loaded game far enough to exercise the migrated data.

## Multiplayer tests

Use the exact same engine build, ordered packages, package versions, Lua sources, and synchronized configuration on every peer. Test:

- new-game setup and loaded-game setup;
- a deliberate package-order mismatch;
- a deliberate source mismatch with the same visible version;
- disconnect or setup rejection before play;
- host execution of `population.growth` or `galaxy_generator` when used; and
- rejection of `ship.destroyed`, `combat.beam_damage`, or any other network-disabled hook.

A successful single-player test does not establish multiplayer support. The hook must appear in the current network allowlist and all of its permitted state must follow an authoritative transfer path.

## Release ZIP verification

Open the archive and verify this shape directly:

```text
mod.json5
config/main.cfg
main.lua                 Optional
content/                 Optional
migrations/              Optional
data/                    Optional
```

Then install only `data/mods/<id>.zip` in a clean game data directory, enable it, and repeat startup, new-game, save/load, and applicable multiplayer tests. A package that works only while its development directory or `data/overrides/` exists is incomplete.