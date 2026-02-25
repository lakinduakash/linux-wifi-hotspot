# linux-wifi-hotspot Web UI

Placeholder directory for the forthcoming headless management interface.

## Planned stack

- **Framework:** SvelteKit (SSR disabled) or SolidStart. Final choice pending.
- **Styling:** TailwindCSS + shadcn/ui components with a dark-first theme.
- **State management:** TanStack Query for API caching/polling.
- **Build output:** Static assets emitted to `webui/dist/` and copied into
  `/usr/share/linux-wifi-hotspot/webui` during installation.

## Screens/pages

1. **Dashboard**
   - AP status + toggle (start/stop + enable/disable for wired-only mode)
   - Connected clients table (MAC, hostname, bytes)
   - Upstream NetworkManager status (SSID, signal, IP)
   - Quick action buttons (restart hotspot, restart dragonsync)

2. **Hotspot configuration**
   - SSID, WPA2/WPA3 passphrase, band (2.4/5 GHz), channel, isolation flags.
   - Advanced: custom hostapd options, MAC allow/deny lists, NAT/bridge mode.
   - “Apply” triggers PUT `/config` + optional restart.

3. **NetworkManager**
   - Scan results for selected Wi-Fi interface.
   - Connect/disconnect workflow that reuses the same dongle for client mode.
   - Saved connections list.

4. **Services**
   - Status + control for dragonsync, VPN tunnels, etc.
   - Future: update hooks and log viewer.

## Development workflow

```
npm install
npm run dev -- --host 0.0.0.0
```

During development the frontend talks to a mock API server (see
`webui/mock-server.ts`). In production the compiled assets are served either by
`hotspotd` or a minimal Node adapter that proxies API calls through the UNIX
socket.
