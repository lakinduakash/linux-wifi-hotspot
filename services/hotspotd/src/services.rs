use anyhow::{Context, Result};
use serde::Serialize;
use std::str::FromStr;
use tokio::process::Command;

#[derive(Clone, Debug)]
pub struct ManagedService {
    pub id: String,
    pub unit: String,
    pub description: Option<String>,
}

#[derive(Clone, Debug)]
pub struct ManagedServiceArg(pub ManagedService);

impl From<ManagedServiceArg> for ManagedService {
    fn from(value: ManagedServiceArg) -> Self {
        value.0
    }
}

impl FromStr for ManagedServiceArg {
    type Err = String;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let mut parts = s.splitn(3, ':');
        let id = parts
            .next()
            .ok_or_else(|| "missing service id".to_string())?
            .trim();
        let unit = parts
            .next()
            .ok_or_else(|| "missing systemd unit name".to_string())?
            .trim();
        let desc = parts
            .next()
            .map(|v| v.trim().to_string())
            .filter(|v| !v.is_empty());
        if id.is_empty() || unit.is_empty() {
            return Err("format must be id:systemd-unit[:description]".to_string());
        }
        Ok(Self(ManagedService {
            id: id.to_string(),
            unit: unit.to_string(),
            description: desc,
        }))
    }
}

#[derive(Debug, Serialize)]
pub struct ManagedServiceStatus {
    pub id: String,
    pub unit: String,
    pub description: Option<String>,
    pub active: bool,
    pub enabled: bool,
}

#[derive(Debug, Clone, Copy)]
pub enum ServiceAction {
    Start,
    Stop,
    Restart,
}

impl ServiceAction {
    pub fn from_str(action: &str) -> Option<Self> {
        match action {
            "start" => Some(ServiceAction::Start),
            "stop" => Some(ServiceAction::Stop),
            "restart" => Some(ServiceAction::Restart),
            _ => None,
        }
    }
}

pub async fn list_status(services: &[ManagedService]) -> Result<Vec<ManagedServiceStatus>> {
    let mut statuses = Vec::with_capacity(services.len());
    for service in services {
        let active = systemctl_success(&["is-active", "--quiet", &service.unit]).await?;
        let enabled = systemctl_success(&["is-enabled", "--quiet", &service.unit]).await?;
        statuses.push(ManagedServiceStatus {
            id: service.id.clone(),
            unit: service.unit.clone(),
            description: service.description.clone(),
            active,
            enabled,
        });
    }
    Ok(statuses)
}

pub async fn run_action(service: &ManagedService, action: ServiceAction) -> Result<()> {
    let args = match action {
        ServiceAction::Start => vec!["start", &service.unit],
        ServiceAction::Stop => vec!["stop", &service.unit],
        ServiceAction::Restart => vec!["restart", &service.unit],
    };
    let status = Command::new("systemctl")
        .args(args)
        .status()
        .await
        .with_context(|| format!("failed to run systemctl on {}", service.unit))?;
    if !status.success() {
        anyhow::bail!(
            "systemctl command failed for {} ({:?})",
            service.unit,
            action
        );
    }
    Ok(())
}

async fn systemctl_success(args: &[&str]) -> Result<bool> {
    let status = Command::new("systemctl")
        .args(args)
        .status()
        .await
        .with_context(|| format!("failed to run systemctl {:?}", args))?;
    Ok(status.success())
}
