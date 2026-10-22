use anyhow::{Context, Result};
use serde::Serialize;
use tokio::process::Command;
use tracing::{info, warn};

#[derive(Debug, Serialize)]
pub struct ServiceState {
    pub active: bool,
    pub enabled: bool,
}

#[derive(Debug, Serialize)]
pub struct ClientInfo {
    pub mac: String,
    pub ip: Option<String>,
    pub hostname: Option<String>,
}

pub async fn start() -> Result<()> {
    info!("starting create_ap via systemctl");
    run_systemctl(&["start", "create_ap"]).await
}

pub async fn restart(wifi_iface: Option<&str>) -> Result<()> {
    info!("restarting create_ap via systemctl");
    run_systemctl(&["stop", "create_ap"]).await?;
    if let Some(iface) = wifi_iface {
        if let Err(err) = cleanup_virtual_ifaces(iface).await {
            warn!("failed to cleanup virtual ifaces after stop: {err:?}");
        }
    }
    run_systemctl(&["start", "create_ap"]).await
}

pub async fn stop() -> Result<()> {
    info!("stopping create_ap via systemctl");
    run_systemctl(&["stop", "create_ap"]).await
}

pub async fn enable_on_boot() -> Result<()> {
    info!("enabling create_ap on boot");
    run_systemctl(&["enable", "--now", "create_ap"]).await
}

pub async fn disable_on_boot() -> Result<()> {
    info!("disabling create_ap on boot");
    run_systemctl(&["disable", "--now", "create_ap"]).await
}

pub async fn list_clients(iface: &str) -> Result<Vec<ClientInfo>> {
    let output = Command::new("create_ap")
        .args(["--list-clients", iface])
        .output()
        .await
        .with_context(|| format!("failed to list clients on {iface}"))?;
    if !output.status.success() {
        anyhow::bail!(
            "create_ap --list-clients failed: {}",
            String::from_utf8_lossy(&output.stderr)
        );
    }
    let stdout = String::from_utf8_lossy(&output.stdout);
    if stdout.to_lowercase().contains("no clients") {
        return Ok(vec![]);
    }
    let mut clients = Vec::new();
    for line in stdout.lines() {
        if line.starts_with("MAC") || line.trim().is_empty() {
            continue;
        }
        let mut parts = line.split_whitespace();
        let mac = parts.next().unwrap_or_default().to_string();
        if mac.is_empty() {
            continue;
        }
        let ip = parts.next().map(|s| s.to_string()).filter(|s| s != "*");
        let host = parts.next().map(|s| s.to_string()).filter(|s| s != "*");
        clients.push(ClientInfo {
            mac,
            ip,
            hostname: host,
        });
    }
    Ok(clients)
}

pub async fn tail_logs(lines: u32) -> Result<String> {
    let line_arg = format!("-n{}", lines);
    let output = Command::new("journalctl")
        .args([
            "-u",
            "create_ap",
            "--no-pager",
            "--output",
            "short",
            &line_arg,
        ])
        .output()
        .await
        .context("failed to read create_ap logs via journalctl")?;
    if !output.status.success() {
        anyhow::bail!(
            "journalctl failed: {}",
            String::from_utf8_lossy(&output.stderr)
        );
    }
    Ok(String::from_utf8_lossy(&output.stdout).to_string())
}

pub async fn service_state() -> Result<ServiceState> {
    let active = check_systemctl(&["is-active", "--quiet", "create_ap"]).await?;
    let enabled = check_systemctl(&["is-enabled", "--quiet", "create_ap"]).await?;
    Ok(ServiceState { active, enabled })
}

async fn run_systemctl(args: &[&str]) -> Result<()> {
    let status = Command::new("systemctl")
        .args(args)
        .status()
        .await
        .with_context(|| format!("failed to run systemctl {:?}", args))?;
    if !status.success() {
        anyhow::bail!("systemctl {:?} failed with {}", args, status);
    }
    Ok(())
}

async fn check_systemctl(args: &[&str]) -> Result<bool> {
    let status = Command::new("systemctl")
        .args(args)
        .status()
        .await
        .with_context(|| format!("failed to run systemctl {:?}", args))?;
    Ok(status.success())
}

#[derive(Debug, Clone)]
struct IwInterface {
    name: String,
    phy: Option<String>,
    addr: Option<String>,
}

pub async fn cleanup_virtual_ifaces(target_iface: &str) -> Result<()> {
    let interfaces = list_wifi_interfaces().await?;
    let Some(target) = interfaces.iter().find(|iface| iface.name == target_iface) else {
        return Ok(());
    };
    let target_phy = target.phy.clone();
    let target_addr = target.addr.clone();

    for iface in interfaces {
        if iface.name == target_iface {
            continue;
        }
        let same_phy = match (&target_phy, &iface.phy) {
            (Some(a), Some(b)) => a == b,
            _ => false,
        };
        let same_addr = match (&target_addr, &iface.addr) {
            (Some(a), Some(b)) => a.eq_ignore_ascii_case(b),
            _ => false,
        };
        if !(same_phy || same_addr) {
            continue;
        }
        info!("removing stale wifi interface {}", iface.name);
        let status = Command::new("iw")
            .args(["dev", &iface.name, "del"])
            .status()
            .await
            .with_context(|| format!("failed to delete iface {}", iface.name))?;
        if !status.success() {
            warn!("iw dev {} del exited with status {}", iface.name, status);
        }
    }
    Ok(())
}

async fn list_wifi_interfaces() -> Result<Vec<IwInterface>> {
    let output = Command::new("iw")
        .arg("dev")
        .output()
        .await
        .context("failed to run iw dev")?;
    if !output.status.success() {
        anyhow::bail!("iw dev failed: {}", String::from_utf8_lossy(&output.stderr));
    }
    let mut interfaces = Vec::new();
    let mut current_phy: Option<String> = None;
    for line in String::from_utf8_lossy(&output.stdout).lines() {
        let trimmed = line.trim();
        if trimmed.starts_with("phy#") {
            current_phy = Some(trimmed.to_string());
        } else if let Some(rest) = trimmed.strip_prefix("Interface ") {
            interfaces.push(IwInterface {
                name: rest.to_string(),
                phy: current_phy.clone(),
                addr: None,
            });
        } else if let Some(rest) = trimmed.strip_prefix("addr ") {
            if let Some(last) = interfaces.last_mut() {
                last.addr = Some(rest.to_string());
            }
        }
    }
    Ok(interfaces)
}
