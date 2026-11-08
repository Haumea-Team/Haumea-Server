# Security

haumea-server holds accounts and people's maps, so please report weaknesses privately: use
**Security → Report a vulnerability** on this repository (GitHub's private reporting), not a
public issue. Include what you found, how to reproduce it, and the version (`git describe`, or the
release's tag).

What it protects, and how, is in the README (*Encrypted, and it proves who it is*). Known limits:
the first connection to a server trusts its fingerprint (as SSH does), and anyone in a session can
send the custom model files everyone's Haumea loads, so give the builder role to people you trust.
