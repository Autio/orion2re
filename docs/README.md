# Documentation portal

The portal is opt-in because configuration reference generation reads native game defaults.
Its only documentation-specific system requirement is Python 3.10 or newer with the standard `venv` and `ensurepip` modules. CMake creates `<project>/.venv` and installs the committed lock file with that environment's `python -m pip`; `uv`, Poetry, and other environment managers are not required. The environment is reused across CMake cache and build-directory recreation.

Configure and build on Linux with:

```sh
cmake --preset docs
cmake --build --preset docs
```

The build preset runs the strict documentation check. To serve the portal locally, run:

```sh
cmake --build out/build/Linux/docs --target docs-serve
```

Generated Markdown and the rendered site remain under the CMake build tree. The reusable Python environment and its lock-file stamp remain in the ignored `<project>/.venv` directory, so recreating the CMake build tree does not reinstall unchanged dependencies. Do not edit generated configuration pages. The `docs-clean` target removes generated Markdown and rendered output while retaining `.venv`.

The historical 1.50.26 manual is committed as categorized Markdown with local images and source provenance. Portal builds do not contact the original manual website.