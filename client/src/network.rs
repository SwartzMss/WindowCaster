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
            .context("Invalid server address")?;

        Ok(Self {
            stream: None,
            server_addr,
        })
    }

    pub async fn connect(&mut self) -> Result<()> {
        info!("Connecting to server {}...", self.server_addr);
        let stream = TcpStream::connect(self.server_addr)
            .await
            .context("Failed to connect to server")?;
        
        self.stream = Some(stream);
        info!("Connected to server");
        Ok(())
    }

    pub async fn send_message(&mut self, message: &[u8]) -> Result<()> {
        if let Some(stream) = &mut self.stream {
            // Send message length
            let len = message.len() as u32;
            stream.write_all(&len.to_le_bytes()).await?;
            
            // Send message content
            stream.write_all(message).await?;
            info!("Sent {} bytes", len);
            Ok(())
        } else {
            error!("Not connected to server");
            anyhow::bail!("Not connected to server")
        }
    }

    pub async fn receive_message(&mut self) -> Result<Vec<u8>> {
        if let Some(stream) = &mut self.stream {
            // Read message length
            let mut len_bytes = [0u8; 4];
            stream.read_exact(&mut len_bytes).await?;
            let len = u32::from_le_bytes(len_bytes) as usize;
            
            // Read message content
            let mut buffer = vec![0u8; len];
            stream.read_exact(&mut buffer).await?;
            debug!("Received {} bytes", len);
            Ok(buffer)
        } else {
            error!("Not connected to server");
            anyhow::bail!("Not connected to server")
        }
    }
}
