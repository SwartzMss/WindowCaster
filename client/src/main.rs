mod cli;
mod network;
mod window;
mod interactive;
mod proto;
mod video;

use anyhow::Result;
use clap::Parser;
use cli::{Cli, Commands};
use network::NetworkClient;
use window::{WindowManager, WindowInfo};
use interactive::InteractiveMode;
use proto::{Protocol, ServerResponse};
use video::VideoRenderer;
use tracing::{info, error};

#[tokio::main]
async fn main() -> Result<()> {
    // 初始化日志
    tracing_subscriber::fmt::init();

    // 解析命令行参数
    let cli = Cli::parse();

    // 创建网络客户端
    let mut client = NetworkClient::new(cli.host, cli.port)?;
    client.connect().await?;

    // 创建窗口管理器
    let mut window_manager = WindowManager::new();

    // 处理命令
    match cli.command {
        Commands::List { filter, verbose } => {
            // 发送获取窗口列表请求
            let request = Protocol::create_get_window_list_request()?;
            client.send_message(&request).await?;

            // 接收响应
            let response = client.receive_message().await?;
            match Protocol::parse_server_response(&response)? {
                ServerResponse::WindowList(windows) => {
                    window_manager.update_windows(windows);
                    window_manager.list_windows(filter.as_deref(), verbose);
                }
                ServerResponse::Status { success, message } => {
                    if !success {
                        error!("获取窗口列表失败: {}", message);
                    }
                }
            }
        }

        Commands::Image { hwnd, file } => {
            let hwnd_str = hwnd.trim_start_matches("0x");
            let hwnd = u64::from_str_radix(hwnd_str, 16)?;
            
            // 发送图片渲染请求
            info!("渲染图片 {} 到窗口 0x{:X}", file.display(), hwnd);
            let request = Protocol::create_image_render_request(hwnd, &file)?;
            client.send_message(&request).await?;

            // 接收响应
            let response = client.receive_message().await?;
            match Protocol::parse_server_response(&response)? {
                ServerResponse::Status { success, message } => {
                    if success {
                        info!("图片渲染成功");
                    } else {
                        error!("图片渲染失败: {}", message);
                    }
                }
                _ => error!("收到意外的服务器响应"),
            }
        }

        Commands::Video { hwnd, file } => {
            let hwnd_str = hwnd.trim_start_matches("0x");
            let hwnd = u64::from_str_radix(hwnd_str, 16)?;
            info!("渲染视频 {} 到窗口 0x{:X}", file.display(), hwnd);
            
            // 创建视频渲染器
            let mut renderer = VideoRenderer::new(client, hwnd);
            
            // 开始渲染视频
            if let Err(e) = renderer.render_video(&file).await {
                error!("视频渲染失败: {}", e);
                return Err(e);
            }
            
            info!("视频渲染完成");
        }

        Commands::Interactive => {
            // 首先获取窗口列表
            let request = Protocol::create_get_window_list_request()?;
            client.send_message(&request).await?;

            let response = client.receive_message().await?;
            match Protocol::parse_server_response(&response)? {
                ServerResponse::WindowList(windows) => {
                    window_manager.update_windows(windows);
                }
                ServerResponse::Status { success, message } => {
                    if !success {
                        error!("获取窗口列表失败: {}", message);
                        return Ok(());
                    }
                }
            }

            let interactive = InteractiveMode::new(window_manager);
            interactive.run().await?;
        }
    }

    Ok(())
}
