//! NetworkManager integration helpers.
//!
//! For the initial implementation we shell out to `nmcli` for simplicity. Later
//! we can replace these helpers with native D-Bus bindings via `zbus`.

use anyhow::{Context, Result};
use serde::Serialize;
use std::{collections::HashSet, process::Output};
use tokio::process::Command;

const NM_SEP: &str = ":";

#[derive(Debug, Clone, Serialize)]
pub struct NmDeviceInfo {
    pub name: String,
    #[serde(rename = "type")]
    pub device_type: String,
    pub state: String,
    pub connection: Option<String>,
}

impl NmDeviceInfo {
    pub fn is_virtual(&self) -> bool {
        matches!(
            self.device_type.as_str(),
            "bridge" | "dummy" | "loopback" | "tun" | "veth"
        ) || self.name.starts_with("docker")
            || self.name.starts_with("br-")
            || self.name.starts_with("veth")
            || self.name == "lo"
    }
}

#[derive(Debug, Clone, Serialize)]
pub struct WifiNetwork {
    pub ssid: String,
    pub strength: u8,
    pub security: String,
    pub active: bool,
}

#[derive(Debug, Clone, Serialize)]
pub struct NmConnection {
    pub name: String,
    pub uuid: String,
    #[serde(rename = "type")]
    pub conn_type: String,
    pub device: String,
    pub active: bool,
}

#[derive(Debug, Clone, Serialize, Default)]
pub struct NmDeviceStatus {
    pub name: String,
    pub state: Option<String>,
    pub connection: Option<String>,
    pub ipv4: Option<String>,
    pub ipv6: Option<String>,
}

#[derive(Debug, Clone, Serialize, Default)]
pub struct NmIpv4Config {
    pub method: String,
    pub addresses: Vec<String>,
    pub gateway: Option<String>,
    pub dns: Vec<String>,
}

#[derive(Debug, Clone, Serialize, Default)]
pub struct NmWifiConfig {
    pub ssid: Option<String>,
    pub security: Option<String>,
}

#[derive(Debug, Clone, Serialize, Default)]
pub struct NmConnectionDetails {
    pub name: String,
    pub uuid: String,
    #[serde(rename = "type")]
    pub conn_type: String,
    pub device: String,
    pub autoconnect: bool,
    pub ipv4: NmIpv4Config,
    pub wifi: Option<NmWifiConfig>,
}

/// List NetworkManager devices (wifi/ethernet/etc.).
pub async fn list_devices() -> Result<Vec<NmDeviceInfo>> {
    let output = Command::new("nmcli")
        .args([
            "-t",
            "-f",
            "DEVICE,TYPE,STATE,CONNECTION",
            "device",
            "status",
        ])
        .output()
        .await
        .context("failed to run nmcli device status")?;
    ensure_success("nmcli device status", &output)?;

    let mut devices = Vec::new();
    for line in String::from_utf8_lossy(&output.stdout).lines() {
        let parts: Vec<_> = line.split(NM_SEP).collect();
        if parts.len() < 3 {
            continue;
        }
        devices.push(NmDeviceInfo {
            name: parts[0].to_string(),
            device_type: parts[1].to_string(),
            state: parts[2].to_string(),
            connection: parts
                .get(3)
                .map(|v| v.to_string())
                .filter(|s| !s.is_empty()),
        });
    }
    Ok(devices.into_iter().filter(|d| !d.is_virtual()).collect())
}

pub async fn detect_wifi_iface() -> Result<Option<String>> {
    let devices = list_devices().await?;
    Ok(devices
        .iter()
        .find(|d| d.device_type == "wifi")
        .map(|d| d.name.clone()))
}

pub async fn primary_active_device() -> Result<Option<String>> {
    let output = Command::new("nmcli")
        .args(["-t", "-f", "DEVICE", "connection", "show", "--active"])
        .output()
        .await
        .context("failed to query active device list")?;
    ensure_success("nmcli connection show --active", &output)?;
    for line in String::from_utf8_lossy(&output.stdout).lines() {
        let device = line.trim();
        if !device.is_empty() && device != "--" {
            return Ok(Some(device.to_string()));
        }
    }
    Ok(None)
}

/// Trigger a Wi-Fi scan for the given interface and return parsed results.
pub async fn scan(iface: &str) -> Result<Vec<WifiNetwork>> {
    let output = Command::new("nmcli")
        .args([
            "-t",
            "-f",
            "ACTIVE,SSID,SECURITY,SIGNAL",
            "device",
            "wifi",
            "list",
            "ifname",
            iface,
        ])
        .output()
        .await
        .with_context(|| format!("failed to run nmcli device wifi list on {iface}"))?;
    ensure_success("nmcli wifi list", &output)?;

    let mut networks = Vec::new();
    for line in String::from_utf8_lossy(&output.stdout).lines() {
        let parts: Vec<_> = line.split(NM_SEP).collect();
        if parts.len() < 4 {
            continue;
        }
        let strength = parts[3].parse::<u8>().unwrap_or_default();
        networks.push(WifiNetwork {
            active: parts[0] == "yes",
            ssid: parts[1].to_string(),
            security: parts[2].to_string(),
            strength,
        });
    }
    Ok(networks)
}

/// Connect the interface to the specified SSID using an optional passphrase.
pub async fn connect(iface: &str, ssid: &str, password: Option<&str>) -> Result<()> {
    let mut cmd = Command::new("nmcli");
    cmd.args(["device", "wifi", "connect", ssid, "ifname", iface]);
    if let Some(pass) = password {
        if !pass.is_empty() {
            cmd.args(["password", pass]);
        }
    }
    let status = cmd
        .status()
        .await
        .with_context(|| format!("failed to run nmcli wifi connect on {iface}"))?;
    if !status.success() {
        anyhow::bail!("nmcli wifi connect returned {}", status);
    }
    Ok(())
}

/// Disconnect the interface from its current NetworkManager connection.
pub async fn disconnect(iface: &str) -> Result<()> {
    let status = Command::new("nmcli")
        .args(["device", "disconnect", iface])
        .status()
        .await
        .with_context(|| format!("failed to run nmcli device disconnect on {iface}"))?;
    if !status.success() {
        anyhow::bail!("nmcli device disconnect returned {}", status);
    }
    Ok(())
}

/// Enumerate stored NetworkManager connections.
pub async fn list_connections() -> Result<Vec<NmConnection>> {
    let active = active_connection_ids().await?;
    let output = Command::new("nmcli")
        .args(["-t", "-f", "NAME,UUID,TYPE,DEVICE", "connection", "show"])
        .output()
        .await
        .context("failed to run nmcli connection show")?;
    ensure_success("nmcli connection show", &output)?;

    let mut connections = Vec::new();
    for line in String::from_utf8_lossy(&output.stdout).lines() {
        let parts: Vec<_> = line.split(NM_SEP).collect();
        if parts.len() < 4 {
            continue;
        }
        connections.push(NmConnection {
            name: parts[0].to_string(),
            uuid: parts[1].to_string(),
            conn_type: parts[2].to_string(),
            device: parts[3].to_string(),
            active: active.contains(parts[1]),
        });
    }
    Ok(connections)
}

pub async fn active_connections() -> Result<Vec<NmConnection>> {
    let connections = list_connections().await?;
    Ok(connections.into_iter().filter(|c| c.active).collect())
}

pub async fn device_status(iface: &str) -> Result<NmDeviceStatus> {
    let output = Command::new("nmcli")
        .args([
            "-t",
            "-f",
            "GENERAL.STATE,GENERAL.CONNECTION,IP4.ADDRESS,IP6.ADDRESS",
            "device",
            "show",
            iface,
        ])
        .output()
        .await
        .with_context(|| format!("failed to run nmcli device show on {iface}"))?;
    ensure_success("nmcli device show", &output)?;
    let mut status = NmDeviceStatus {
        name: iface.to_string(),
        ..Default::default()
    };
    for line in String::from_utf8_lossy(&output.stdout).lines() {
        let mut parts = line.splitn(2, ':');
        let key = parts.next().unwrap_or("").trim();
        let value = parts.next().unwrap_or("").trim();
        if value.is_empty() {
            continue;
        }
        match key {
            "GENERAL.STATE" => status.state = Some(value.to_string()),
            "GENERAL.CONNECTION" => status.connection = Some(value.to_string()),
            "IP4.ADDRESS[1]" | "IP4.ADDRESS" => status.ipv4 = Some(value.to_string()),
            "IP6.ADDRESS[1]" | "IP6.ADDRESS" => status.ipv6 = Some(value.to_string()),
            _ => {}
        }
    }
    Ok(status)
}

/// Delete a NetworkManager connection by UUID or name.
pub async fn delete_connection(id: &str) -> Result<()> {
    let connections = list_connections().await?;
    for conn in &connections {
        if conn.uuid == id {
            return delete_connection_uuid(id).await;
        }
        if conn.name == id {
            return delete_connection_name(id).await;
        }
    }
    delete_connection_name(id).await
}

/// Delete all saved Wi-Fi connections (802-11-wireless).
pub async fn delete_all_wifi_connections() -> Result<()> {
    let connections = list_connections().await?;
    for conn in connections
        .into_iter()
        .filter(|c| c.conn_type == "802-11-wireless")
    {
        delete_connection_uuid(&conn.uuid).await?;
    }
    Ok(())
}

pub async fn connection_details(id: &str) -> Result<NmConnectionDetails> {
    let uuid = resolve_connection_uuid(id).await?;
    let output = Command::new("nmcli")
        .args([
            "-t",
            "-f",
            "connection.id,connection.uuid,connection.type,connection.interface-name,connection.autoconnect,ipv4.method,ipv4.addresses,ipv4.gateway,ipv4.dns,802-11-wireless.ssid,802-11-wireless-security.key-mgmt",
            "connection",
            "show",
            "uuid",
            &uuid,
        ])
        .output()
        .await
        .context("failed to query connection details")?;
    ensure_success("nmcli connection show uuid", &output)?;

    let mut details = NmConnectionDetails {
        uuid: uuid.clone(),
        ..Default::default()
    };
    let mut ipv4 = NmIpv4Config::default();
    let mut wifi = NmWifiConfig::default();
    for line in String::from_utf8_lossy(&output.stdout).lines() {
        let mut parts = line.splitn(2, ':');
        let key = parts.next().unwrap_or("").trim();
        let value = parts.next().unwrap_or("").trim();
        match key {
            "connection.id" => details.name = value.to_string(),
            "connection.uuid" => details.uuid = value.to_string(),
            "connection.type" => details.conn_type = value.to_string(),
            "connection.interface-name" => details.device = value.to_string(),
            "connection.autoconnect" => details.autoconnect = value.eq_ignore_ascii_case("yes"),
            "ipv4.method" => ipv4.method = value.to_string(),
            "ipv4.addresses" => {
                if !value.is_empty() {
                    ipv4.addresses.push(value.to_string());
                }
            }
            "ipv4.gateway" => {
                if !value.is_empty() {
                    ipv4.gateway = Some(value.to_string())
                }
            }
            "ipv4.dns" => {
                if !value.is_empty() {
                    ipv4.dns.push(value.to_string());
                }
            }
            "802-11-wireless.ssid" => {
                wifi.ssid = if value.is_empty() {
                    None
                } else {
                    Some(value.to_string())
                }
            }
            "802-11-wireless-security.key-mgmt" => {
                if !value.is_empty() {
                    wifi.security = Some(value.to_string());
                }
            }
            _ => {}
        }
    }
    details.ipv4 = ipv4;
    if details.conn_type == "802-11-wireless" {
        details.wifi = Some(wifi);
    }
    Ok(details)
}

pub async fn update_ipv4_settings(
    id: &str,
    method: &str,
    address: Option<&str>,
    gateway: Option<&str>,
    dns: &[String],
) -> Result<()> {
    let uuid = resolve_connection_uuid(id).await?;
    let mut args = vec![
        "connection".to_string(),
        "modify".to_string(),
        uuid.clone(),
        "ipv4.method".to_string(),
        method.to_string(),
    ];
    match method {
        "manual" => {
            if let Some(addr) = address {
                args.push("ipv4.addresses".to_string());
                args.push(addr.to_string());
            } else {
                anyhow::bail!("manual method requires ipv4 address");
            }
            if let Some(gw) = gateway {
                args.push("ipv4.gateway".to_string());
                args.push(gw.to_string());
            } else {
                args.push("ipv4.gateway".to_string());
                args.push(String::new());
            }
            if !dns.is_empty() {
                let joined = dns.join(",");
                args.push("ipv4.dns".to_string());
                args.push(joined);
            } else {
                args.push("ipv4.dns".to_string());
                args.push(String::new());
            }
            args.push("ipv4.may-fail".to_string());
            args.push("no".to_string());
        }
        "auto" => {
            args.extend(
                [
                    "ipv4.addresses",
                    "",
                    "ipv4.gateway",
                    "",
                    "ipv4.dns",
                    "",
                    "ipv4.may-fail",
                    "yes",
                ]
                .iter()
                .map(|s| s.to_string()),
            );
        }
        other => {
            anyhow::bail!("unsupported ipv4 method {other}");
        }
    }
    let status = Command::new("nmcli").args(&args).status().await?;
    if !status.success() {
        anyhow::bail!("nmcli connection modify returned {}", status);
    }
    // bring the connection up to apply new settings
    let up_status = Command::new("nmcli")
        .args(["connection", "up", &uuid])
        .status()
        .await?;
    if !up_status.success() {
        anyhow::bail!("nmcli connection up returned {}", up_status);
    }
    Ok(())
}

pub async fn activate_connection(id: &str) -> Result<()> {
    let uuid = resolve_connection_uuid(id).await?;
    let status = Command::new("nmcli")
        .args(["connection", "up", &uuid])
        .status()
        .await
        .with_context(|| format!("failed to activate connection {uuid}"))?;
    if !status.success() {
        anyhow::bail!("nmcli connection up returned {}", status);
    }
    Ok(())
}

pub async fn add_connection(
    name: &str,
    iface: &str,
    conn_type: &str,
    ssid: Option<&str>,
    password: Option<&str>,
    autoconnect: bool,
    hidden: bool,
) -> Result<()> {
    let mut args = vec![
        "connection".to_string(),
        "add".to_string(),
        "type".to_string(),
        conn_type.to_string(),
        "ifname".to_string(),
        iface.to_string(),
        "con-name".to_string(),
        name.to_string(),
    ];
    if conn_type == "802-11-wireless" {
        let Some(ssid_val) = ssid else {
            anyhow::bail!("ssid is required for wifi connections");
        };
        args.push("ssid".to_string());
        args.push(ssid_val.to_string());
    }
    let output = Command::new("nmcli").args(&args).output().await?;
    ensure_success("nmcli connection add", &output)?;
    if conn_type == "802-11-wireless" {
        let hidden_flag = if hidden { "yes" } else { "no" };
        let output_hidden = Command::new("nmcli")
            .args(["connection", "modify", name, "wifi.hidden", hidden_flag])
            .output()
            .await
            .context("failed to mark wifi connection hidden")?;
        ensure_success("nmcli connection modify wifi.hidden", &output_hidden)?;
        match password {
            Some(pass) if !pass.is_empty() => {
                let output_pw = Command::new("nmcli")
                    .args([
                        "connection",
                        "modify",
                        name,
                        "wifi-sec.key-mgmt",
                        "wpa-psk",
                        "wifi-sec.psk",
                        pass,
                    ])
                    .output()
                    .await
                    .context("failed to set wifi password")?;
                ensure_success("nmcli connection modify wifi-sec.psk", &output_pw)?;
            }
            _ => {
                let output_clear = Command::new("nmcli")
                    .args(["connection", "modify", name, "wifi-sec.key-mgmt", "none"])
                    .output()
                    .await
                    .context("failed to clear wifi key management")?;
                ensure_success("nmcli connection modify wifi-sec.key-mgmt", &output_clear)?;
            }
        }
    }
    let auto_output = Command::new("nmcli")
        .args([
            "connection",
            "modify",
            name,
            "autoconnect",
            if autoconnect { "yes" } else { "no" },
        ])
        .output()
        .await
        .context("failed to set connection autoconnect")?;
    ensure_success("nmcli connection modify autoconnect", &auto_output)?;
    Ok(())
}

pub async fn connect_wifi(
    iface: &str,
    ssid: &str,
    password: Option<&str>,
    autoconnect: bool,
) -> Result<()> {
    let mut cmd = Command::new("nmcli");
    cmd.args(["device", "wifi", "connect", ssid, "ifname", iface]);
    if let Some(pass) = password {
        if !pass.is_empty() {
            cmd.args(["password", pass]);
        }
    }
    let status = cmd.status().await?;
    if !status.success() {
        anyhow::bail!("nmcli wifi connect returned {}", status);
    }
    if !autoconnect {
        // turn off autoconnect on the newly created/updated profile
        let output = Command::new("nmcli")
            .args(["connection", "modify", ssid, "autoconnect", "no"])
            .output()
            .await
            .context("failed to disable autoconnect on wifi profile")?;
        ensure_success("nmcli connection modify autoconnect", &output)?;
    }
    Ok(())
}

async fn delete_connection_uuid(uuid: &str) -> Result<()> {
    let status = Command::new("nmcli")
        .args(["connection", "delete", "uuid", uuid])
        .status()
        .await
        .with_context(|| format!("failed to delete connection {uuid} by uuid"))?;
    if !status.success() {
        anyhow::bail!("nmcli connection delete uuid returned {}", status);
    }
    Ok(())
}

async fn delete_connection_name(name: &str) -> Result<()> {
    let status = Command::new("nmcli")
        .args(["connection", "delete", name])
        .status()
        .await
        .with_context(|| format!("failed to delete connection {name} by name"))?;
    if !status.success() {
        anyhow::bail!("nmcli connection delete by name returned {}", status);
    }
    Ok(())
}

async fn active_connection_ids() -> Result<HashSet<String>> {
    let output = Command::new("nmcli")
        .args(["-t", "-f", "UUID", "connection", "show", "--active"])
        .output()
        .await
        .context("failed to query active connections")?;
    ensure_success("nmcli connection show --active", &output)?;

    let mut set = HashSet::new();
    for line in String::from_utf8_lossy(&output.stdout).lines() {
        if !line.trim().is_empty() {
            set.insert(line.trim().to_string());
        }
    }
    Ok(set)
}

fn ensure_success(cmd: &str, output: &Output) -> Result<()> {
    if !output.status.success() {
        anyhow::bail!("{cmd} failed: {}", String::from_utf8_lossy(&output.stderr));
    }
    Ok(())
}

async fn resolve_connection_uuid(id: &str) -> Result<String> {
    let connections = list_connections().await?;
    if let Some(conn) = connections.iter().find(|c| c.uuid == id || c.name == id) {
        Ok(conn.uuid.clone())
    } else {
        anyhow::bail!("unknown connection id {id}")
    }
}
