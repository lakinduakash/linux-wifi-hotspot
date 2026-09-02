use crate::state::AppState;
use axum::{
    body::Body,
    extract::State,
    http::{header::AUTHORIZATION, Request, StatusCode},
    middleware::Next,
    response::Response,
};
use tracing::warn;

/// Enforces that callers present the configured API token (if any).
pub async fn enforce_token(
    State(state): State<AppState>,
    req: Request<Body>,
    next: Next,
) -> Result<Response, StatusCode> {
    if let Some(expected) = &state.api_token {
        let authorized = extract_token(&req)
            .map(|token| token == expected.as_str())
            .unwrap_or(false);
        if !authorized {
            warn!("rejecting request without valid API token");
            return Err(StatusCode::UNAUTHORIZED);
        }
    }

    Ok(next.run(req).await)
}

fn extract_token(req: &Request<Body>) -> Option<String> {
    if let Some(header) = req.headers().get(AUTHORIZATION) {
        if let Ok(value) = header.to_str() {
            if let Some(token) = value.strip_prefix("Bearer ") {
                return Some(token.trim().to_string());
            }
        }
    }
    if let Some(header) = req.headers().get("x-hotspotd-token") {
        return header.to_str().ok().map(|s| s.trim().to_string());
    }
    None
}
