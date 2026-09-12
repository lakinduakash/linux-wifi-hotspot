# hotspotd

`hotspotd` is a privileged helper daemon that exposes a narrow API for the
linux-wifi-hotspot web interface. It is responsible for:

- reading and writing `/etc/create_ap.conf`
- starting/stopping the `create_ap` systemd service
- querying hostapd status and connected clients
- orchestrating NetworkManager actions (scan, connect, disconnect)
- reporting/controlling other services (e.g., dragonsync) in future modules

## Architecture

```
web UI (unprivileged) ──> hotspotd (root) ──> create_ap, NetworkManager, systemd
```

The daemon exposes a tiny HTTP API over either a UNIX domain socket
(`/run/hotspotd/hotspotd.sock`) or a localhost TCP port. Authentication will be
handled via shared tokens once the UI is wired up.

## Running locally

```
$ cd services/hotspotd
$ cargo run -- --config-path /etc/create_ap.conf --runtime-dir /tmp/hotspotd
```

The binary listens on the UNIX socket by default; pass `--http-listen 127.0.0.1:8085`
to expose an HTTP port for development.

## Systemd service (planned)

```
[Unit]
Description=Hotspot helper for linux-wifi-hotspot web UI
After=network.target

[Service]
ExecStart=/usr/lib/linux-wifi-hotspot/hotspotd --config-path /etc/create_ap.conf --runtime-dir /run/hotspotd
User=root
Group=root
RuntimeDirectory=hotspotd
AmbientCapabilities=CAP_NET_ADMIN CAP_NET_RAW

[Install]
WantedBy=multi-user.target
```

The installer will provision `/run/hotspotd` with `0770` permissions and group
`hotspotd`. Any process that needs to call the API (e.g., the web server) will
join that group instead of running as root or touching `/etc/create_ap.conf`
directly.

### API authentication

Set `HOTSPOTD_API_TOKEN` (either in `/etc/linux-wifi-hotspot/hotspotd.env` or
via the `--api-token` CLI flag) to force callers to send `Authorization: Bearer
<token>` or `X-Hotspotd-Token: <token>` headers. When unset, the helper assumes
only local UNIX socket clients can reach it.

## Default configuration overrides

If `/etc/create_ap.conf` is missing, the daemon seeds it using the upstream
template. Override the initial SSID/passphrase/interfaces via CLI flags or
environment variables (set inside `/etc/linux-wifi-hotspot/hotspotd.env`; a
template is installed as `hotspotd.env.example`):

```
HOTSPOTD_DEFAULT_SSID=DragonKit
HOTSPOTD_DEFAULT_PASSPHRASE=ChangeMe1234
HOTSPOTD_DEFAULT_WIFI_IFACE=wlan0
HOTSPOTD_DEFAULT_INTERNET_IFACE=eth0
```

These values are only applied when the config file does not yet exist.

### Managed services

You can expose additional systemd units (for example `dragonsync.service`) via
the REST API by setting `HOTSPOTD_MANAGED_SERVICES` to a comma-separated list of
`id:unit[:description]`. Example:

```
HOTSPOTD_MANAGED_SERVICES=dragonsync:dragonsync.service:DragonSync OTA,vpn:vpnkit.service
```

After setting, `/services` shows their status and `POST /services/<id>/{start|stop|restart}`
will relay the request to `systemctl`.
