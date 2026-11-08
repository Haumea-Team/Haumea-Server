#!/bin/sh
# haumea-server starts, signs in nobody it shouldn't, and stops cleanly:
# an invite and the fingerprint from the commands, the server up with the same
# identity and its log.txt, a clean stop on SIGTERM. (The handshake itself is
# tested end to end by Haumea's own multiplayer tests, which need the editor.)
#   tests/smoke.sh build/haumea-server
set -eu
SERVER=$(cd "$(dirname "$1")" && pwd)/$(basename "$1")
WORK=$(mktemp -d)
trap 'kill "$PID" 2>/dev/null || true; rm -rf "$WORK"' EXIT
cd "$WORK"
"$SERVER" --help > /dev/null
"$SERVER" --state-dir state invite --role owner | grep -q '^Invite for an owner (1 use, 7 days): [A-Za-z0-9]\{6\}-[A-Za-z0-9]\{18\}$'
"$SERVER" --state-dir state users | grep -q '^No accounts yet'
FINGERPRINT=$("$SERVER" --state-dir state fingerprint)
echo "$FINGERPRINT" | grep -q '^[0-9a-f]\{4\}\(:[0-9a-f]\{4\}\)\{7\}$'
[ "$(stat -c %a state/identity.key)" = 600 ]
"$SERVER" --state-dir state --port 0 > out.txt 2>&1 &
PID=$!
for i in $(seq 1 50); do grep -q 'listening on port' out.txt && break; sleep 0.1; done
grep -q 'listening on port' out.txt
grep -q "fingerprint is $FINGERPRINT" state/log.txt
grep -q '\[Warning\] no accounts yet' state/log.txt
kill -TERM "$PID"
wait "$PID"
grep -q '\[Info\] stopped' state/log.txt
echo "smoke test: ok"
