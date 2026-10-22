<script lang="ts">
  let hotspotStatus = { service: { active: true, enabled: true } };
  let clients = [
    { mac: 'AA:BB:CC:DD:EE:FF', ip: '192.168.12.42', hostname: 'device-01' },
    { mac: '11:22:33:44:55:66', ip: '192.168.12.43', hostname: 'tablet' }
  ];
  let logs = 'Jan 12 10:22:01 host create_ap[1234]: AP started\nJan 12 10:22:03 host ...';
  let nmStatus = { default_iface: 'wlan0', active_connections: [{ name: 'OfficeWiFi' }] };
  let services = [
    { id: 'dragonsync', unit: 'dragonsync.service', description: 'DragonSync OTA', active: true }
  ];
</script>

<div class="page">
  <h1>Mock Dashboard</h1>
  <pre>{JSON.stringify({ hotspotStatus, clients, logs, nmStatus, services }, null, 2)}</pre>
</div>
