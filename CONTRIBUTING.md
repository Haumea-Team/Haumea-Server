# Contributing to haumea-server

Thanks for helping. Bug reports, ideas, translations of the docs and code are all welcome.

- **Bugs and ideas:** open an issue, one problem or idea per issue. For a bug, include the version
  (`haumea-server --version`), your operating system, how you run it, and the lines of `log.txt`
  around the problem.
- **Questions:** the [Haumea Discord](https://discord.gg/YSxtANrsB4).
- **Security problems:** never in a public issue; see [SECURITY.md](SECURITY.md).

Everyone taking part follows the [code of conduct](CODE_OF_CONDUCT.md).

## Building and testing

```sh
make            # build/haumea-server, static, for any x86-64 Linux
make windows    # build/haumea-server.exe (MinGW-w64)
make check
tests/smoke.sh build/haumea-server
```

CI runs the same on every push and pull request.

## Rules for changes

- **The protocol is shared with Haumea Studio.** The multiplayer core lives in `core/`, and
  Haumea Studio keeps a copy (`scripts/sync-to-haumea.sh /path/to/haumea` updates it). A change to
  what goes over the wire bumps `MP_PROTOCOL` in `core/mpprotocol.h`, and both are released
  together.
- **Keep it one small program** with no dependencies beyond what is in `core/`.
- **Docs in both languages.** A change people will notice goes in `CHANGELOG.md`, and in
  `docs/USER_GUIDE.md` and `docs/USER_GUIDE.pt-BR.md` when it changes how the server is used.
- **Licence.** Contributions are GPL-2.0-or-later, as the rest of the project.

## Releasing

Push a `v*` tag (`v1.0.1`, say): CI builds Linux and Windows and publishes a release with both, the
`deploy/` files and `SHA256SUMS`. Move the changelog's Unreleased section under the version first.
