## Work in progress notes

- Config writes are now 0600: `services/hotspotd/src/config.rs` uses `OpenOptions` + `.mode(0o600)` to protect `/etc/create_ap.conf` secrets.
- NetworkManager helpers now check `nmcli` exit codes when setting wifi hidden/password/autoconnect and when disabling autoconnect after wifi connect: `services/hotspotd/src/nm.rs`.
- Installer respects `PREFIX` for the shared template: `Makefile` installs `create_ap.conf` into `$(PREFIX)/share/linux-wifi-hotspot`.
- Git ignores build artifacts: `.gitignore` now lists `services/hotspotd/target/`, `webui/node_modules/`, `webui/dist/`.
- Makefile has `webui-build` to run `npm ci && npm run build` (no install step); output stays under `webui/dist`.
- Socket perms reminder: current `hotspotd.service` runs as `root:root` with `RuntimeDirectoryMode=0770`, so only root/root-group processes can reach `/run/hotspotd/hotspotd.sock`. Exposing HTTP or changing the service group is needed for an unprivileged web server.

TODO before committing:
- Prune generated folders: `rm -rf services/hotspotd/target webui/node_modules webui/dist`.
- Rebuild when needed: `cargo build -p hotspotd` (from repo root or services/hotspotd) and `npm install && npm run build` in `webui`.
- Usual install reminder if cargo is in user profile: `CARGO="$(which cargo)" sudo -E make install`.
