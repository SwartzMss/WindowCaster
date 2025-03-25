use anyhow::Result;
use clap::Parser;
use tracing::{info, error};

mod cli;
mod network;
mod window;
mod proto;
mod video;

use cli::{Cli, Commands};
use network::NetworkClient;
use window::WindowManager;
use proto::Protocol;
use video::VideoRenderer;

#[tokio::main]
async fn main() -> Result<()> {
    tracing_subscriber::fmt()
    .with_max_level(tracing::Level::INFO)
    .with_ansi(false)
    .init();

    let cli = Cli::parse();
    let mut client = NetworkClient::new(cli.ip, cli.port)?;
    client.connect().await?;

    let mut window_manager = WindowManager::new();

    match cli.command {
        Commands::List { filter, verbose } => {
            let request = Protocol::create_get_window_list_request()?;
            client.send_message(&request).await?;

            let response = client.receive_message().await?;
            let resp = Protocol::parse_server_response(&response)?;
            if let Some(window_list_msg) = resp.window_list.as_ref() {
                // window_list_msg 是 WindowList { windows: Vec<WindowInfo> }
                let converted: Vec<window::WindowInfo> = window_list_msg
                .windows
                .iter()
                .map(|w| {
                    window::WindowInfo {
                        handle: w.handle,
                        title: w.title.clone(),
                        class_name: w.class_name.clone(),
                    }
                })
                .collect();
            
            window_manager.update_windows(converted);
                window_manager.list_windows(filter.as_deref(), verbose);
            } else if let Some(status_msg) = resp.status.as_ref() {
                if !status_msg.success {
                    error!("Image rendering failed: {}", String::from_utf8_lossy(&status_msg.message));
                }
            } else {
                error!("Unexpected server response: neither window_list nor status is set.");
            }
        }

        Commands::Image { hwnd, file } => {
            let hwnd_str = hwnd.trim_start_matches("0x");
            let hwnd = u64::from_str_radix(hwnd_str, 16)?;
            
            info!("Rendering image {} to window 0x{:X}", file.display(), hwnd);
            let request = Protocol::create_image_render_request(hwnd, &file)?;
            client.send_message(&request).await?;

            let response = client.receive_message().await?;
            let server_response = Protocol::parse_server_response(&response)?;

            if let Some(status) = server_response.status.as_ref() {
                if status.success {
                    info!("Image rendering succeeded");
                } else {
                    error!("Image rendering failed: {}", String::from_utf8_lossy(&status.message));
                }
            } else {
                error!("Received an unexpected server response (no status)");
            }
        }

        Commands::Video { hwnd, file } => {
            let hwnd_str = hwnd.trim_start_matches("0x");
            let hwnd = u64::from_str_radix(hwnd_str, 16)?;
            info!("Rendering video {} to window 0x{:X}", file.display(), hwnd);
            
            // Create video renderer
            let mut renderer = VideoRenderer::new(client, hwnd);
            
            // Start video rendering
            if let Err(e) = renderer.render_video(&file).await {
                error!("Video rendering failed: {}", e);
                return Err(e);
            }
            
            info!("Video rendering completed");
        }
    }

    Ok(())
}
