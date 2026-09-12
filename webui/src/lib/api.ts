import axios from 'axios';

export interface ApiConfig {
  baseURL?: string;
  token?: string;
  socketPath?: string; // reserved for axios adapters that support unix sockets
}

export class HotspotApi {
  private client;
  private token?: string;

  constructor(config: ApiConfig = {}) {
    this.token = config.token || import.meta.env.VITE_API_TOKEN;
    this.client = axios.create({
      baseURL: config.baseURL || import.meta.env.VITE_API_BASE || 'http://localhost:8085',
      timeout: 5000,
    });
    this.client.interceptors.request.use((req) => {
      if (this.token) {
        req.headers = req.headers || {};
        req.headers['Authorization'] = `Bearer ${this.token}`;
      }
      return req;
    });
  }

  setToken(token: string) {
    this.token = token;
  }

  async health() {
    return this.client.get('/healthz').then((r) => r.data);
  }

  async hotspotStatus() {
    return this.client.get('/hotspot/status').then((r) => r.data);
  }

  async hotspotClients(iface?: string) {
    return this.client
      .get('/hotspot/clients', { params: iface ? { iface } : undefined })
      .then((r) => r.data);
  }

  async hotspotLogs(lines?: number) {
    return this.client
      .get('/hotspot/logs', { params: lines ? { lines } : undefined })
      .then((r) => r.data);
  }

  async getHotspotSettings() {
    return this.client.get('/hotspot/settings').then((r) => r.data);
  }

  async updateHotspotSettings(payload: Record<string, any>) {
    return this.client.put('/hotspot/settings', payload).then((r) => r.data);
  }

  async hotspotStart() {
    return this.client.post('/hotspot/start', {}).then((r) => r.data);
  }

  async hotspotStop() {
    return this.client.post('/hotspot/stop', {}).then((r) => r.data);
  }

  async hotspotEnable() {
    return this.client.post('/hotspot/enable', {}).then((r) => r.data);
  }

  async hotspotDisable() {
    return this.client.post('/hotspot/disable', {}).then((r) => r.data);
  }

  async nmStatus() {
    return this.client.get('/nm/status').then((r) => r.data);
  }

  async listServices() {
    return this.client.get('/services').then((r) => r.data);
  }

  async runServiceAction(id: string, action: 'start' | 'stop' | 'restart') {
    return this.client.post(`/services/${id}/${action}`).then((r) => r.data);
  }

  async listConnections() {
    return this.client.get('/nm/connections').then((r) => r.data);
  }

  async createConnection(payload: {
    name: string;
    type: string;
    iface: string;
    ssid?: string;
    password?: string;
    autoconnect?: boolean;
  }) {
    return this.client.post('/nm/connections', payload).then((r) => r.data);
  }

  async connectionDetail(id: string) {
    return this.client.get(`/nm/connections/${id}/detail`).then((r) => r.data);
  }

  async deleteConnection(id: string) {
    return this.client.delete(`/nm/connections/${id}`).then((r) => r.data);
  }

  async activateConnection(id: string) {
    return this.client.post(`/nm/connections/${id}/activate`, {}).then((r) => r.data);
  }

  async updateConnectionIpv4(
    id: string,
    payload: { method: string; address?: string; gateway?: string; dns?: string }
  ) {
    const body: any = { method: payload.method };
    if (payload.address) body.address = payload.address;
    if (payload.gateway) body.gateway = payload.gateway;
    if (payload.dns !== undefined) {
      body.dns = payload.dns
        .split(',')
        .map((s) => s.trim())
        .filter((s) => !!s);
    }
    return this.client.put(`/nm/connections/${id}/ipv4`, body).then((r) => r.data);
  }

  async wifiConnect(payload: { iface: string; ssid: string; password?: string; autoconnect?: boolean }) {
    return this.client.post('/nm/wifi/connect', payload).then((r) => r.data);
  }

  async wifiScan(iface: string) {
    return this.client.post('/nm/scan', { iface }).then((r) => r.data);
  }

  async disconnectDevice(iface: string) {
    return this.client.post('/nm/disconnect', { iface }).then((r) => r.data);
  }
}

export const api = new HotspotApi();
