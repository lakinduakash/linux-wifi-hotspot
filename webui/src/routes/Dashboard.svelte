<script lang="ts">
  import { onMount } from 'svelte';
  import { api } from '$lib/api';
  import QRCode from 'qrcode';

  let loading = true;
  let error: string | null = null;

  let hotspotStatus: any = null;
  let clients: any[] = [];
  let logs = '';
  let nmStatus: any = null;
  let services: any[] = [];
  let deviceList: any[] = [];
  let connections: any[] = [];
  let selectedConnection: string = '';
  let connectionDetailData: any = null;
  let ipv4Form = { method: 'auto', address: '', gateway: '', dns: '' };
  let connectionForm = {
    name: '',
    iface: '',
    type: '802-3-ethernet',
    ssid: '',
    password: '',
    autoconnect: true,
  };
  let wifiForm = { iface: '', ssid: '', password: '', autoconnect: true };
  let wifiNetworks: any[] = [];
  let hotspotSettings: any = {};
  let hotspotForm = {
    wifi_iface: '',
    internet_iface: '',
    freq_band: '2.4',
    ssid: '',
    passphrase: '',
    channel: '1',
    security: '2',
    hidden: false,
    open_network: false,
    no_virtual: false,
    restart: true,
    share_method: 'nat',
    gateway: '192.168.12.1',
    dhcp_dns: 'gateway',
    driver: 'nl80211',
    country: '',
    isolate_clients: false,
    no_dns: false,
    no_dnsmasq: false,
    no_haveged: false,
    ieee80211n: false,
    ieee80211ac: false,
    ieee80211ax: false,
    ht_capab: '[HT40+]',
    vht_capab: '',
    new_macaddr: '',
    dhcp_hosts: '',
  };
  let hotspotQr = '';
  let hotspotQrText = '';
  type TabId = 'hotspot' | 'network' | 'logs';
  const tabs: { id: TabId; label: string }[] = [
    { id: 'hotspot', label: 'Hotspot' },
    { id: 'network', label: 'Network' },
    { id: 'logs', label: 'Logs' }
  ];
  let activeTab: TabId = 'hotspot';

  async function loadAll() {
    loading = true;
    error = null;
    try {
      const [hs, cl, lg, nm, svc, conns, hotspotCfg] = await Promise.all([
        api.hotspotStatus(),
        api.hotspotClients(),
        api.hotspotLogs(100),
        api.nmStatus(),
        api.listServices(),
        api.listConnections(),
        api.getHotspotSettings()
      ]);
      hotspotStatus = hs;
      clients = cl.clients || [];
      logs = lg.logs || '';
      nmStatus = nm;
      services = svc.services || [];
      const rawConnections = conns.connections || [];
      const filteredConnections = rawConnections.filter((conn) =>
        ['802-3-ethernet', '802-11-wireless'].includes(conn.conn_type)
      );
      connections = filteredConnections.length > 0 ? filteredConnections : rawConnections;
      hotspotSettings = hotspotCfg.settings || {};
      hotspotForm = {
        wifi_iface: hotspotSettings.wifi_iface || hotspotForm.wifi_iface || '',
        internet_iface: hotspotSettings.internet_iface || hotspotForm.internet_iface || '',
        freq_band: hotspotSettings.freq_band || hotspotForm.freq_band || '2.4',
        ssid: hotspotSettings.ssid || '',
        passphrase: hotspotSettings.passphrase || '',
        channel: hotspotSettings.channel || '1',
        security: hotspotSettings.wpa_version || hotspotForm.security || '2',
        hidden: hotspotSettings.hidden ?? hotspotForm.hidden ?? false,
        open_network: hotspotSettings.open_network ?? false,
        no_virtual: hotspotSettings.no_virtual ?? hotspotForm.no_virtual ?? false,
        restart: false,
        share_method: hotspotSettings.share_method ?? hotspotForm.share_method ?? 'nat',
        gateway: hotspotSettings.gateway ?? hotspotForm.gateway ?? '192.168.12.1',
        dhcp_dns: hotspotSettings.dhcp_dns ?? hotspotForm.dhcp_dns ?? 'gateway',
        driver: hotspotSettings.driver ?? hotspotForm.driver ?? 'nl80211',
        country: hotspotSettings.country ?? hotspotForm.country ?? '',
        isolate_clients: hotspotSettings.isolate_clients ?? hotspotForm.isolate_clients ?? false,
        no_dns: hotspotSettings.no_dns ?? hotspotForm.no_dns ?? false,
        no_dnsmasq: hotspotSettings.no_dnsmasq ?? hotspotForm.no_dnsmasq ?? false,
        no_haveged: hotspotSettings.no_haveged ?? hotspotForm.no_haveged ?? false,
        ieee80211n: hotspotSettings.ieee80211n ?? hotspotForm.ieee80211n ?? false,
        ieee80211ac: hotspotSettings.ieee80211ac ?? hotspotForm.ieee80211ac ?? false,
        ieee80211ax: hotspotSettings.ieee80211ax ?? hotspotForm.ieee80211ax ?? false,
        ht_capab: hotspotSettings.ht_capab ?? hotspotForm.ht_capab ?? '[HT40+]',
        vht_capab: hotspotSettings.vht_capab ?? hotspotForm.vht_capab ?? '',
        new_macaddr: hotspotSettings.new_macaddr ?? hotspotForm.new_macaddr ?? '',
        dhcp_hosts: hotspotSettings.dhcp_hosts ?? hotspotForm.dhcp_hosts ?? '',
      };
      deviceList = nm.devices || [];
      if (!wifiForm.iface) {
        const firstWifi = deviceList.find((d) => d.type === 'wifi');
        if (firstWifi) {
          wifiForm.iface = firstWifi.name;
        }
      }
      if (connections.length > 0) {
        const defaultConn =
          connections.find((c) => c.device === nmStatus?.default_iface) ||
          connections.find((c) => c.active) ||
          connections[0];
        if (!selectedConnection || !connections.some((c) => c.uuid === selectedConnection)) {
          selectedConnection = defaultConn.uuid;
          await loadConnectionDetail(defaultConn.uuid);
        }
      } else {
        selectedConnection = '';
        connectionDetailData = null;
      }
    } catch (err: any) {
      error = err?.message || 'Failed to reach hotspotd API';
    } finally {
      loading = false;
    }
  }

  onMount(() => {
    loadAll();
  });

  async function handleService(id: string, action: 'start' | 'stop' | 'restart') {
    try {
      await api.runServiceAction(id, action);
      await loadAll();
    } catch (err: any) {
      error = err?.message || `Failed to ${action} service`;
    }
  }

  async function loadConnectionDetail(id: string) {
    try {
      selectedConnection = id;
      const detail = await api.connectionDetail(id);
      if (detail.status !== 'ok' || !detail.connection) {
        connectionDetailData = null;
        error = detail.message || 'Failed to load connection details';
        return;
      }
      connectionDetailData = detail.connection;
      ipv4Form.method = connectionDetailData?.ipv4?.method || 'auto';
      ipv4Form.address = connectionDetailData?.ipv4?.addresses?.[0] || '';
      ipv4Form.gateway = connectionDetailData?.ipv4?.gateway || '';
      ipv4Form.dns = (connectionDetailData?.ipv4?.dns || []).join(', ');
      error = null;
    } catch (err: any) {
      error = err?.message || 'Failed to load connection details';
    }
  }

  async function saveIpv4() {
    if (!selectedConnection) return;
    try {
      await api.updateConnectionIpv4(selectedConnection, {
        method: ipv4Form.method,
        address: ipv4Form.address,
        gateway: ipv4Form.gateway,
        dns: ipv4Form.dns,
      });
      await loadConnectionDetail(selectedConnection);
    } catch (err: any) {
      error = err?.message || 'Failed to update IPv4 settings';
    }
  }

  async function deleteSelectedConnection() {
    if (!selectedConnection) return;
    const confirmed = confirm('Delete this NetworkManager connection?');
    if (!confirmed) return;
    try {
      await api.deleteConnection(selectedConnection);
      await loadAll();
    } catch (err: any) {
      error = err?.message || 'Failed to delete connection';
    }
  }

  async function disconnectSelectedConnection() {
    if (!connectionDetailData?.device) return;
    try {
      await api.disconnectDevice(connectionDetailData.device);
      await loadAll();
    } catch (err: any) {
      error = err?.message || 'Failed to disconnect interface';
    }
  }

  async function activateSelectedConnection() {
    if (!selectedConnection) return;
    try {
      await api.activateConnection(selectedConnection);
      await loadAll();
    } catch (err: any) {
      error = err?.message || 'Failed to activate connection';
    }
  }

  async function connectWifiClient() {
    try {
      await api.wifiConnect(wifiForm);
      wifiForm.ssid = '';
      wifiForm.password = '';
      await loadAll();
    } catch (err: any) {
      error = err?.message || 'Failed to connect to Wi-Fi';
    }
  }

  async function disconnectWifiClient() {
    if (!wifiForm.iface) return;
    try {
      await api.disconnectDevice(wifiForm.iface);
      await loadAll();
    } catch (err: any) {
      error = err?.message || 'Failed to disconnect Wi-Fi interface';
    }
  }

  async function createConnection() {
    if (!connectionForm.name.trim()) {
      error = 'Connection name is required';
      return;
    }
    if (!connectionForm.iface) {
      error = 'Select an interface for the new connection';
      return;
    }
    const payload: any = {
      name: connectionForm.name.trim(),
      type: connectionForm.type,
      iface: connectionForm.iface,
      autoconnect: connectionForm.autoconnect,
    };
    if (connectionForm.type === '802-11-wireless') {
      if (!connectionForm.ssid.trim()) {
        error = 'SSID is required for Wi-Fi connections';
        return;
      }
      payload.ssid = connectionForm.ssid.trim();
      if (connectionForm.password.trim().length > 0) {
        payload.password = connectionForm.password;
      }
      payload.hidden = connectionForm.hidden;
    }
    try {
      await api.createConnection(payload);
      connectionForm.name = '';
      connectionForm.ssid = '';
      connectionForm.password = '';
      connectionForm.hidden = false;
      connectionForm.autoconnect = true;
      error = null;
      await loadAll();
    } catch (err: any) {
      error = err?.message || 'Failed to create connection';
    }
  }

  function onConnectionChange(event: Event) {
    const target = event.target as HTMLSelectElement;
    loadConnectionDetail(target.value);
  }

  $: wifiDevices = deviceList.filter((d) => d.type === 'wifi');
  $: ethernetDevices = deviceList.filter((d) => d.type === 'ethernet');
  $: if (!hotspotForm.wifi_iface && wifiDevices.length > 0) {
    hotspotForm.wifi_iface = wifiDevices[0].name;
  }
  $: if (wifiDevices.length > 0) {
    const exists = wifiDevices.some((d) => d.name === wifiForm.iface);
    if (!wifiForm.iface || !exists) {
      wifiForm.iface = wifiDevices[0].name;
    }
  }

  $: {
    if (connectionForm.type === '802-11-wireless') {
      if (
        connectionForm.iface &&
        wifiDevices.some((d) => d.name === connectionForm.iface)
      ) {
        // ok
      } else if (wifiDevices.length > 0) {
        connectionForm.iface = wifiDevices[0].name;
      }
    } else {
      const candidates = ethernetDevices.length > 0 ? ethernetDevices : deviceList;
      if (
        connectionForm.iface &&
        candidates.some((d) => d.name === connectionForm.iface)
      ) {
        // ok
      } else if (candidates.length > 0) {
        connectionForm.iface = candidates[0].name;
      }
    }
  }
  $: internetCandidates = deviceList.filter((d) =>
    ['ethernet', 'wifi'].includes(d.type)
  );
  $: if (!hotspotForm.internet_iface) {
    const preferred =
      internetCandidates.find((d) => d.name === nmStatus?.default_iface) ||
      internetCandidates.find((d) => d.type === 'ethernet') ||
      internetCandidates[0];
    if (preferred) {
      hotspotForm.internet_iface = preferred.name;
    }
  }
  $: if (hotspotForm.open_network) {
    hotspotForm.passphrase = '';
  } else if (hotspotForm.passphrase && hotspotForm.passphrase.length > 0) {
    hotspotForm.open_network = false;
  }

  async function scanWifi() {
    if (!wifiForm.iface) {
      error = 'Select a Wi-Fi interface first';
      return;
    }
    try {
      const resp = await api.wifiScan(wifiForm.iface);
      wifiNetworks = resp.networks || [];
      if (wifiNetworks.length === 0) {
        error = 'No networks found';
      } else {
        error = null;
      }
    } catch (err: any) {
      error = err?.message || 'Failed to scan Wi-Fi';
    }
  }

  function useNetwork(ssid: string) {
    wifiForm.ssid = ssid;
  }

  async function saveHotspotSettings() {
    if (!hotspotForm.open_network && (!hotspotForm.passphrase || hotspotForm.passphrase.length < 8)) {
      error = 'Passphrase must be at least 8 characters for secured networks';
      return;
    }
    try {
      await api.updateHotspotSettings({
        wifi_iface: hotspotForm.wifi_iface,
        internet_iface: hotspotForm.internet_iface,
        freq_band: hotspotForm.freq_band,
        ssid: hotspotForm.ssid,
        passphrase: hotspotForm.passphrase,
        channel: hotspotForm.channel,
        wpa_version: hotspotForm.open_network ? undefined : hotspotForm.security,
        hidden: hotspotForm.hidden,
        open_network: hotspotForm.open_network,
        no_virtual: hotspotForm.no_virtual,
        restart: hotspotForm.restart,
        share_method: hotspotForm.share_method,
        gateway: hotspotForm.gateway,
        dhcp_dns: hotspotForm.dhcp_dns,
        driver: hotspotForm.driver,
        country: hotspotForm.country,
        isolate_clients: hotspotForm.isolate_clients,
        no_dns: hotspotForm.no_dns,
        no_dnsmasq: hotspotForm.no_dnsmasq,
        no_haveged: hotspotForm.no_haveged,
        ieee80211n: hotspotForm.ieee80211n,
        ieee80211ac: hotspotForm.ieee80211ac,
        ieee80211ax: hotspotForm.ieee80211ax,
        ht_capab: hotspotForm.ht_capab,
        vht_capab: hotspotForm.vht_capab,
        new_macaddr: hotspotForm.new_macaddr,
        dhcp_hosts: hotspotForm.dhcp_hosts,
      });
      await loadAll();
    } catch (err: any) {
      error = err?.message || 'Failed to update hotspot settings';
    }
  }

  async function startHotspot() {
    try {
      await api.hotspotStart();
      await loadAll();
    } catch (err: any) {
      error = err?.message || 'Failed to start hotspot';
    }
  }

  async function stopHotspot() {
    try {
      await api.hotspotStop();
      await loadAll();
    } catch (err: any) {
      error = err?.message || 'Failed to stop hotspot';
    }
  }

  async function enableHotspot() {
    try {
      await api.hotspotEnable();
      await loadAll();
    } catch (err: any) {
      error = err?.message || 'Failed to enable hotspot';
    }
  }

  async function disableHotspot() {
    try {
      await api.hotspotDisable();
      await loadAll();
    } catch (err: any) {
      error = err?.message || 'Failed to disable hotspot';
    }
  }

  async function generateHotspotQr() {
    const ssid = hotspotForm.ssid?.trim();
    if (!ssid) {
      hotspotQrText = '';
      hotspotQr = '';
      return;
    }
    const type = hotspotForm.open_network ? 'nopass' : 'WPA';
    let qr = `WIFI:T:${type};S:${ssid};`;
    if (!hotspotForm.open_network) {
      qr += `P:${hotspotForm.passphrase || ''};`;
    }
    if (hotspotForm.hidden) {
      qr += 'H:true;';
    }
    qr += ';';
    hotspotQrText = qr;
    try {
      hotspotQr = await QRCode.toDataURL(qr);
    } catch (e) {
      hotspotQr = '';
    }
  }

  $: hotspotForm.ssid, hotspotForm.passphrase, hotspotForm.open_network, hotspotForm.hidden, (() => {
    void generateHotspotQr();
  })();
</script>

<svelte:head>
  <title>WiFi Hotspot Dashboard</title>
</svelte:head>

<div class="page">
  <header>
    <h1>WiFi Hotspot</h1>
    <button on:click={loadAll}>Refresh</button>
  </header>

  {#if loading}
    <div class="card">Loading…</div>
  {:else if error}
    <div class="card error">{error}</div>
  {:else}
    <div class="tab-bar">
      {#each tabs as tab}
        <button
          type="button"
          class:active={activeTab === tab.id}
          on:click={() => (activeTab = tab.id)}
        >
          {tab.label}
        </button>
      {/each}
    </div>

    {#if activeTab === 'hotspot'}
      <section class="grid">
        <div class="card">
          <h2>Hotspot</h2>
          <p>Active: {hotspotStatus?.service?.active ? 'Yes' : 'No'}</p>
          <p>Enabled on boot: {hotspotStatus?.service?.enabled ? 'Yes' : 'No'}</p>
          <div class="button-row">
            <button type="button" on:click={startHotspot}>Start</button>
            <button type="button" on:click={stopHotspot}>Stop</button>
            <button type="button" on:click={enableHotspot}>Enable on Boot</button>
            <button type="button" on:click={disableHotspot}>Disable on Boot</button>
          </div>
          <div class="form-row">
            <label for="hotspot-wifi">Hotspot Wi-Fi Interface</label>
            {#if wifiDevices.length > 0}
              <select id="hotspot-wifi" bind:value={hotspotForm.wifi_iface}>
                {#each wifiDevices as dev}
                  <option value={dev.name}>{dev.name} ({dev.state})</option>
                {/each}
              </select>
            {:else}
              <input id="hotspot-wifi" placeholder="wlan0" bind:value={hotspotForm.wifi_iface} />
            {/if}
          </div>
          <div class="form-row">
            <label for="internet-iface">Internet Interface</label>
            <select id="internet-iface" bind:value={hotspotForm.internet_iface}>
              {#each internetCandidates as iface}
                <option value={iface.name}>{iface.name} ({iface.type})</option>
              {/each}
            </select>
          </div>
          <div class="form-row">
            <label for="freq-band">Frequency Band</label>
            <select id="freq-band" bind:value={hotspotForm.freq_band}>
              <option value="2.4">2.4 GHz</option>
              <option value="5">5 GHz</option>
            </select>
          </div>
          <div class="form-row">
            <label for="channel">Channel</label>
            <input id="channel" bind:value={hotspotForm.channel} />
          </div>
          <div class="form-row">
            <label for="ssid">SSID</label>
            <input id="ssid" bind:value={hotspotForm.ssid} />
          </div>
          <div class="form-row">
            <label for="passphrase">Passphrase</label>
            {#if hotspotForm.open_network}
              <input
                id="passphrase"
                type="text"
                placeholder="Open network"
                bind:value={hotspotForm.passphrase}
                disabled
              />
            {:else}
              <input
                id="passphrase"
                type="password"
                placeholder="Minimum 8 characters"
                bind:value={hotspotForm.passphrase}
              />
            {/if}
          </div>
          <div class="form-row">
            <label for="security">Security</label>
            <select id="security" bind:value={hotspotForm.security} disabled={hotspotForm.open_network}>
              <option value="2">WPA2</option>
              <option value="3">WPA3</option>
            </select>
          </div>
          <div class="form-row checkbox">
            <label><input type="checkbox" bind:checked={hotspotForm.hidden} /> Hidden SSID</label>
          </div>
          <div class="form-row checkbox">
            <label><input type="checkbox" bind:checked={hotspotForm.open_network} /> Open Network</label>
          </div>
        <div class="form-row checkbox">
          <label><input type="checkbox" bind:checked={hotspotForm.no_virtual} /> Force --no-virt</label>
        </div>
        <div class="form-row checkbox">
          <label
            ><input type="checkbox" bind:checked={hotspotForm.restart} /> Restart hotspot after saving</label
          >
        </div>
        <details class="advanced">
          <summary>Advanced settings</summary>
          <div class="advanced-grid">
            <div class="form-row">
              <label for="share-method">Share Method</label>
              <select id="share-method" bind:value={hotspotForm.share_method}>
                <option value="nat">NAT (default)</option>
                <option value="bridge">Bridge</option>
                <option value="none">None (router only)</option>
              </select>
            </div>
            <div class="form-row">
              <label for="hotspot-gateway">Gateway</label>
              <input id="hotspot-gateway" placeholder="192.168.12.1" bind:value={hotspotForm.gateway} />
            </div>
            <div class="form-row">
              <label for="dhcp-dns">DHCP DNS</label>
              <input
                id="dhcp-dns"
                placeholder="gateway or comma-separated IPs"
                bind:value={hotspotForm.dhcp_dns}
              />
            </div>
            <div class="form-row">
              <label for="driver">Driver</label>
              <input id="driver" list="driver-options" bind:value={hotspotForm.driver} />
              <datalist id="driver-options">
                <option value="nl80211" />
                <option value="wext" />
                <option value="none" />
              </datalist>
            </div>
            <div class="form-row">
              <label for="country">Country Code</label>
              <input id="country" placeholder="US" bind:value={hotspotForm.country} />
            </div>
            <div class="form-row">
              <label for="mac-addr">Custom MAC</label>
              <input id="mac-addr" placeholder="02:00:00:00:00:01" bind:value={hotspotForm.new_macaddr} />
            </div>
            <div class="form-row">
              <label for="ht-capab">HT Capabilities</label>
              <input id="ht-capab" placeholder="[HT40+]" bind:value={hotspotForm.ht_capab} />
            </div>
            <div class="form-row">
              <label for="vht-capab">VHT Capabilities</label>
              <input id="vht-capab" placeholder="[VHT160-80PLUS80]" bind:value={hotspotForm.vht_capab} />
            </div>
            <div class="form-row">
              <label for="dhcp-hosts">Static DHCP Hosts</label>
              <textarea
                id="dhcp-hosts"
                placeholder="aa:bb:cc:dd:ee:ff,192.168.12.50,hostname"
                bind:value={hotspotForm.dhcp_hosts}
              />
            </div>
          </div>
          <div class="checkbox-grid">
            <label><input type="checkbox" bind:checked={hotspotForm.isolate_clients} /> Isolate clients</label>
            <label><input type="checkbox" bind:checked={hotspotForm.no_dns} /> Disable DNS server</label>
            <label><input type="checkbox" bind:checked={hotspotForm.no_dnsmasq} /> Disable dnsmasq</label>
            <label><input type="checkbox" bind:checked={hotspotForm.no_haveged} /> Disable haveged</label>
            <label><input type="checkbox" bind:checked={hotspotForm.ieee80211n} /> Enable 802.11n</label>
            <label><input type="checkbox" bind:checked={hotspotForm.ieee80211ac} /> Enable 802.11ac</label>
            <label><input type="checkbox" bind:checked={hotspotForm.ieee80211ax} /> Enable 802.11ax</label>
          </div>
        </details>
        <button on:click={saveHotspotSettings}>Save Hotspot Settings</button>
        {#if !hotspotForm.open_network}
          <div class="qr-section">
            <h3>Share via QR</h3>
            {#if hotspotQr}
                <img src={hotspotQr} alt="Wi-Fi QR" />
              {/if}
              <code>{hotspotQrText}</code>
            </div>
          {/if}
        </div>
      </section>

      <section class="card">
        <h2>Connected Clients</h2>
        {#if clients.length === 0}
          <p>No clients connected</p>
        {:else}
          <table>
            <thead>
              <tr><th>MAC</th><th>IP</th><th>Hostname</th></tr>
            </thead>
            <tbody>
              {#each clients as client}
                <tr>
                  <td>{client.mac}</td>
                  <td>{client.ip || '-'}</td>
                  <td>{client.hostname || '-'}</td>
                </tr>
              {/each}
            </tbody>
          </table>
        {/if}
      </section>

      <section class="card">
        <h2>Services</h2>
        {#if services.length === 0}
          <p>No additional services configured.</p>
        {:else}
          <table>
            <thead>
              <tr><th>Name</th><th>Unit</th><th>Status</th><th>Actions</th></tr>
            </thead>
            <tbody>
              {#each services as svc}
                <tr>
                  <td>{svc.description || svc.id}</td>
                  <td>{svc.unit}</td>
                  <td>{svc.active ? 'Running' : 'Stopped'}</td>
                  <td>
                    <button on:click={() => handleService(svc.id, 'start')}>Start</button>
                    <button on:click={() => handleService(svc.id, 'stop')}>Stop</button>
                    <button on:click={() => handleService(svc.id, 'restart')}>Restart</button>
                  </td>
                </tr>
              {/each}
            </tbody>
          </table>
        {/if}
      </section>
    {:else if activeTab === 'network'}
      <section class="card">
        <h2>Network Manager</h2>
        <p>Default iface: {nmStatus?.default_iface || 'unknown'}</p>
        <p>Active connections: {nmStatus?.active_connections?.length || 0}</p>
      </section>

      <section class="card">
        <h2>Interfaces</h2>
        {#if deviceList.length === 0}
          <p>No interfaces reported by NetworkManager.</p>
        {:else}
          <table>
            <thead>
              <tr><th>Name</th><th>Type</th><th>State</th><th>Connection</th></tr>
            </thead>
            <tbody>
              {#each deviceList as dev}
                <tr>
                  <td>{dev.name}</td>
                  <td>{dev.type}</td>
                  <td>{dev.state}</td>
                  <td>{dev.connection || '-'}</td>
                </tr>
              {/each}
            </tbody>
          </table>
        {/if}
      </section>

    <section class="card">
      <h2>Network Connections</h2>
      <div class="sub-card">
        <h3>Create Connection</h3>
        <div class="form-row">
          <label for="new-conn-name">Name</label>
          <input id="new-conn-name" placeholder="My Connection" bind:value={connectionForm.name} />
        </div>
        <div class="form-row">
          <label for="new-conn-type">Type</label>
          <select id="new-conn-type" bind:value={connectionForm.type}>
            <option value="802-3-ethernet">Ethernet</option>
            <option value="802-11-wireless">Wi-Fi</option>
          </select>
        </div>
        <div class="form-row">
          <label for="new-conn-iface">Interface</label>
          {#if connectionForm.type === '802-11-wireless' && wifiDevices.length > 0}
            <select id="new-conn-iface" bind:value={connectionForm.iface}>
              {#each wifiDevices as dev}
                <option value={dev.name}>{dev.name} ({dev.state})</option>
              {/each}
            </select>
          {:else if connectionForm.type !== '802-11-wireless' && ethernetDevices.length > 0}
            <select id="new-conn-iface" bind:value={connectionForm.iface}>
              {#each ethernetDevices as dev}
                <option value={dev.name}>{dev.name} ({dev.state})</option>
              {/each}
            </select>
          {:else if deviceList.length > 0}
            <select id="new-conn-iface" bind:value={connectionForm.iface}>
              {#each deviceList as dev}
                <option value={dev.name}>{dev.name} ({dev.type})</option>
              {/each}
            </select>
          {:else}
            <input
              id="new-conn-iface"
              placeholder="Interface (e.g., wlx0...)"
              bind:value={connectionForm.iface}
            />
          {/if}
        </div>
        {#if connectionForm.type === '802-11-wireless'}
          <div class="form-row">
            <label for="new-conn-ssid">SSID</label>
            <input id="new-conn-ssid" placeholder="Network name" bind:value={connectionForm.ssid} />
          </div>
          <div class="form-row checkbox">
            <label><input type="checkbox" bind:checked={connectionForm.hidden} /> Hidden SSID</label>
          </div>
          <div class="form-row">
            <label for="new-conn-pass">Password</label>
            <input
              id="new-conn-pass"
              type="password"
              placeholder="Leave blank for open network"
              bind:value={connectionForm.password}
            />
          </div>
        {/if}
        <div class="form-row checkbox">
          <label><input type="checkbox" bind:checked={connectionForm.autoconnect} /> Autoconnect</label>
        </div>
        <div class="button-row">
          <button type="button" on:click={createConnection}>Add Connection</button>
        </div>
      </div>
      <p class="hint">Select an existing NM profile to view or update IPv4 settings (this controls the upstream interface).</p>
        {#if connections.length === 0}
          <p>No NetworkManager connections found.</p>
        {:else}
          <div class="form-row">
            <label for="connection-select">Profile</label>
            <select id="connection-select" bind:value={selectedConnection} on:change={onConnectionChange}>
              {#each connections as conn}
                <option value={conn.uuid}>
                  {conn.name} ({conn.conn_type}){conn.active ? ' *' : ''}
                </option>
              {/each}
            </select>
          </div>
          {#if connectionDetailData}
            <div class="form-row">
              <label for="interface-display">Interface</label>
              <p id="interface-display">{connectionDetailData.device || 'n/a'}</p>
            </div>
            <div class="form-row">
              <label for="ipv4-method">IPv4 Method</label>
              <select id="ipv4-method" bind:value={ipv4Form.method}>
                <option value="auto">Automatic (DHCP)</option>
                <option value="manual">Manual</option>
              </select>
            </div>
          {#if ipv4Form.method === 'manual'}
            <div class="form-row">
              <label for="ipv4-address">Address (CIDR)</label>
              <input id="ipv4-address" placeholder="192.168.1.10/24" bind:value={ipv4Form.address} />
            </div>
              <div class="form-row">
                <label for="ipv4-gateway">Gateway</label>
                <input id="ipv4-gateway" placeholder="192.168.1.1" bind:value={ipv4Form.gateway} />
              </div>
              <div class="form-row">
                <label for="ipv4-dns">DNS (comma separated)</label>
                <input id="ipv4-dns" placeholder="1.1.1.1,8.8.8.8" bind:value={ipv4Form.dns} />
              </div>
          {/if}
          <div class="button-row">
            <button on:click={saveIpv4}>Save IPv4 Settings</button>
            <button type="button" on:click={activateSelectedConnection}>Activate Connection</button>
            <button type="button" on:click={disconnectSelectedConnection}>Disconnect Interface</button>
            <button type="button" class="danger" on:click={deleteSelectedConnection}>Delete Profile</button>
          </div>
        {:else}
          <p>Select a connection to view details.</p>
        {/if}
      {/if}
      </section>

      <section class="card">
        <h2>Wi-Fi Client</h2>
        <div class="form-row">
          <label for="wifi-iface">Interface</label>
          {#if wifiDevices.length > 0}
            <select id="wifi-iface" bind:value={wifiForm.iface}>
              {#each wifiDevices as dev}
                <option value={dev.name}>{dev.name} ({dev.state})</option>
              {/each}
            </select>
          {:else}
            <input id="wifi-iface" placeholder="wlan0" bind:value={wifiForm.iface} />
          {/if}
        </div>
        <div class="form-row">
          <label for="wifi-ssid">SSID</label>
          <input id="wifi-ssid" bind:value={wifiForm.ssid} />
        </div>
        <div class="form-row">
          <label for="wifi-pass">Password</label>
          <input id="wifi-pass" type="password" bind:value={wifiForm.password} />
        </div>
        <div class="form-row checkbox">
          <label><input type="checkbox" bind:checked={wifiForm.autoconnect} /> Autoconnect</label>
        </div>
        <div class="form-row">
          <button type="button" on:click={scanWifi}>Scan Networks</button>
        </div>
      {#if wifiNetworks.length > 0}
        <div class="network-list">
          <table>
            <thead>
              <tr><th>SSID</th><th>Signal</th><th>Security</th><th></th></tr>
              </thead>
              <tbody>
                {#each wifiNetworks as net}
                  <tr>
                    <td>{net.ssid || '(hidden)'}</td>
                    <td>{net.strength}%</td>
                    <td>{net.security}</td>
                    <td><button type="button" on:click={() => useNetwork(net.ssid)}>Use</button></td>
                  </tr>
                {/each}
              </tbody>
            </table>
        </div>
      {/if}
      <div class="button-row">
        <button on:click={connectWifiClient}>Connect</button>
        <button type="button" on:click={disconnectWifiClient}>Disconnect</button>
      </div>
    </section>
    {:else}
      <section class="card">
        <h2>Logs</h2>
        <pre>{logs}</pre>
      </section>
    {/if}

  {/if}
</div>

<style>
  .page {
    padding: 2rem;
    color: #f3f4f6;
  }
  header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 1.5rem;
  }
  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(240px, 1fr));
    gap: 1rem;
    margin-bottom: 1rem;
  }
  .card {
    background: #1f2937;
    border-radius: 12px;
    padding: 1rem;
    box-shadow: 0 4px 20px rgba(0,0,0,0.3);
  }
  .error {
    border: 1px solid #f87171;
    color: #fca5a5;
  }
  .button-row {
    display: flex;
    flex-wrap: wrap;
    gap: 0.5rem;
    margin: 0.75rem 0;
  }
  table {
    width: 100%;
    border-collapse: collapse;
  }
  th, td {
    padding: 0.5rem;
    border-bottom: 1px solid #374151;
  }
  pre {
    max-height: 320px;
    overflow: auto;
    background: #0f172a;
    padding: 1rem;
    border-radius: 8px;
  }
  button {
    margin-right: 0.25rem;
    background: #2563eb;
    border: none;
    color: #fff;
    padding: 0.4rem 0.75rem;
    border-radius: 6px;
    cursor: pointer;
  }
  button.danger {
    background: #dc2626;
  }
  .tab-bar {
    display: flex;
    flex-wrap: wrap;
    gap: 0.5rem;
    margin-bottom: 1rem;
  }
  .tab-bar button {
    background: #111827;
    border: 1px solid #374151;
    color: #d1d5db;
  }
  .tab-bar button.active {
    background: #2563eb;
    border-color: #2563eb;
    color: #fff;
  }
  .form-row {
    margin-bottom: 0.75rem;
    display: flex;
    flex-direction: column;
    gap: 0.35rem;
  }
  input, select, textarea {
    background: #0f172a;
    border: 1px solid #374151;
    border-radius: 8px;
    color: #f3f4f6;
    padding: 0.5rem;
  }
  textarea {
    min-height: 80px;
    resize: vertical;
  }
  .checkbox {
    flex-direction: row;
    align-items: center;
  }
  .hint {
    color: #9ca3af;
    font-size: 0.9rem;
    margin-bottom: 0.5rem;
  }
  .network-list {
    max-height: 200px;
    overflow: auto;
    margin-bottom: 0.75rem;
  }
  .qr-section {
    margin-top: 1rem;
    text-align: center;
  }
  .qr-section img {
    max-width: 200px;
    border: 1px solid #374151;
    border-radius: 8px;
    background: #fff;
    padding: 0.5rem;
  }
  .qr-section code {
    display: block;
    margin-top: 0.5rem;
    word-break: break-all;
    background: #0f172a;
    padding: 0.5rem;
    border-radius: 6px;
  }
  details.advanced {
    margin: 0.75rem 0;
    background: #111827;
    border-radius: 10px;
    padding: 0.75rem;
  }
  details.advanced summary {
    cursor: pointer;
    font-weight: 600;
    margin: -0.75rem -0.75rem 0.5rem;
    padding: 0.75rem;
  }
  .advanced-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
    gap: 0.75rem 1rem;
    margin-bottom: 0.5rem;
  }
  .checkbox-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
    gap: 0.35rem 1rem;
  }
  .checkbox-grid label {
    display: flex;
    align-items: center;
    gap: 0.35rem;
  }
  .sub-card {
    background: #111827;
    border-radius: 10px;
    padding: 0.75rem;
    margin-bottom: 1rem;
  }
  .sub-card h3 {
    margin-top: 0;
    margin-bottom: 0.75rem;
  }
</style>
