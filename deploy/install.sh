#!/bin/sh
# Installs or updates haumea-server as haumea-session.service. Run with sudo,
# from a folder holding haumea-server (the static Linux build) and this deploy/
# folder's files.
#
# - A server user, haumea-session, owns /var/lib/haumea-session: accounts,
#   rooms, maps and log.txt.
# - Settings are /etc/haumea-session/env (see env.example); the service reads it.
# - A one-session server from before (crew.hmps) becomes the room "main".
# - With no accounts yet, it prints an owner's invite: join with it in Haumea
#   (File > Multiplayer > Join, Invite code) to make your account.
# Running it again only updates the program.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
STATE=/var/lib/haumea-session
ENVFILE=/etc/haumea-session/env
[ "$(id -u)" = 0 ] || { echo "run it with sudo"; exit 1; }
BINARY="$HERE/haumea-server"
[ -x "$BINARY" ] || BINARY="$HERE/../build/haumea-server"
[ -x "$BINARY" ] || { echo "no haumea-server next to this script (or in ../build)"; exit 1; }

id haumea-session >/dev/null 2>&1 || useradd --system --no-create-home --shell /usr/sbin/nologin haumea-session
systemctl stop haumea-session 2>/dev/null || true
install -m 755 "$BINARY" /usr/local/bin/haumea-session
install -m 644 "$HERE/haumea-session.service" /etc/systemd/system/haumea-session.service
install -d -m 750 -o root -g haumea-session /etc/haumea-session
install -d -m 700 -o haumea-session -g haumea-session "$STATE" "$STATE/rooms"

# Settings: what an older install had stays; what's new is added.
touch "$ENVFILE"
add() { grep -q "^$1=" "$ENVFILE" || printf '%s=%s\n' "$1" "$2" >> "$ENVFILE"; }
add HAUMEA_SERVER_STATE_DIR "$STATE"
add HAUMEA_SERVER_PORT 7790
add HAUMEA_SERVER_NAME Crew
add HAUMEA_SERVER_MAPS_DIR "$STATE/maps"
add HAUMEA_SERVER_AUTOSAVE 10
chgrp haumea-session "$ENVFILE"
chmod 640 "$ENVFILE"

# One session from before: room "main", its maps with it.
if [ -f "$STATE/crew.hmps" ] && [ ! -f "$STATE/rooms/main.hmps" ]; then
	mv "$STATE/crew.hmps" "$STATE/rooms/main.hmps"
	[ -f "$STATE/crew.hmps.log" ] && mv "$STATE/crew.hmps.log" "$STATE/rooms/main.hmps.log"
	if [ -d "$STATE/maps" ] && [ ! -d "$STATE/maps/main" ]; then
		mkdir -p "$STATE/maps/main"
		find "$STATE/maps" -maxdepth 1 -mindepth 1 ! -name main -exec mv {} "$STATE/maps/main/" \;
	fi
	echo "Your session from before is now the room \"main\"."
fi
chown -R haumea-session:haumea-session "$STATE"

as_server() { runuser -u haumea-session -- /usr/local/bin/haumea-session --state-dir "$STATE" "$@"; }
if as_server users | grep -q '^No accounts'; then
	echo
	echo "No accounts yet. Your owner's invite (one use, 7 days):"
	as_server invite --role owner | head -1
	echo "In Haumea: File > Multiplayer > Join > Invite code, with this server's address and port."
fi
echo
echo "Server fingerprint (Haumea shows the same when you join): $(as_server fingerprint)"

systemctl daemon-reload
systemctl enable haumea-session >/dev/null 2>&1
systemctl restart haumea-session
if ufw status 2>/dev/null | grep -q "Status: active"; then
	ufw allow 7790/tcp comment 'Haumea multiplayer' >/dev/null && echo "ufw: 7790/tcp is open"
else
	echo "ufw isn't active: open TCP 7790 in your provider's firewall if it has one"
fi
sleep 1
systemctl --no-pager --lines=0 status haumea-session | sed -n '1,3p'
echo "Log: $STATE/log.txt (or journalctl -u haumea-session)"
