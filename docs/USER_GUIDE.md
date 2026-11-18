English | [Português (Brasil)](USER_GUIDE.pt-BR.md)

# haumea-server user guide

haumea-server keeps a [Haumea Studio](https://github.com/Haumea-Team/Haumea-Studio) multiplayer
session running on a machine that is always on, so nobody has to keep Haumea open to host. People
join it from Haumea and build maps together. It needs no game and no screen.

- [Download](#download)
- [Your first run](#your-first-run)
- [Joining from Haumea](#joining-from-haumea)
- [Accounts, invites and roles](#accounts-invites-and-roles)
- [Rooms](#rooms)
- [Getting the map to your game server](#getting-the-map-to-your-game-server)
- [Settings](#settings)
- [Running it as a service on Linux](#running-it-as-a-service-on-linux)
- [Windows](#windows)
- [Updating](#updating)
- [Backups](#backups)
- [The log](#the-log)
- [Troubleshooting](#troubleshooting)

## Download

Get the latest release from
[github.com/Haumea-Team/Haumea-Server/releases](https://github.com/Haumea-Team/Haumea-Server/releases):

| File | |
|---|---|
| `haumea-server-linux-x86_64` | the server for any 64-bit Linux (static: nothing else to install) |
| `haumea-server-windows-x86_64.exe` | the server for 64-bit Windows |
| `install.sh`, `haumea-session.service`, `env.example` | to run it as a service on Linux |
| `SHA256SUMS` | checksums: `sha256sum -c SHA256SUMS --ignore-missing` |

The server and Haumea must speak the same multiplayer protocol. When they don't, Haumea says which
side is out of date; update that one.

## Your first run

On Linux:

```sh
mv haumea-server-linux-x86_64 haumea-server
chmod +x haumea-server
./haumea-server --state-dir state invite --role owner   # prints your owner's invite code
./haumea-server --state-dir state                        # runs the server
```

Everything it keeps (accounts, invites, rooms, its identity key and the log) goes in the `state`
folder. Keep that folder: it is your server.

The server listens on TCP port **7790**. Open it in your firewall (and in your host's firewall
panel, if it has one) so people can reach it.

For a server that starts on boot and restarts by itself, see
[Running it as a service on Linux](#running-it-as-a-service-on-linux).

## Joining from Haumea

In Haumea, open **File → Multiplayer...** and choose **Join**:

1. Type the server's address and port (7790).
2. The first time, choose **Invite code**, paste the code, and pick a name and a password. That
   makes your account; the code is used up.
3. From then on, choose **Account** and sign in with your name and password.
4. Leave **Room** empty to see the list of rooms, or type one. As the owner you can type a new name
   (`main`, say): the room is made and the map you have open becomes it.

The first time you join, Haumea shows the server's **fingerprint** and remembers it. Check that it
matches what the server says (`haumea-server --state-dir state fingerprint`). If another server ever
answers at that address, Haumea refuses to join: something would be in the middle.

## Accounts, invites and roles

People get in with an invite. Make one on the server:

```sh
./haumea-server --state-dir state invite                         # a builder, one use, 7 days
./haumea-server --state-dir state invite --role viewer --uses 5 --days 30
```

or in Haumea, as an admin: **Admin → Make an invite code** in the Multiplayer panel.

| Role | Can |
|---|---|
| **viewer** | look around and chat; their edits come back undone |
| **builder** | edit the map |
| **admin** | also invite people, set roles, remove people, make, lock and delete rooms |
| **owner** | everything, including making other owners |

A role applies to the whole server, or to one room only. Admins change roles and remove people from
Haumea (right-click someone under **People**) or on the server:

| Command | |
|---|---|
| `users` | the accounts and their roles |
| `role NAME R` | give an account a role everywhere (`viewer`, `builder`, `admin`, `owner`) |
| `passwd NAME` | set an account's password (someone forgot theirs) |
| `remove NAME` | delete an account |
| `fingerprint` | this server's identity, as Haumea shows it |

Run them as `haumea-server --state-dir state COMMAND`. They work while the server runs: it takes the
change in at once.

Five wrong passwords from one address make that address wait ten minutes.

Anyone in a session can send 0.3.DL custom models, which everyone's Haumea loads. Invite people you
trust, and give the builder role with care.

## Rooms

One server holds several maps, each in its own room, with its own people, chat and saves. Admins:

- **make** a room by joining with a new room name: the map they have open becomes it;
- **lock** a room with a password, which everyone else then needs to enter it;
- **delete** a room;
- decide what AI agents (Haumea's Namaka) may do in each room.

People move between rooms, or back to the lobby, from the Multiplayer panel without reconnecting.

Every room is written to disk as it changes, so a crash or a restart loses nothing.

## Getting the map to your game server

With `--maps-dir`, the server keeps each room's map as Pawn:

```sh
./haumea-server --state-dir state --maps-dir maps \
    --map-out /home/samp/server/gamemodes/map.pwn --map-out-room main
```

- Whenever someone saves in Haumea, the server writes the map to `maps/<room>/<name>.pwn`, a dated
  copy to `maps/<room>/history`, and says so in the chat.
- Every 10 minutes the map changed, it asks an editor in the room for it, so the files stay current
  even when nobody saves (`--autosave MIN` changes that; `--autosave 0` turns it off).
- `--map-out FILE` also writes one room's map (`--map-out-room`, `main` by default) straight into
  your game server's folder. Run haumea-server on the same machine as your open.mp or SA-MP server
  and the map is there when you restart your gamemode.

## Settings

Settings come from the built-in defaults, then a `.env` file (the one in the working folder, or
`--env FILE`), then `HAUMEA_SERVER_*` environment variables, then the command line, each overriding
the one before. `env.example` lists them all, and `haumea-server --help` too.

| Option | Variable | |
|---|---|---|
| `--state-dir DIR` | `HAUMEA_SERVER_STATE_DIR` | accounts, invites, roles and rooms (rooms in `DIR/rooms`) |
| `--port N` | `HAUMEA_SERVER_PORT` | TCP port (default 7790) |
| `--bind ADDRESS` | `HAUMEA_SERVER_BIND` | listen on one address only, IPv4 or IPv6 (default: all) |
| `--name TEXT` | `HAUMEA_SERVER_NAME` | the server's name in Haumea |
| `--max-clients N` | `HAUMEA_SERVER_MAX_CLIENTS` | editors at once (default 32) |
| `--maps-dir DIR` | `HAUMEA_SERVER_MAPS_DIR` | keep each room's map as Pawn |
| `--map-out FILE` | `HAUMEA_SERVER_MAP_OUT` | also write one room's map to FILE |
| `--map-out-room R` | `HAUMEA_SERVER_MAP_OUT_ROOM` | the room `--map-out` is for (default `main`) |
| `--autosave MIN` | `HAUMEA_SERVER_AUTOSAVE` | ask for the map every MIN minutes it changed (default 10; 0: off) |
| `--log-file FILE` | `HAUMEA_SERVER_LOG_FILE` | the log (default `DIR/log.txt`; empty: none) |
| `--no-log-chat` | `HAUMEA_SERVER_LOG_CHAT=0` | leave the chat out of the log |
| `--no-log-connections` | `HAUMEA_SERVER_LOG_CONNECTIONS=0` | leave who comes and goes out of the log |
| `--timestamp-format F` | `HAUMEA_SERVER_TIMESTAMP_FORMAT` | strftime format of the log's timestamps |
| `--no-update-check` | `HAUMEA_SERVER_UPDATE_CHECK=0` | don't check GitHub for a newer release |

### One session, no accounts

Without `--state-dir`, the server runs one session behind one password, the way **Host this map**
does in Haumea:

```sh
./haumea-server --password secret --state crew.hmps
```

Without `--password` it makes a random one and prints it. `--state FILE` keeps the session across
restarts. `--admin-password TEXT` lets only who gives it load another map for everyone;
`--no-replace` lets nobody.

## Running it as a service on Linux

`install.sh` sets haumea-server up as a systemd service. Put the release's files in one folder and
run:

```sh
mv haumea-server-linux-x86_64 haumea-server
chmod +x haumea-server
sudo sh install.sh
```

It:

- makes a `haumea-session` system user, and keeps everything in `/var/lib/haumea-session`;
- installs the program as `/usr/local/bin/haumea-session` and the service as `haumea-session`;
- reads settings from `/etc/haumea-session/env` (see `env.example`); settings you already have stay;
- opens TCP 7790 in ufw when ufw is active (otherwise, open it in your provider's firewall);
- the first time, prints your owner's invite and the server's fingerprint.

Then:

```sh
sudo systemctl status haumea-session
sudo systemctl restart haumea-session      # after changing /etc/haumea-session/env
sudo journalctl -u haumea-session -f
```

Commands on the service's state run as its user:

```sh
sudo runuser -u haumea-session -- haumea-session --state-dir /var/lib/haumea-session invite
```

## Windows

`haumea-server-windows-x86_64.exe` takes the same options and commands. In a terminal (PowerShell or
cmd), in the folder you downloaded it to:

```bat
haumea-server-windows-x86_64.exe --state-dir state invite --role owner
haumea-server-windows-x86_64.exe --state-dir state
```

Allow it through Windows Firewall when Windows asks, and forward TCP 7790 on your router if people
join over the internet. It runs while that window is open; to keep it up on its own, run it with
Task Scheduler at startup or as a service with a tool such as NSSM.

## Updating

Once a day the server asks GitHub's API for the latest release and logs a warning when there's a
newer one (`--no-update-check` turns that off). Haumea also tells people trying to join when the
server is the side that's out of date.

To update, stop the server, replace the program and start it again. Your state folder stays as it
is. With the service, run `install.sh` again with the new release: it only updates the program.

## Backups

Back up the state folder (`/var/lib/haumea-session` for the service), and the maps folder if it's
elsewhere. It holds:

| | |
|---|---|
| `accounts.txt`, `invites.txt` | accounts (no passwords, only Argon2id-derived keys) and open invites |
| `identity.key` | the server's identity: lose it and everyone's Haumea sees a different server |
| `secret.key` | keeps it from telling which account names exist |
| `rooms/` | each room's map, history and settings (lock, roles, what agents may do) |
| `log.txt` | the log |

Keep the keys private: anyone with `identity.key` can pretend to be your server.

## The log

`log.txt` is written as open.mp's: `[2026-11-08T01:30:12+0000] [Info] ...`, with `[Warning]` and
`[Error]` lines too. It has who signs in and from where, the chat, and every admin action (invites,
roles, removals, locks).

## Troubleshooting

| Problem | What to do |
|---|---|
| Haumea can't connect | Check the server runs, the address and port are right, and TCP 7790 is open in every firewall on the way (the machine's, your host's, your router's). |
| Haumea says the server is out of date | Update haumea-server. |
| Haumea says it is out of date | Update Haumea (it updates itself, or download it). |
| Haumea refuses to join over the fingerprint | Another server answers at that address. If you moved or reinstalled the server without its `identity.key`, that's expected: forget the saved server in Haumea and join again. Otherwise, don't. |
| Wrong password, then a wait | Five wrong passwords make the address wait ten minutes. An admin can set a new password with `passwd NAME`. |
| A viewer's edits disappear | That's the viewer role. Give them `builder`. |
| The map file doesn't update | It updates on save and every `--autosave` minutes the map changed, and only with `--maps-dir`. Check the log. |

Still stuck? Ask in the [Haumea Discord](https://discord.gg/YSxtANrsB4) or open an issue.
