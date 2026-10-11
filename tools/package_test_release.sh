#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(cd -- "$SCRIPT_DIR/.." && pwd)
RELEASES_DIR="$ROOT_DIR/test-releases"
BASE_DATA_DIR="$ROOT_DIR/data/base"
DEFAULT_CONFIG="$ROOT_DIR/data/orion2re.cfg"
SUPPORTED_MODS_DIR="$ROOT_DIR/data/mods"

fail() {
    echo "ERROR: $*" >&2
    exit 1
}

require_file() {
    local path="$1"
    [[ -f "$path" ]] || fail "Required file not found: $path"
}

require_dir() {
    local path="$1"
    [[ -d "$path" ]] || fail "Required directory not found: $path"
}

require_dir "$BASE_DATA_DIR"
require_file "$DEFAULT_CONFIG"
require_dir "$SUPPORTED_MODS_DIR"

latest_recipe=$(find "$ROOT_DIR/orion2re" -type f -name 'orion2re.exe.recipe' -printf '%T@ %p\n' | sort -n | tail -n 1 | cut -d' ' -f2-)
[[ -n "$latest_recipe" ]] || fail "Could not find any orion2re.exe.recipe files under $ROOT_DIR/orion2re"
require_file "$latest_recipe"

exe_win_path=$(sed -n 's:.*<FullPath>\(.*\)</FullPath>.*:\1:p' "$latest_recipe" | tr -d '\r' | sed '1s/^﻿//')
[[ -n "$exe_win_path" ]] || fail "Could not extract output executable path from $latest_recipe"

if command -v wslpath >/dev/null 2>&1; then
    exe_path=$(wslpath -u "$exe_win_path")
else
    fail "wslpath is required to convert Windows build output paths"
fi

require_file "$exe_path"

pdb_path="${exe_path%.exe}.pdb"
if [[ ! -f "$pdb_path" ]]; then
    pdb_path=$(find "$(dirname "$exe_path")" -maxdepth 1 -type f -name '*.pdb' -printf '%T@ %p\n' | sort -n | tail -n 1 | cut -d' ' -f2-)
fi
[[ -n "${pdb_path:-}" ]] || fail "Could not find a matching PDB next to $exe_path"
require_file "$pdb_path"

asan_dll_path=$(find "/mnt/c/Program Files/Microsoft Visual Studio" -type f -name 'clang_rt.asan_dynamic-i386.dll' | sort | tail -n 1)
[[ -n "$asan_dll_path" ]] || fail "Could not find clang_rt.asan_dynamic-i386.dll under Visual Studio installation"
require_file "$asan_dll_path"

stamp=$(date +%Y%m%d_%H%M%S)
rand_id=$(printf '%06x' "$((RANDOM << 16 | RANDOM))")
release_dir="$RELEASES_DIR/${stamp}_${rand_id}"
tester_dir="$release_dir/tester-package"
release_name=$(basename "$release_dir")
zip_path="$release_dir/${release_name}.zip"

mkdir -p "$release_dir" "$tester_dir/data/mods"

cp "$exe_path" "$tester_dir/"
cp "$pdb_path" "$tester_dir/"
cp "$asan_dll_path" "$tester_dir/"
cp "$DEFAULT_CONFIG" "$tester_dir/data/orion2re.cfg"

cat > "$tester_dir/README.txt" <<EOF
Run orion2re.exe directly. ASan logging is enabled automatically in ASan builds.

If the game crashes, send back the newest files from:
- logs
- dumps
EOF

python3 - <<PY
from pathlib import Path
import zipfile

tester_dir = Path(r'''$tester_dir''')
zip_path = Path(r'''$zip_path''')
base_data_dir = Path(r'''$BASE_DATA_DIR''')
supported_mods_dir = Path(r'''$SUPPORTED_MODS_DIR''')

def write_directory_archive(source_dir, archive_path):
    with zipfile.ZipFile(
            archive_path, 'w', compression=zipfile.ZIP_DEFLATED) as zf:
        for path in sorted(source_dir.rglob('*')):
            if path.is_file():
                zf.write(path, path.relative_to(source_dir))

write_directory_archive(base_data_dir, tester_dir / 'data' / 'base.zip')
for mod_dir in sorted(supported_mods_dir.iterdir()):
    if mod_dir.is_dir():
        write_directory_archive(
            mod_dir, tester_dir / 'data' / 'mods' / f'{mod_dir.name}.zip')

with zipfile.ZipFile(zip_path, 'w', compression=zipfile.ZIP_DEFLATED) as zf:
    for path in sorted(tester_dir.rglob('*')):
        if path.is_file():
            zf.write(path, path.relative_to(tester_dir))
PY

echo "Created test release: $release_dir"
echo "Tester package contents:"
echo "  tester-package/$(basename "$exe_path")"
echo "  tester-package/$(basename "$pdb_path")"
echo "  tester-package/$(basename "$asan_dll_path")"
echo "  tester-package/README.txt"
echo "  tester-package/data/base.zip"
echo "  tester-package/data/orion2re.cfg"
for mod_dir in "$SUPPORTED_MODS_DIR"/*; do
    if [[ -d "$mod_dir" ]]; then
        echo "  tester-package/data/mods/$(basename "$mod_dir").zip"
    fi
done
echo "Zip archive:"
echo "  ${release_name}.zip"