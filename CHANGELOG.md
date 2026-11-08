# Changelog

## Unreleased: multiplayer protocol 5

- Its own repository, moved out of Haumea's `tools/haumea-server`, with the multiplayer core in `core/`.
- Accounts, invites and roles (owner, admin, builder, viewer), per server and per room; viewers'
  edits are undone on their screen.
- Rooms: one server, many maps; admins make, lock and delete them and set what agents may do there.
- The server's Ed25519 identity signs every handshake; editors pin its fingerprint.
- Agent tokens: an account's AI agents join as its own, tagged "via" that account.
- Ping, measured by every editor and shown to everyone.
- Logging as open.mp's (log.txt, timestamps, [Info]/[Warning]/[Error]), with chat, connections and
  every admin action; settings from .env and HAUMEA_SERVER_* variables.
- Commands: invite, users, role, passwd, remove, fingerprint.
- CI: static Linux and Windows builds, a smoke test, and a release with both builds for each v* tag.
- Says which side is out of date when versions differ, logs it, and checks GitHub for a newer release once a day.
