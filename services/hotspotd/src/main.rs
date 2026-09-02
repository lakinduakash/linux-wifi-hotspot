mod api;
mod auth;
mod config;
mod hotspot;
mod nm;
mod services;
mod state;

use anyhow::Result;
use clap::Parser;
use services::{ManagedService, ManagedServiceArg};
use state::AppState;
use tokio::signal;
use tracing_subscriber::EnvFilter;

#[derive(Parser, Debug)]
#[command(
    author,
    version,
    about = "Privileged helper daemon for linux-wifi-hotspot web control"
)]
struct Cli {
    /// Path to the global create_ap config file.
    #[arg(long, default_value = "/etc/create_ap.conf")]
    config_path: String,

    /// Path to store runtime state (sockets, pid files).
    #[arg(long, default_value = "/run/hotspotd")]
    runtime_dir: String,

    /// Optional API bind address (unix socket by default).
    #[arg(long)]
    http_listen: Option<String>,

    /// Default SSID to use when bootstrapping /etc/create_ap.conf (env: HOTSPOTD_DEFAULT_SSID).
    #[arg(long, env = "HOTSPOTD_DEFAULT_SSID")]
    default_ssid: Option<String>,

    /// Default passphrase to use when bootstrapping config (env: HOTSPOTD_DEFAULT_PASSPHRASE).
    #[arg(long, env = "HOTSPOTD_DEFAULT_PASSPHRASE")]
    default_passphrase: Option<String>,

    /// Default Wi-Fi interface name (env: HOTSPOTD_DEFAULT_WIFI_IFACE).
    #[arg(long, env = "HOTSPOTD_DEFAULT_WIFI_IFACE")]
    default_wifi_iface: Option<String>,

    /// Default upstream interface (env: HOTSPOTD_DEFAULT_INTERNET_IFACE).
    #[arg(long, env = "HOTSPOTD_DEFAULT_INTERNET_IFACE")]
    default_internet_iface: Option<String>,

    /// Shared API token required for HTTP clients (env: HOTSPOTD_API_TOKEN).
    #[arg(long, env = "HOTSPOTD_API_TOKEN")]
    api_token: Option<String>,

    /// System services to expose via the API (format id:unit, comma separated env HOTSPOTD_MANAGED_SERVICES).
    #[arg(
        long = "managed-service",
        env = "HOTSPOTD_MANAGED_SERVICES",
        value_delimiter = ','
    )]
    managed_services: Vec<ManagedServiceArg>,
}

#[tokio::main]
async fn main() -> Result<()> {
    tracing_subscriber::fmt()
        .with_env_filter(EnvFilter::from_default_env())
        .init();

    let cli = Cli::parse();

    let defaults = config::ConfigDefaults::from_template()
        .unwrap_or_default()
        .override_entry("SSID", cli.default_ssid.clone())
        .override_entry("PASSPHRASE", cli.default_passphrase.clone())
        .override_entry("WIFI_IFACE", cli.default_wifi_iface.clone())
        .override_entry("INTERNET_IFACE", cli.default_internet_iface.clone());

    let managed_services: Vec<ManagedService> = cli
        .managed_services
        .into_iter()
        .map(|arg| arg.into())
        .collect();

    let state = AppState::initialize(
        &cli.config_path,
        &cli.runtime_dir,
        defaults,
        cli.api_token,
        managed_services,
    )
    .await?;

    let server = api::serve(&state, cli.http_listen.clone());

    tokio::select! {
        _ = server => {
            tracing::info!("hotspotd server exited");
        }
        _ = signal::ctrl_c() => {
            tracing::info!("received shutdown signal");
        }
    }

    Ok(())
}
