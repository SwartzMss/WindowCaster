use anyhow::{Context, Result};
use std::net::SocketAddr;
use tokio::net::TcpStream;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tracing::{debug, error, info};

pub struct NetworkClient {
    stream: Option<TcpStream>,
    server_addr: SocketAddr,
}

impl NetworkClient {
    pub fn new(host: String, port: u16) -> Result<Self> {
        let server_addr = format!("{}:{}", host, port)
            .parse()
            .context("无效的服务器地址")?;

        Ok(Self {
            stream: None,
            server_addr,
        })
    }

    pub async fn connect(&mut self) -> Result<()> {
        info!("正在连接到服务器 {}...", self.server_addr);
        let stream = TcpStream::connect(self.server_addr)
            .await
            .context("连接服务器失败")?;
        
        self.stream = Some(stream);
        info!("已连接到服务器");
        Ok(())
    }

    pub async fn send_message(&mut self, message: &[u8]) -> Result<()> {
        if let Some(stream) = &mut self.stream {
            // 发送消息长度
            let len = message.len() as u32;
            stream.write_all(&len.to_le_bytes()).await?;
            
            // 发送消息内容
            stream.write_all(message).await?;
            debug!("已发送 {} 字节", len);
            Ok(())
        } else {
            error!("未连接到服务器");
            anyhow::bail!("未连接到服务器")
        }
    }

    pub async fn receive_message(&mut self) -> Result<Vec<u8>> {
        if let Some(stream) = &mut self.stream {
            // 读取消息长度
            let mut len_bytes = [0u8; 4];
            stream.read_exact(&mut len_bytes).await?;
            let len = u32::from_le_bytes(len_bytes) as usize;
            
            // 读取消息内容
            let mut buffer = vec![0u8; len];
            stream.read_exact(&mut buffer).await?;
            debug!("已接收 {} 字节", len);
            Ok(buffer)
        } else {
            error!("未连接到服务器");
            anyhow::bail!("未连接到服务器")
        }
    }

    pub fn is_connected(&self) -> bool {
        self.stream.is_some()
    }

    pub async fn disconnect(&mut self) {
        if let Some(stream) = self.stream.take() {
            drop(stream);
            info!("已断开连接");
        }
    }
} 