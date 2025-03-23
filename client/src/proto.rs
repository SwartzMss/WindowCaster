use anyhow::{Result, anyhow};
use serde::{Serialize, Deserialize};
use std::path::Path;
use crate::window::WindowInfo;

#[derive(Debug, Serialize, Deserialize)]
pub enum ClientRequest {
    GetWindowList,
    RenderImage {
        hwnd: u64,
        image_data: Vec<u8>,
    },
    RenderVideo {
        hwnd: u64,
        frame_data: Vec<u8>,
        frame_index: u32,
        is_last_frame: bool,
    },
}

#[derive(Debug, Serialize, Deserialize)]
pub enum ServerResponse {
    WindowList(Vec<WindowInfo>),
    Status {
        success: bool,
        message: String,
    },
}

pub struct Protocol;

impl Protocol {
    pub fn create_get_window_list_request() -> Result<Vec<u8>> {
        let request = ClientRequest::GetWindowList;
        Ok(bincode::serialize(&request)?)
    }

    pub fn create_image_render_request(hwnd: u64, file_path: &Path) -> Result<Vec<u8>> {
        let image_data = std::fs::read(file_path)?;
        let request = ClientRequest::RenderImage {
            hwnd,
            image_data,
        };
        Ok(bincode::serialize(&request)?)
    }

    pub fn create_video_frame_request(hwnd: u64, frame_data: Vec<u8>, frame_index: u32, is_last_frame: bool) -> Result<Vec<u8>> {
        let request = ClientRequest::RenderVideo {
            hwnd,
            frame_data,
            frame_index,
            is_last_frame,
        };
        Ok(bincode::serialize(&request)?)
    }

    pub fn parse_server_response(data: &[u8]) -> Result<ServerResponse> {
        bincode::deserialize(data).map_err(|e| anyhow!("解析服务器响应失败: {}", e))
    }
} 