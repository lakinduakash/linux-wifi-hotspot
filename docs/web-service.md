# Web Configuration Service

This document describes the add-on web experience for linux-wifi-hotspot. The
goal is to provide a headless-friendly management portal for configuring access
points, NetworkManager client mode, and future services (e.g., dragonsync)
without relying on the GTK UI.

## Components

1. **hotspotd** (Rust, runs as root)
   - Exposes an authenticated REST-ish API over a UNIX socket.
   - Owns `/etc/create_ap.conf` and writes updates atomically.
   - Controls the `create_ap` service (start, stop, restart, enable/disable).
   - Talks to NetworkManager over D-Bus to perform scans and connection
     management on the Wi-Fi interface shared between AP + client modes.
   - Acts as a capability broker for future system integrations (VPN, dragonsync).

2. **Web UI service** (unprivileged)
   - Provides a SPA dashboard for hotspot status, clients, upstream connectivity,
     and service health.
   - Authenticates local administrators (password + optional TOTP/SAML later).
   - Communicates with hotspotd via its UNIX socket (default
     `/run/hotspotd/hotspotd.sock`) using an API token negotiated at install
     time.
   - Continues running when the Wi-Fi AP is disabled so wired management is
     always available.

3. **Static assets**
   - Built via SvelteKit (TBD) and bundled into `/usr/share/linux-wifi-hotspot/webui`.
   - Served either by the Node/Denon server used during development or directly
     from hotspotd in production through an embedded file server.

With these components in place the headless workflow includes managing
NetworkManager client profiles entirely from the web UI: scanning for upstream
APs, connecting/disconnecting with the shared dongle, and forgetting stored
credentials (either individually or en masse) before shipping a unit.

When you want to rely on Ethernet only, the dashboard exposes buttons wired to
`POST /hotspot/disable` and `POST /hotspot/enable`. Disabling issues
`systemctl disable --now create_ap`, which tears down hostapd but keeps hotspotd
and the web server accessible over the wired link so you can reconfigure later.

For observability, the helper can tail `journalctl -u create_ap` and call
`create_ap --list-clients` so the web UI can show which stations are connected
to the AP, along with the last N log lines for debugging radio issues.

## API outline

| Method | Path              | Description                                       |
|--------|-------------------|---------------------------------------------------|
| GET    | `/healthz`        | readiness probe                                   |
| GET    | `/config`         | read `create_ap` key/value map                    |
| PUT    | `/config`         | validate + write config, restart service optional |
| GET    | `/hotspot/status` | active/enabled flags for the create_ap service    |
| GET    | `/hotspot/settings` | read hotspot interface + SSID/band settings      |
| PUT    | `/hotspot/settings` | update hotspot interface/SSID/security (optionally restart) |
| GET    | `/hotspot/clients`| list Wi-Fi clients (uses default iface unless `?iface=` provided) |
| GET    | `/hotspot/logs`   | tail `journalctl -u create_ap` (default 200 lines)|
| POST   | `/hotspot/start`  | `systemctl start create_ap`                       |
| POST   | `/hotspot/stop`   | `systemctl stop create_ap`                        |
| POST   | `/hotspot/enable` | `systemctl enable --now create_ap`                |
| POST   | `/hotspot/disable`| stop + `systemctl disable --now` (wired-only)     |
| GET    | `/nm/devices`     | list Wi-Fi devices and states                     |
| GET    | `/nm/status`      | combined device + active connection summary       |
| POST   | `/nm/scan`        | trigger scan on given interface                   |
| POST   | `/nm/connect`     | connect interface to upstream AP                  |
| POST   | `/nm/disconnect`  | disconnect interface                              |
| GET    | `/nm/connections` | list saved NM connections (including active)      |
| DELETE | `/nm/connections/:id` | delete/forget a saved connection by UUID or name |
| GET    | `/nm/connections/:id/detail` | fetch IPv4/wifi details for a profile    |
| PUT    | `/nm/connections/:id/ipv4` | update IPv4 method/address/gateway/DNS        |
| POST   | `/nm/connections` | create a new NM profile (Ethernet or Wi-Fi, incl. hidden SSIDs) |
| POST   | `/nm/connections/:id/activate` | bring a saved profile up (`nmcli connection up`) |
| DELETE | `/nm/connections` | delete all saved Wi-Fi connections (factory reset)|
| POST   | `/nm/wifi/connect` | connect a Wi-Fi interface to an SSID (client mode)|
| GET    | `/services`       | status for dragonsync/VPN/etc.                    |
| POST   | `/services/{id}/{action}` | run `start`, `stop`, or `restart` on a managed service |

All mutating endpoints require a bearer token tied to the service account
(`hotspot-web`). Requests are logged to `/var/log/hotspotd.log` for audit.

### Authentication

Set `HOTSPOTD_API_TOKEN` inside `/etc/linux-wifi-hotspot/hotspotd.env` (or pass
`--api-token` to the daemon) to require an `Authorization: Bearer <token>`
header or `X-Hotspotd-Token` header on every request. When unset, the helper
assumes it is only reachable via the protected UNIX socket. For deployments
exposing the HTTP listener, always configure a long, random token.

To expose the API over HTTP for the browser-based UI, add for example
`HTTP_LISTEN=127.0.0.1:8085` to `/etc/linux-wifi-hotspot/hotspotd.env` and
restart `hotspotd.service`. The systemd unit will expand that variable into
`--http-listen 127.0.0.1:8085`, and CORS is enabled automatically.

### Managing NetworkManager from the web UI

The new NM endpoints allow the frontend to present a list of physical devices,
connection profiles, and their IP configuration. A client can fetch
`/nm/connections/:uuid/detail`, render the IPv4 method/address/gateway/DNS, and
submit edits back via `PUT /nm/connections/:uuid/ipv4`. Wi-Fi client onboarding
can be scripted with `POST /nm/wifi/connect` (interface + SSID + passphrase),
which creates/updates the NM profile before the hotspot reuses the interface.
You can also create profiles manually (including hidden SSIDs or Ethernet-only
connections) via `POST /nm/connections`, and bring them up with the
`/nm/connections/:id/activate` helper rather than shelling into `nmcli`.

## Permissions model

- `/etc/create_ap.conf` stays `root:root 0600`.
- `/run/hotspotd` directory is `root:hotspotd 0770`, socket `0660`.
- Web service runs as `hotspot-web` in the `hotspotd` group.
- Optional polkit rule can permit interactive admins (desktop) to call helper
  endpoints via `pkexec` for debugging.

## Installation flow

1. `sudo make install` builds the GTK app plus the new helper + web assets,
   installs `hotspotd` to `/usr/lib/linux-wifi-hotspot/`, and drops the systemd
   unit + `/etc/linux-wifi-hotspot/hotspotd.env` template.
2. During provisioning make sure a `hotspotd` system user/group exists, and
   enable both `hotspotd.service` and the forthcoming `hotspot-webui.service`.
3. Default API token is generated at install time and written to
   `/etc/linux-wifi-hotspot/webui.env`. First login forces password change.
4. On boot the AP is enabled via `create_ap.service`, hotspotd loads the config,
   and web UI becomes reachable at `http://hotspot.local/` or the AP’s default
   IP (192.168.12.1).

### Filesystem + service setup

Manual steps if you are testing before packaging:

```
sudo useradd --system --home /var/lib/hotspot-web --shell /usr/sbin/nologin hotspot-web
sudo groupadd --system hotspotd
sudo usermod -a -G hotspotd hotspot-web
sudo install -d -m 0770 -o root -g hotspotd /run/hotspotd
sudo install services/hotspotd/target/release/hotspotd /usr/lib/linux-wifi-hotspot/hotspotd
sudo install -m 0644 services/hotspotd/hotspotd.service /etc/systemd/system/hotspotd.service
echo 'HOTSPOTD_API_TOKEN=generate-a-long-token' | sudo tee /etc/linux-wifi-hotspot/hotspotd.env
sudo systemctl enable --now hotspotd
```

Web UI service (Node or Rust) should ship its own unit file that sets
`User=hotspot-web` and `Group=hotspotd`. The service can either bind to localhost
port 8080 or serve through nginx; in both cases it needs read/write access to
`/run/hotspotd/hotspotd.sock`.

## Default SSID and passphrase

When `/etc/create_ap.conf` does not exist, `hotspotd` seeds it using the
template shipped in `src/scripts/create_ap.conf`. You can override the
initial SSID/passphrase/interfaces by exporting variables in
`/etc/linux-wifi-hotspot/hotspotd.env` (or via CLI flags):

```
HOTSPOTD_DEFAULT_SSID=DragonKit
HOTSPOTD_DEFAULT_PASSPHRASE=ChangeMe1234
HOTSPOTD_DEFAULT_WIFI_IFACE=wlan0
HOTSPOTD_DEFAULT_INTERNET_IFACE=eth0
```

This provides deterministic credentials for factory images. Encourage end
users to change them immediately using the web UI or CLI (`wihotspot`) before
going online.

The sample `/etc/linux-wifi-hotspot/hotspotd.env` dropped by `make install`
contains commented defaults for SSID, passphrase, interfaces, and the API token.

### Managed services

Expose additional systemd units (e.g., dragonsync, VPN daemons) through the web
API by setting `HOTSPOTD_MANAGED_SERVICES` to a comma-separated list of
`id:unit[:description]` entries. For example:

```
HOTSPOTD_MANAGED_SERVICES=dragonsync:dragonsync.service:DragonSync OTA,vpn:vpnkit.service:VPN Tunnel
```

The `/services` endpoint reports their status and `POST /services/{id}/{start|stop|restart}`
invokes `systemctl` on the corresponding unit.

## Future enhancements

- Serve HTTPS (self-signed on first boot, allow uploading certificates).
- Add onboarding wizard (set SSID/password, choose upstream network, set admin
  password).
- Provide OTA/firmware update hooks plus dragonsync configuration panels.
- Export metrics via Prometheus and push structured logs to journald.
