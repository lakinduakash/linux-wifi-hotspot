use crate::{auth, config, nm, services, state::AppState};
use anyhow::Result;
use axum::{
    extract::{Path, Query, State},
    middleware,
    routing::{delete, get, post, put},
    Json, Router,
};
use hyper_util::{
    rt::{TokioExecutor, TokioIo},
    server::conn::auto::Builder as HyperBuilder,
    service::TowerToHyperService,
};
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;
use std::{net::SocketAddr, sync::Arc};
use tokio::net::{TcpListener, UnixListener};
use tower_http::{cors::CorsLayer, trace::TraceLayer};
use tracing::{info, warn};

#[derive(Serialize)]
struct HealthResponse {
    status: &'static str,
}

#[derive(Serialize)]
struct HotspotSettingsPayload {
    wifi_iface: Option<String>,
    internet_iface: Option<String>,
    freq_band: Option<String>,
    ssid: Option<String>,
    passphrase: Option<String>,
    channel: Option<String>,
    wpa_version: Option<String>,
    hidden: Option<bool>,
    open_network: bool,
    no_virtual: bool,
    share_method: Option<String>,
    gateway: Option<String>,
    dhcp_dns: Option<String>,
    driver: Option<String>,
    country: Option<String>,
    isolate_clients: bool,
    no_dns: bool,
    no_dnsmasq: bool,
    no_haveged: bool,
    ieee80211n: bool,
    ieee80211ac: bool,
    ieee80211ax: bool,
    ht_capab: Option<String>,
    vht_capab: Option<String>,
    new_macaddr: Option<String>,
    dhcp_hosts: Option<String>,
}

pub async fn serve(state: &AppState, http_listen: Option<String>) -> Result<()> {
    let router = Router::new()
        .route("/healthz", get(health))
        .route("/config", get(get_config).put(update_config))
        .route(
            "/hotspot/settings",
            get(get_hotspot_settings).put(update_hotspot_settings),
        )
        .route("/hotspot/status", get(hotspot_status))
        .route("/hotspot/clients", get(hotspot_clients))
        .route("/hotspot/logs", get(hotspot_logs))
        .route("/hotspot/enable", post(enable_hotspot))
        .route("/hotspot/disable", post(disable_hotspot))
        .route("/hotspot/start", post(start_hotspot))
        .route("/hotspot/stop", post(stop_hotspot))
        .route(
            "/nm/connections",
            get(list_nm_connections)
                .delete(delete_all_nm_connections)
                .post(create_nm_connection),
        )
        .route("/nm/connections/:id", delete(delete_nm_connection))
        .route("/nm/connections/:id/activate", post(activate_nm_connection))
        .route("/nm/connections/:id/detail", get(connection_detail))
        .route("/nm/connections/:id/ipv4", put(update_connection_ipv4))
        .route("/nm/status", get(nm_status))
        .route("/nm/devices", get(list_nm_devices))
        .route("/nm/scan", post(scan_nm))
        .route("/nm/connect", post(connect_nm))
        .route("/nm/disconnect", post(disconnect_nm))
        .route("/nm/wifi/connect", post(connect_wifi_client))
        .route("/services", get(list_services))
        .route("/services/:id/:action", post(manage_service))
        .with_state(state.clone())
        .layer(middleware::from_fn_with_state(
            state.clone(),
            auth::enforce_token,
        ))
        .layer(CorsLayer::permissive())
        .layer(TraceLayer::new_for_http());

    if let Some(addr) = http_listen {
        let addr: SocketAddr = addr.parse().expect("invalid listen addr");
        info!("hotspotd listening on {addr}");
        let listener: TcpListener = TcpListener::bind(addr).await?;
        axum::serve(listener, router).await?;
    } else {
        let sock_path = state.runtime_dir.join("hotspotd.sock");
        if sock_path.exists() {
            tokio::fs::remove_file(&sock_path).await?;
        }
        info!("hotspotd listening on unix://{}", sock_path.display());
        let listener = UnixListener::bind(&sock_path)?;
        serve_unix(listener, router).await?;
    }

    Ok(())
}

async fn health() -> Json<HealthResponse> {
    Json(HealthResponse { status: "ok" })
}

#[derive(Debug, Deserialize)]
struct UpdateRequest {
    entries: BTreeMap<String, String>,
    #[serde(default)]
    restart: bool,
}

#[derive(Debug, Deserialize)]
struct HotspotSettingsRequest {
    wifi_iface: Option<String>,
    internet_iface: Option<String>,
    freq_band: Option<String>,
    ssid: Option<String>,
    passphrase: Option<String>,
    channel: Option<StringOrNumber>,
    wpa_version: Option<String>,
    hidden: Option<bool>,
    open_network: Option<bool>,
    no_virtual: Option<bool>,
    share_method: Option<String>,
    gateway: Option<String>,
    dhcp_dns: Option<String>,
    driver: Option<String>,
    country: Option<String>,
    isolate_clients: Option<bool>,
    no_dns: Option<bool>,
    no_dnsmasq: Option<bool>,
    no_haveged: Option<bool>,
    ieee80211n: Option<bool>,
    ieee80211ac: Option<bool>,
    ieee80211ax: Option<bool>,
    ht_capab: Option<String>,
    vht_capab: Option<String>,
    new_macaddr: Option<String>,
    dhcp_hosts: Option<String>,
    #[serde(default)]
    restart: bool,
}

#[derive(Debug, Deserialize)]
struct NmScanRequest {
    iface: String,
}

#[derive(Debug, Deserialize)]
struct NmConnectRequest {
    iface: String,
    ssid: String,
    password: Option<String>,
}

#[derive(Debug, Deserialize)]
struct NmConnectionCreateRequest {
    name: String,
    #[serde(rename = "type")]
    conn_type: String,
    iface: String,
    ssid: Option<String>,
    password: Option<String>,
    #[serde(default)]
    autoconnect: bool,
    #[serde(default)]
    hidden: bool,
}

#[derive(Debug, Deserialize)]
struct NmDisconnectRequest {
    iface: String,
}

#[derive(Debug, Deserialize, Default)]
struct ClientsQuery {
    iface: Option<String>,
}

#[derive(Debug, Deserialize, Default)]
struct LogsQuery {
    lines: Option<u32>,
}

#[derive(Debug, Deserialize)]
struct Ipv4UpdateRequest {
    method: String,
    address: Option<String>,
    gateway: Option<String>,
    dns: Option<Vec<String>>,
}

#[derive(Debug, Deserialize)]
struct WifiConnectRequest {
    iface: String,
    ssid: String,
    password: Option<String>,
    #[serde(default)]
    autoconnect: bool,
}

async fn get_config(State(state): State<AppState>) -> Json<serde_json::Value> {
    let config = state.config.read().await;
    Json(serde_json::json!({ "entries": config.entries }))
}

async fn get_hotspot_settings(State(state): State<AppState>) -> Json<serde_json::Value> {
    let config = state.config.read().await;
    let payload = HotspotSettingsPayload {
        wifi_iface: config.get("WIFI_IFACE").cloned(),
        internet_iface: config.get("INTERNET_IFACE").cloned(),
        freq_band: config.get("FREQ_BAND").cloned(),
        ssid: config.get("SSID").cloned(),
        passphrase: config.get("PASSPHRASE").cloned(),
        channel: config.get("CHANNEL").cloned(),
        wpa_version: config.get("WPA_VERSION").cloned(),
        hidden: config.get("HIDDEN").map(|v| v == "1"),
        open_network: config
            .get("PASSPHRASE")
            .map(|v| v.is_empty())
            .unwrap_or(true),
        no_virtual: config.get("NO_VIRT").map(|v| v == "1").unwrap_or(false),
        share_method: config.get("SHARE_METHOD").cloned(),
        gateway: config.get("GATEWAY").cloned(),
        dhcp_dns: config.get("DHCP_DNS").cloned(),
        driver: config.get("DRIVER").cloned(),
        country: config.get("COUNTRY").cloned(),
        isolate_clients: config_bool(config.get("ISOLATE_CLIENTS")),
        no_dns: config_bool(config.get("NO_DNS")),
        no_dnsmasq: config_bool(config.get("NO_DNSMASQ")),
        no_haveged: config_bool(config.get("NO_HAVEGED")),
        ieee80211n: config_bool(config.get("IEEE80211N")),
        ieee80211ac: config_bool(config.get("IEEE80211AC")),
        ieee80211ax: config_bool(config.get("IEEE80211AX")),
        ht_capab: config.get("HT_CAPAB").cloned(),
        vht_capab: config.get("VHT_CAPAB").cloned(),
        new_macaddr: config.get("NEW_MACADDR").cloned(),
        dhcp_hosts: config.get("DHCP_HOSTS").cloned(),
    };
    Json(serde_json::json!({ "status": "ok", "settings": payload }))
}

async fn update_config(
    State(state): State<AppState>,
    Json(payload): Json<UpdateRequest>,
) -> Json<serde_json::Value> {
    let mut guard = state.config.write().await;
    let mut updated = guard.entries.clone();
    for (k, v) in payload.entries {
        updated.insert(k, v);
    }
    if let Err(err) = config::validate_entries(&updated) {
        warn!("config validation failed: {err:?}");
        return Json(serde_json::json!({ "status": "error", "message": err.to_string() }));
    }

    guard.entries = updated;
    let wifi_iface = guard.entries.get("WIFI_IFACE").cloned();
    if let Err(err) = guard.write(&state.config_path).await {
        warn!("failed to write config: {err:?}");
        return Json(serde_json::json!({ "status": "error", "message": err.to_string() }));
    }

    drop(guard);

    if payload.restart {
        if let Err(err) = crate::hotspot::restart(wifi_iface.as_deref()).await {
            warn!("unable to restart hotspot after config update: {err:?}");
            return Json(
                serde_json::json!({ "status": "error", "message": format!("config saved but restart failed: {err}") }),
            );
        }
    }

    Json(serde_json::json!({ "status": "ok" }))
}

async fn update_hotspot_settings(
    State(state): State<AppState>,
    Json(payload): Json<HotspotSettingsRequest>,
) -> Json<serde_json::Value> {
    let mut cfg = state.config.write().await;

    if let Some(iface) = payload.wifi_iface {
        if iface.trim().is_empty() {
            cfg.delete("WIFI_IFACE");
        } else {
            cfg.set("WIFI_IFACE", iface.trim().to_string());
        }
    }
    if let Some(internet) = payload.internet_iface {
        if internet.trim().is_empty() {
            cfg.delete("INTERNET_IFACE");
        } else {
            cfg.set("INTERNET_IFACE", internet.trim().to_string());
        }
    }
    if let Some(freq) = payload.freq_band {
        if freq.trim().is_empty() {
            cfg.delete("FREQ_BAND");
        } else {
            cfg.set("FREQ_BAND", freq.trim().to_string());
        }
    }

    if let Some(ssid) = payload.ssid {
        if ssid.trim().is_empty() {
            cfg.delete("SSID");
        } else {
            cfg.set("SSID", ssid.trim().to_string());
        }
    }

    if let Some(pass) = payload.passphrase.as_ref() {
        if pass.trim().is_empty() {
            cfg.delete("PASSPHRASE");
        } else {
            cfg.set("PASSPHRASE", pass.trim().to_string());
        }
    }

    if let Some(channel) = payload.channel {
        let channel = channel.into_string();
        if channel.trim().is_empty() {
            cfg.delete("CHANNEL");
        } else {
            cfg.set("CHANNEL", channel.trim().to_string());
        }
    }

    if let Some(hidden) = payload.hidden {
        cfg.set("HIDDEN", if hidden { "1" } else { "0" }.to_string());
    }

    set_string_entry(&mut cfg, "SHARE_METHOD", payload.share_method);
    set_string_entry(&mut cfg, "GATEWAY", payload.gateway);
    set_string_entry(&mut cfg, "DHCP_DNS", payload.dhcp_dns);
    set_string_entry(&mut cfg, "DRIVER", payload.driver);
    set_string_entry(&mut cfg, "COUNTRY", payload.country);
    set_string_entry(&mut cfg, "HT_CAPAB", payload.ht_capab);
    set_string_entry(&mut cfg, "VHT_CAPAB", payload.vht_capab);
    set_string_entry(&mut cfg, "NEW_MACADDR", payload.new_macaddr);
    set_string_entry(&mut cfg, "DHCP_HOSTS", payload.dhcp_hosts);
    set_bool_entry(&mut cfg, "ISOLATE_CLIENTS", payload.isolate_clients);
    set_bool_entry(&mut cfg, "NO_DNS", payload.no_dns);
    set_bool_entry(&mut cfg, "NO_DNSMASQ", payload.no_dnsmasq);
    set_bool_entry(&mut cfg, "NO_HAVEGED", payload.no_haveged);
    set_bool_entry(&mut cfg, "IEEE80211N", payload.ieee80211n);
    set_bool_entry(&mut cfg, "IEEE80211AC", payload.ieee80211ac);
    set_bool_entry(&mut cfg, "IEEE80211AX", payload.ieee80211ax);

    let mut open_network = cfg.get("PASSPHRASE").map(|v| v.is_empty()).unwrap_or(true);
    if let Some(open) = payload.open_network {
        open_network = open;
    }

    if open_network {
        cfg.set("PASSPHRASE", String::new());
        cfg.delete("WPA_VERSION");
    } else {
        if let Some(pass) = payload.passphrase.as_ref() {
            cfg.set("PASSPHRASE", pass.trim().to_string());
        }
        if let Some(wpa) = payload.wpa_version {
            if wpa.trim().is_empty() {
                cfg.delete("WPA_VERSION");
            } else {
                cfg.set("WPA_VERSION", wpa.trim().to_string());
            }
        }
    }

    if !open_network {
        let pass_len = cfg.get("PASSPHRASE").map(|p| p.len()).unwrap_or(0);
        if pass_len < 8 {
            return Json(
                serde_json::json!({ "status": "error", "message": "Passphrase must be 8-63 characters" }),
            );
        }
        if cfg.get("WPA_VERSION").is_none() {
            cfg.set("WPA_VERSION", "2".to_string());
        }
    }

    if let Some(no_virt) = payload.no_virtual {
        cfg.set("NO_VIRT", if no_virt { "1" } else { "0" }.to_string());
    }

    if let Err(err) = config::validate_entries(&cfg.entries) {
        warn!("hotspot settings validation failed: {err:?}");
        return Json(serde_json::json!({ "status": "error", "message": err.to_string() }));
    }

    let wifi_iface = cfg.get("WIFI_IFACE").cloned();

    if let Err(err) = cfg.write(&state.config_path).await {
        warn!("failed to persist hotspot settings: {err:?}");
        return Json(serde_json::json!({ "status": "error", "message": err.to_string() }));
    }

    drop(cfg);

    if payload.restart {
        if let Err(err) = crate::hotspot::restart(wifi_iface.as_deref()).await {
            warn!("failed to restart hotspot after settings update: {err:?}");
            return Json(serde_json::json!({ "status": "error", "message": err.to_string() }));
        }
    }

    Json(serde_json::json!({ "status": "ok" }))
}

async fn start_hotspot(State(state): State<AppState>) -> Json<serde_json::Value> {
    if let Err(err) = crate::hotspot::start().await {
        warn!("failed to start hotspot: {err:?}");
        if let Some(iface) = current_wifi_iface(&state).await {
            if let Err(clean_err) = crate::hotspot::cleanup_virtual_ifaces(&iface).await {
                warn!("failed to cleanup virtual ifaces after start error: {clean_err:?}");
            }
        }
        return Json(serde_json::json!({ "status": "error", "message": err.to_string() }));
    }
    Json(serde_json::json!({ "status": "ok" }))
}

async fn stop_hotspot(State(state): State<AppState>) -> Json<serde_json::Value> {
    let wifi_iface = current_wifi_iface(&state).await;
    if let Err(err) = crate::hotspot::stop().await {
        warn!("failed to stop hotspot: {err:?}");
        return Json(serde_json::json!({ "status": "error", "message": err.to_string() }));
    }
    if let Some(iface) = wifi_iface {
        if let Err(err) = crate::hotspot::cleanup_virtual_ifaces(&iface).await {
            warn!("failed to cleanup virtual ifaces after stop: {err:?}");
        }
    }
    Json(serde_json::json!({ "status": "ok" }))
}

async fn enable_hotspot(State(state): State<AppState>) -> Json<serde_json::Value> {
    if let Err(err) = crate::hotspot::enable_on_boot().await {
        warn!("failed to enable hotspot: {err:?}");
        if let Some(iface) = current_wifi_iface(&state).await {
            if let Err(clean_err) = crate::hotspot::cleanup_virtual_ifaces(&iface).await {
                warn!("failed to cleanup virtual ifaces after enable error: {clean_err:?}");
            }
        }
        return Json(serde_json::json!({ "status": "error", "message": err.to_string() }));
    }
    Json(serde_json::json!({ "status": "ok" }))
}

async fn disable_hotspot(State(state): State<AppState>) -> Json<serde_json::Value> {
    let wifi_iface = current_wifi_iface(&state).await;
    if let Err(err) = crate::hotspot::disable_on_boot().await {
        warn!("failed to disable hotspot: {err:?}");
        return Json(serde_json::json!({ "status": "error", "message": err.to_string() }));
    }
    if let Some(iface) = wifi_iface {
        if let Err(err) = crate::hotspot::cleanup_virtual_ifaces(&iface).await {
            warn!("failed to cleanup virtual ifaces after disable: {err:?}");
        }
    }
    Json(serde_json::json!({ "status": "ok" }))
}

async fn hotspot_status() -> Json<serde_json::Value> {
    match crate::hotspot::service_state().await {
        Ok(state) => Json(serde_json::json!({ "status": "ok", "service": state })),
        Err(err) => {
            warn!("failed to query hotspot status: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn hotspot_clients(
    State(state): State<AppState>,
    Query(params): Query<ClientsQuery>,
) -> Json<serde_json::Value> {
    let iface = if let Some(iface) = params.iface {
        iface
    } else {
        let cfg = state.config.read().await;
        if let Some(iface) = cfg.entries.get("WIFI_IFACE") {
            iface.clone()
        } else {
            return Json(serde_json::json!({
                "status":"error",
                "message":"Wi-Fi interface not configured"
            }));
        }
    };

    match crate::hotspot::list_clients(&iface).await {
        Ok(clients) => {
            Json(serde_json::json!({ "status": "ok", "iface": iface, "clients": clients }))
        }
        Err(err) => {
            warn!("failed to list clients on {iface}: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn hotspot_logs(Query(params): Query<LogsQuery>) -> Json<serde_json::Value> {
    let lines = params.lines.unwrap_or(200).clamp(10, 2000);
    match crate::hotspot::tail_logs(lines).await {
        Ok(logs) => Json(serde_json::json!({ "status": "ok", "lines": lines, "logs": logs })),
        Err(err) => {
            warn!("failed to tail logs: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn list_nm_devices() -> Json<serde_json::Value> {
    match nm::list_devices().await {
        Ok(devices) => Json(serde_json::json!({ "devices": devices })),
        Err(err) => {
            warn!("failed to list NM devices: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn scan_nm(Json(req): Json<NmScanRequest>) -> Json<serde_json::Value> {
    match nm::scan(&req.iface).await {
        Ok(networks) => Json(serde_json::json!({ "networks": networks })),
        Err(err) => {
            warn!("failed to scan wifi: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn connect_nm(Json(req): Json<NmConnectRequest>) -> Json<serde_json::Value> {
    match nm::connect(&req.iface, &req.ssid, req.password.as_deref()).await {
        Ok(()) => Json(serde_json::json!({ "status": "ok" })),
        Err(err) => {
            warn!("failed to connect via nm: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn disconnect_nm(Json(req): Json<NmDisconnectRequest>) -> Json<serde_json::Value> {
    match nm::disconnect(&req.iface).await {
        Ok(()) => Json(serde_json::json!({ "status": "ok" })),
        Err(err) => {
            warn!("failed to disconnect via nm: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn list_nm_connections() -> Json<serde_json::Value> {
    match nm::list_connections().await {
        Ok(conns) => Json(serde_json::json!({ "connections": conns })),
        Err(err) => {
            warn!("failed to list NM connections: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn create_nm_connection(
    Json(req): Json<NmConnectionCreateRequest>,
) -> Json<serde_json::Value> {
    match nm::add_connection(
        &req.name,
        &req.iface,
        &req.conn_type,
        req.ssid.as_deref(),
        req.password.as_deref(),
        req.autoconnect,
        req.hidden,
    )
    .await
    {
        Ok(()) => Json(serde_json::json!({ "status": "ok" })),
        Err(err) => {
            warn!("failed to create NM connection {}: {err:?}", req.name);
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn nm_status(State(state): State<AppState>) -> Json<serde_json::Value> {
    let devices = match nm::list_devices().await {
        Ok(devices) => devices,
        Err(err) => {
            warn!("failed to list NM devices: {err:?}");
            return Json(serde_json::json!({ "status": "error", "message": err.to_string() }));
        }
    };

    let connections = match nm::active_connections().await {
        Ok(conns) => conns,
        Err(err) => {
            warn!("failed to list active NM connections: {err:?}");
            return Json(serde_json::json!({ "status": "error", "message": err.to_string() }));
        }
    };

    let mut default_iface = devices
        .iter()
        .find(|d| {
            d.device_type == "ethernet" && d.state.to_ascii_lowercase().starts_with("connected")
        })
        .map(|d| d.name.clone())
        .or_else(|| {
            devices
                .iter()
                .find(|d| {
                    d.device_type == "wifi" && d.state.to_ascii_lowercase().starts_with("connected")
                })
                .map(|d| d.name.clone())
        });

    if default_iface.is_none() {
        if let Some(active_eth) = connections
            .iter()
            .find(|c| c.active && c.conn_type == "802-3-ethernet" && !c.device.is_empty())
        {
            default_iface = Some(active_eth.device.clone());
        } else if let Some(active_wifi) = connections
            .iter()
            .find(|c| c.active && c.conn_type == "802-11-wireless" && !c.device.is_empty())
        {
            default_iface = Some(active_wifi.device.clone());
        }
    }

    if default_iface.is_none() {
        if let Ok(primary) = nm::primary_active_device().await {
            default_iface = primary;
        }
    }

    if default_iface.is_none() {
        if let Ok(auto) = nm::detect_wifi_iface().await {
            default_iface = auto;
        }
    }

    if default_iface.is_none() {
        let cfg_iface = {
            let cfg = state.config.read().await;
            cfg.entries.get("WIFI_IFACE").cloned()
        };

        if let Some(iface) = cfg_iface {
            if devices.iter().any(|d| d.name == iface) {
                default_iface = Some(iface);
            }
        }
    }

    let device_status = if let Some(iface) = default_iface.as_deref() {
        match nm::device_status(iface).await {
            Ok(status) => Some(status),
            Err(err) => {
                warn!("failed to read device status for {iface}: {err:?}");
                None
            }
        }
    } else {
        None
    };

    Json(serde_json::json!({
        "status": "ok",
        "default_iface": default_iface,
        "devices": devices,
        "active_connections": connections,
        "device_status": device_status
    }))
}

async fn delete_nm_connection(Path(id): Path<String>) -> Json<serde_json::Value> {
    match nm::delete_connection(&id).await {
        Ok(()) => Json(serde_json::json!({ "status": "ok" })),
        Err(err) => {
            warn!("failed to delete NM connection {id}: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn delete_all_nm_connections() -> Json<serde_json::Value> {
    match nm::delete_all_wifi_connections().await {
        Ok(()) => Json(serde_json::json!({ "status": "ok" })),
        Err(err) => {
            warn!("failed to delete all wifi NM connections: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn activate_nm_connection(Path(id): Path<String>) -> Json<serde_json::Value> {
    match nm::activate_connection(&id).await {
        Ok(()) => Json(serde_json::json!({ "status": "ok" })),
        Err(err) => {
            warn!("failed to activate NM connection {id}: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn connection_detail(Path(id): Path<String>) -> Json<serde_json::Value> {
    match nm::connection_details(&id).await {
        Ok(details) => Json(serde_json::json!({ "status": "ok", "connection": details })),
        Err(err) => {
            warn!("failed to get connection {id}: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn update_connection_ipv4(
    Path(id): Path<String>,
    Json(req): Json<Ipv4UpdateRequest>,
) -> Json<serde_json::Value> {
    let dns = req.dns.unwrap_or_default();
    match nm::update_ipv4_settings(
        &id,
        &req.method,
        req.address.as_deref(),
        req.gateway.as_deref(),
        &dns,
    )
    .await
    {
        Ok(()) => Json(serde_json::json!({ "status": "ok" })),
        Err(err) => {
            warn!("failed to update ipv4 for {id}: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn connect_wifi_client(Json(req): Json<WifiConnectRequest>) -> Json<serde_json::Value> {
    match nm::connect_wifi(
        &req.iface,
        &req.ssid,
        req.password.as_deref(),
        req.autoconnect,
    )
    .await
    {
        Ok(()) => Json(serde_json::json!({ "status": "ok" })),
        Err(err) => {
            warn!("failed to connect wifi {}: {err:?}", req.ssid);
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn list_services(State(state): State<AppState>) -> Json<serde_json::Value> {
    if state.services.is_empty() {
        return Json(serde_json::json!({ "status": "ok", "services": [] }));
    }
    match services::list_status(state.services.as_ref()).await {
        Ok(statuses) => Json(serde_json::json!({ "status": "ok", "services": statuses })),
        Err(err) => {
            warn!("failed to query managed services: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn manage_service(
    State(state): State<AppState>,
    Path((id, action)): Path<(String, String)>,
) -> Json<serde_json::Value> {
    let Some(service) = state.services.iter().find(|svc| svc.id == id) else {
        return Json(
            serde_json::json!({ "status": "error", "message": format!("unknown service {id}") }),
        );
    };
    let Some(action_enum) = services::ServiceAction::from_str(&action) else {
        return Json(
            serde_json::json!({ "status": "error", "message": "action must be start/stop/restart" }),
        );
    };
    match services::run_action(service, action_enum).await {
        Ok(()) => Json(serde_json::json!({ "status": "ok" })),
        Err(err) => {
            warn!("failed to run action {action} on {id}: {err:?}");
            Json(serde_json::json!({ "status": "error", "message": err.to_string() }))
        }
    }
}

async fn current_wifi_iface(state: &AppState) -> Option<String> {
    let cfg = state.config.read().await;
    cfg.entries.get("WIFI_IFACE").cloned()
}

fn set_string_entry(cfg: &mut config::HotspotConfig, key: &str, value: Option<String>) {
    if let Some(val) = value {
        if val.trim().is_empty() {
            cfg.delete(key);
        } else {
            cfg.set(key, val.trim().to_string());
        }
    }
}

fn set_bool_entry(cfg: &mut config::HotspotConfig, key: &str, value: Option<bool>) {
    if let Some(flag) = value {
        cfg.set(key, if flag { "1" } else { "0" }.to_string());
    }
}

fn config_bool(value: Option<&String>) -> bool {
    matches!(value, Some(v) if v == "1" || v.eq_ignore_ascii_case("true") || v.eq_ignore_ascii_case("yes"))
}

async fn serve_unix(listener: UnixListener, router: Router) -> Result<()> {
    let shared = Arc::new(router);
    loop {
        let (stream, _) = listener.accept().await?;
        let svc = shared.clone();
        tokio::spawn(async move {
            if let Err(err) = HyperBuilder::new(TokioExecutor::new())
                .serve_connection(
                    TokioIo::new(stream),
                    TowerToHyperService::new((*svc).clone()),
                )
                .await
            {
                warn!("error serving unix connection: {err:?}");
            }
        });
    }
}
#[derive(Debug, Deserialize)]
#[serde(untagged)]
enum StringOrNumber {
    Str(String),
    Int(i64),
    Float(f64),
}

impl StringOrNumber {
    fn into_string(self) -> String {
        match self {
            StringOrNumber::Str(s) => s,
            StringOrNumber::Int(i) => i.to_string(),
            StringOrNumber::Float(f) => {
                if f.fract() == 0.0 {
                    (f as i64).to_string()
                } else {
                    f.to_string()
                }
            }
        }
    }
}
