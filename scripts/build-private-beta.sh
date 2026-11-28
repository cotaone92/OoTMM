#!/usr/bin/env bash
set -euo pipefail

# Build a private OoTMM beta from user-supplied ROM paths.
# ROMs are copied only into the ignored/local roms/ workspace and removed on exit.
if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "Usage: $0 /path/to/oot.z64 /path/to/mm.z64 [seed]" >&2
  exit 2
fi

ROOT="$(git rev-parse --show-toplevel)"
OOT_INPUT="$(realpath "$1")"
MM_INPUT="$(realpath "$2")"
SEED="${3:-DEKU-BETA-001}"
OOT_LOCAL="$ROOT/roms/oot.z64"
MM_LOCAL="$ROOT/roms/mm.z64"

cleanup() {
  rm -f "$OOT_LOCAL" "$MM_LOCAL"
}
trap cleanup EXIT INT TERM

for file in "$OOT_INPUT" "$MM_INPUT"; do
  if [[ ! -f "$file" ]]; then
    echo "ROM file not found: $file" >&2
    exit 1
  fi
  if [[ "$(stat -c%s "$file")" -ne 33554432 ]]; then
    echo "Expected a 32 MiB big-endian .z64 ROM: $file" >&2
    exit 1
  fi
done

mkdir -p "$ROOT/roms"
cp "$OOT_INPUT" "$OOT_LOCAL"
cp "$MM_INPUT" "$MM_LOCAL"

cd "$ROOT"
node --input-type=module - "$OOT_LOCAL" "$MM_LOCAL" <<'NODE'
import fs from 'node:fs';
import CRC32 from 'crc-32';

const expected = new Map([
  ['oot', 0xcd16c529],
  ['mm', 0xb428d8a7],
]);
const files = [process.argv[2], process.argv[3]];
for (const [index, game] of ['oot', 'mm'].entries()) {
  const data = fs.readFileSync(files[index]);
  const actual = CRC32.buf(data) >>> 0;
  const wanted = expected.get(game);
  if (actual !== wanted) {
    console.error(`${game.toUpperCase()} ROM mismatch: expected 0x${wanted.toString(16)}, got 0x${actual.toString(16)}`);
    process.exit(1);
  }
  console.log(`${game.toUpperCase()} ROM accepted (CRC32 0x${actual.toString(16)})`);
}
NODE

echo "Building native assets with the supplied ROMs..."
pnpm nx run @ootmm/generator:build

echo "Generating private beta ROM (seed: $SEED)..."
pnpm --filter @ootmm/generator start --seed "$SEED"

echo "Generation finished. Check $ROOT/out for the .z64 output."
echo "The temporary ROM copies in roms/ will now be removed."
