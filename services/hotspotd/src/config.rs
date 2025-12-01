use anyhow::{ensure, Context, Result};
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;
use std::os::unix::fs::OpenOptionsExt;
use std::path::Path;
use tokio::fs;
use tokio::io::AsyncWriteExt;

const DEFAULT_TEMPLATE: &str = include_str!("../assets/create_ap.conf");

#[derive(Debug, Clone, Serialize, Deserialize, Default)]
pub struct HotspotConfig {
    pub entries: BTreeMap<String, String>,
}

#[derive(Debug, Clone, Default)]
pub struct ConfigDefaults {
    entries: BTreeMap<String, String>,
}

impl ConfigDefaults {
    pub fn from_template() -> Result<Self> {
        let entries = parse_ini(DEFAULT_TEMPLATE)?;
        Ok(Self { entries })
    }

    pub fn override_entry(mut self, key: &str, value: Option<String>) -> Self {
        if let Some(v) = value {
            self.entries.insert(key.to_string(), v);
        }
        self
    }

    pub fn entries(&self) -> &BTreeMap<String, String> {
        &self.entries
    }
}

impl HotspotConfig {
    pub async fn load(path: &Path) -> Result<Self> {
        let contents = fs::read_to_string(path)
            .await
            .with_context(|| format!("failed to read config {}", path.display()))?;
        let entries = parse_ini(&contents)?;
        Ok(Self { entries })
    }

    pub async fn load_or_init(path: &Path, defaults: &ConfigDefaults) -> Result<Self> {
        if tokio::fs::try_exists(path).await? {
            return Self::load(path).await;
        }
        if let Some(parent) = path.parent() {
            fs::create_dir_all(parent).await?;
        }
        let config = HotspotConfig {
            entries: defaults.entries().clone(),
        };
        config.write(path).await?;
        Ok(config)
    }

    pub async fn write(&self, path: &Path) -> Result<()> {
        let mut buf = String::new();
        for (k, v) in &self.entries {
            buf.push_str(&format!("{k}={v}\n"));
        }
        if let Some(parent) = path.parent() {
            fs::create_dir_all(parent).await?;
        }

        let mut file = fs::OpenOptions::new()
            .write(true)
            .create(true)
            .truncate(true)
            // create_ap.conf holds credentials; keep it 0600
            .mode(0o600)
            .open(path)
            .await
            .with_context(|| format!("failed to open config {}", path.display()))?;

        file
            .write_all(buf.as_bytes())
            .await
            .with_context(|| format!("failed to write config {}", path.display()))
    }

    pub fn get(&self, key: &str) -> Option<&String> {
        self.entries.get(key)
    }

    pub fn set(&mut self, key: &str, value: String) {
        self.entries.insert(key.to_string(), value);
    }

    pub fn delete(&mut self, key: &str) {
        self.entries.remove(key);
    }
}

pub fn validate_entries(entries: &BTreeMap<String, String>) -> Result<()> {
    if let Some(ssid) = entries.get("SSID") {
        ensure!(
            !ssid.trim().is_empty() && ssid.len() <= 32,
            "SSID must be 1-32 characters"
        );
    }
    if let Some(pass) = entries.get("PASSPHRASE") {
        if !pass.is_empty() {
            ensure!(
                (8..=63).contains(&pass.len()),
                "Passphrase must be 8-63 characters"
            );
        }
    }
    if let Some(band) = entries.get("FREQ_BAND") {
        ensure!(
            ["2.4", "5", "2.4GHz", "5GHz"].contains(&band.as_str()),
            "FREQ_BAND must be 2.4 or 5"
        );
    }
    if let Some(iface) = entries.get("WIFI_IFACE") {
        ensure!(!iface.trim().is_empty(), "WIFI_IFACE cannot be empty");
    }
    if let Some(iface) = entries.get("INTERNET_IFACE") {
        ensure!(!iface.trim().is_empty(), "INTERNET_IFACE cannot be empty");
    }
    Ok(())
}

fn parse_ini(contents: &str) -> Result<BTreeMap<String, String>> {
    let mut out = BTreeMap::new();
    for line in contents.lines() {
        let trimmed = line.trim();
        if trimmed.is_empty() || trimmed.starts_with('#') {
            continue;
        }
        if let Some((k, v)) = trimmed.split_once('=') {
            out.insert(k.trim().to_string(), v.trim().to_string());
        }
    }
    Ok(out)
}
