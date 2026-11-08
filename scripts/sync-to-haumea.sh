#!/bin/sh
# Copies the multiplayer core into a Haumea checkout, which uses it for its own
# "Host this map" relay and to join servers. This repo owns the core; Haumea
# keeps a copy. Both ends check the protocol number when they connect, so a
# mismatch is refused rather than misread; still, release them together.
#
#   scripts/sync-to-haumea.sh /path/to/haumea
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
DEST=${1:?usage: $0 /path/to/haumea}/tools/haumea
[ -f "$DEST/multiplayer.cpp" ] || { echo "$DEST doesn't look like Haumea's tools/haumea" >&2; exit 1; }
for f in mpprotocol.h mprelay.h mprelay.cpp mpaccounts.h mpaccounts.cpp; do
	cp "$HERE/core/$f" "$DEST/$f"
done
cp "$HERE/core/monocypher/monocypher.c" "$HERE/core/monocypher/monocypher.h" "$DEST/monocypher/"
cp "$HERE/core/minilzo/minilzo.c" "$HERE/core/minilzo/minilzo.h" "$DEST/minilzo/"
echo "synced the multiplayer core ($(grep -o 'MP_PROTOCOL = [0-9]*' "$HERE/core/mpprotocol.h")) into $DEST"
