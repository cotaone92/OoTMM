#!/usr/bin/env bash
set -euo pipefail

# Build a private OoTMM beta from user-supplied ROM paths.
# ROMs are copied only into the local roms/ workspace and removed on exit.
if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "Usage: $0 /path/to/oot.z64 /path/to/mm.z64 [seed]" >&2
  exit 2
fi

ROOT="$(git rev-parse --show-toplevel)"
OOT_INPUT="$(realpath "$1")"
MM_INPUT="$(realpath "$2")"
SEED="DEKU-BETA-001"
if [[ $# -eq 3 ]]; then SEED="$3"; fi
OOT_LOCAL="$ROOT/roms/oot.z64"
MM_LOCAL="$ROOT/roms/mm.z64"

for file in "$OOT_INPUT" "$MM_INPUT"; do
  if [[ ! -f "$file" ]]; then
    echo "ROM file not found: $file" >&2
    exit 1
  fi
  if [[ "$(stat -c%s "$file")" -ne 33554432 ]]; then
    echo "Expected a 32 MiB .z64 ROM: $file" >&2
    exit 1
  fi
done

# Verify the exact supplied ROM revisions before copying them into the build workspace.
OOT_SHA="$(sha256sum "$OOT_INPUT" | cut -d' ' -f1)"
MM_SHA="$(sha256sum "$MM_INPUT" | cut -d' ' -f1)"
if [[ "$OOT_SHA" != "c916ab315fbe82a22169bff13d6b866e9fddc907461eb6b0a227b82acdf5b506" ]]; then
  echo "OoT ROM SHA-256 mismatch: $OOT_SHA" >&2
  exit 1
fi
if [[ "$MM_SHA" != "efb1365b3ae362604514c0c1a2d11f5dc8688ba5be660a37debf5e3be43f2b" ]]; then
  echo "Majora's Mask ROM SHA-256 mismatch: $MM_SHA" >&2
  exit 1
fi

mkdir -p "$ROOT/roms"
if [[ -e "$OOT_LOCAL" || -e "$MM_LOCAL" ]]; then
  echo "Refusing to overwrite existing files in $ROOT/roms. Move them aside first." >&2
  exit 1
fi

OOT_COPIED=false
MM_COPIED=false
cleanup() {
  if [[ "$OOT_COPIED" == true ]]; then rm -f "$OOT_LOCAL"; fi
  if [[ "$MM_COPIED" == true ]]; then rm -f "$MM_LOCAL"; fi
}
trap cleanup EXIT INT TERM

OOT_COPIED=true
cp "$OOT_INPUT" "$OOT_LOCAL"
MM_COPIED=true
cp "$MM_INPUT" "$MM_LOCAL"

cd "$ROOT"
pnpm --filter @ootmm/generator exec node --input-type=module - "$OOT_LOCAL" "$MM_LOCAL" <<'NODE'
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
    console.error(game.toUpperCase() + ' ROM mismatch: expected 0x' + wanted.toString(16) + ', got 0x' + actual.toString(16));
    process.exit(1);
  }
  console.log(game.toUpperCase() + ' ROM accepted (CRC32 0x' + actual.toString(16) + ')');
}
NODE

echo "Building native assets with the supplied ROMs..."
pnpm nx run @ootmm/generator:build

echo "Generating private beta ROM (seed: $SEED)..."
pnpm --filter @ootmm/generator exec tsx ./lib/cli.ts --seed "$SEED"

echo "Generation finished. Check $ROOT/out for the .z64 output."
echo "The temporary ROM copies in roms/ will now be removed."
