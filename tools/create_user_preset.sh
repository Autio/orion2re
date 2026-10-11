#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd -- "$SCRIPT_DIR/.." && pwd)
OUTPUT_PATH="$ROOT_DIR/cmake/local-user.cmake"

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

usage() {
    cat >&2 <<EOF
Usage: $(basename "$0") <game-path>

Creates $OUTPUT_PATH and sets ORION2RE_GAME_DIR to the provided game path.
The build keeps normal outputs in the build tree and copies the finished game binary there after build.
EOF
    exit 1
}

[[ $# -eq 1 ]] || usage

game_path="$1"
[[ -n "$game_path" ]] || usage

if command -v wslpath >/dev/null 2>&1 && [[ "$game_path" == /mnt/* ]]; then
    game_path=$(wslpath -m "$game_path")
fi

mkdir -p "$(dirname "$OUTPUT_PATH")"

quote='"'
escaped_game_path=${game_path//\/\\}
escaped_game_path=${escaped_game_path//$quote/\\$quote}

{
    printf '%s\n' '# Local user overrides for CMake. This file is gitignored.'
    printf '%s\n' "set(ORION2RE_GAME_DIR \"$escaped_game_path\" CACHE PATH \"Optional game directory used for post-build deployment and debugger working directory\" FORCE)"
} > "$OUTPUT_PATH"

echo "Wrote $OUTPUT_PATH"
echo "Configured ORION2RE_GAME_DIR=$game_path"