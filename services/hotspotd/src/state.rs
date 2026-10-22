use crate::config::{ConfigDefaults, HotspotConfig};
use crate::services::ManagedService;
use anyhow::Result;
use std::path::PathBuf;
use std::sync::Arc;
use tokio::sync::RwLock;

#[derive(Clone)]
pub struct AppState {
    pub config_path: PathBuf,
    pub runtime_dir: PathBuf,
    pub config: Arc<RwLock<HotspotConfig>>,
    pub api_token: Option<String>,
    pub services: Arc<Vec<ManagedService>>,
}

impl AppState {
    pub async fn initialize(
        config_path: &str,
        runtime_dir: &str,
        defaults: ConfigDefaults,
        api_token: Option<String>,
        services: Vec<ManagedService>,
    ) -> Result<Self> {
        let cfg_path = PathBuf::from(config_path);
        let rt_dir = PathBuf::from(runtime_dir);
        tokio::fs::create_dir_all(&rt_dir).await?;

        let config = HotspotConfig::load_or_init(&cfg_path, &defaults).await?;

        Ok(Self {
            config_path: cfg_path,
            runtime_dir: rt_dir,
            config: Arc::new(RwLock::new(config)),
            api_token,
            services: Arc::new(services),
        })
    }
}
