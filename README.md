# haumea-server

The multiplayer server for [Haumea Studio](https://github.com/itsneufox/haumea), the map editor
for open.mp and SA-MP. People join it from Haumea (**File → Multiplayer → Join**) and build one
map together: they see each other's edits as they happen, each other's cameras and selections, and
talk in a chat. AI agents (Haumea's Namaka) can join too, tagged as agents and as whose they are.

It is one small program with no dependencies: run it on the machine your game server runs on.

## What it does

- **Accounts and invites.** You make an invite; whoever gets it picks a name and a password in
  Haumea, and from then on signs in with those. Invites are used up and expire.
- **Roles.** Owner, admin, builder, viewer, on the whole server or per room. The server enforces
  them: a viewer's edits come back undone, and admin actions are refused to everyone else.
- **Rooms.** One server holds several maps, each with its own people, chat and saves. Admins make,
  lock (with a room password) and delete rooms, and decide what AI agents may do in each.
- **It keeps everything.** Each room is written to disk as it changes, so a crash or restart loses
  nothing. With `--maps-dir` it also keeps the map as Pawn whenever someone saves it (and a dated
  copy), autosaves it while it changes, and `--map-out` can write it into your gamemodes folder.
- **Encrypted, and it proves who it is.** Every connection is encrypted (X25519, XChaCha20-Poly1305
  from Monocypher), keyed to the password through Argon2id, and the server signs each handshake with
  its own Ed25519 identity. Haumea remembers a server's fingerprint and refuses to join if another
  answers at that address later. A name nobody has is refused the same way a wrong password is, and
  five wrong passwords from one address make it wait ten minutes.
- **Logs as open.mp does.** `log.txt` with `[2026-11-08T01:30:12+0000] [Info] ...` lines: who
  signs in and from where, the chat, and every admin action (invites, roles, removals, locks).
- **Ping.** Everyone sees everyone's round trip to the server.
- **Tells you when it's behind.** Haumea updates itself; when an editor newer than the server tries
  to join, it's told the server needs updating, and the log says so. Once a day the server also asks
  GitHub's API (only that, with curl) for the latest release and logs a warning when there's a newer
  one; `--no-update-check` (or `HAUMEA_SERVER_UPDATE_CHECK=0`) turns that off. `--version` says which
  this is.

## Build

```sh
make            # build/haumea-server, static, for any x86-64 Linux
make windows    # build/haumea-server.exe (MinGW-w64)
make check
```

## Run

```sh
haumea-server --state-dir ./state                      # accounts and rooms, kept in ./state
haumea-server --state-dir ./state invite --role owner  # your first invite
```

Join with that invite in Haumea, pick your name and password, and type a room name (`main`, say):
as the owner you make it, and your open map becomes it. Invite others from Haumea's Multiplayer panel
(**Admin → Make an invite code**) or with `haumea-server invite`.

### Commands

| Command | |
|---|---|
| `invite [--role R] [--uses N] [--days N]` | a code to make an account with (builder, one use, 7 days by default) |
| `users` | the accounts and their roles |
| `role NAME R` | give an account a role everywhere (`viewer`, `builder`, `admin`, `owner`) |
| `passwd NAME` | set an account's password |
| `remove NAME` | delete an account |
| `fingerprint` | this server's identity, as Haumea shows it |

They work while the server runs: it takes the change in at once.

### Settings

From the built-in defaults, then a `.env` file (the one in the working folder, or `--env FILE`), then
`HAUMEA_SERVER_*` variables, then the command line. `deploy/env.example` lists them all;
`haumea-server --help` too.

Without `--state-dir` it runs one session behind one password (`--password`, `--state`), as an
editor hosting from Haumea does.

## Deploy (systemd)

Copy `build/haumea-server` and the `deploy/` folder to the server, then:

```sh
sudo ./deploy/install.sh
```

It makes a `haumea-session` user and service, keeps everything in `/var/lib/haumea-session`, reads
`/etc/haumea-session/env`, opens TCP 7790 in ufw, and prints your owner's invite and the server's
fingerprint. Running it again updates the program. A one-session server from before becomes the room
`main`.

## Haumea and this repo

This repo owns the multiplayer core in `core/` (the protocol, the relay, accounts). Haumea keeps a
copy for its own **Host this map** and for joining servers: `scripts/sync-to-haumea.sh
/path/to/haumea` updates it. Both ends check the protocol number (`MP_PROTOCOL` in
`core/mpprotocol.h`, now 5) when they connect, so mismatched versions are refused, not misread;
release them together.

## Licence

GPL-2.0-or-later, as Haumea. Monocypher (BSD-2-Clause or CC0) and miniLZO (GPL-2.0-or-later) are in
`core/`, with their licences.
