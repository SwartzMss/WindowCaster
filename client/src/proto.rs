use anyhow::{Result, Context};
use std::path::Path;
use protobuf::Message;

include!(concat!(env!("OUT_DIR"), "/protos/mod.rs"));

pub struct Protocol;

impl Protocol {
    pub fn create_get_window_list_request() -> Result<Vec<u8>> {
        let mut request = windowcaster::ClientRequest::new();
        let get_list = windowcaster::GetWindowList::new();
        request.set_get_window_list(get_list);

        Ok(request.write_to_bytes()?)
    }

    pub fn create_image_render_request(hwnd: u64, file_path: &Path) -> Result<Vec<u8>> {
        let image_data = std::fs::read(file_path)
            .context("Failed to read image file")?;

        let mut image = windowcaster::Image::new();
        image.data = image_data;
        image.width = 0;   
        image.height = 0;

        let mut render_command = windowcaster::RenderCommand::new();
        render_command.target_window = hwnd; 
        render_command.set_image(image);

        let mut request = windowcaster::ClientRequest::new();
        request.set_render_command(render_command);

        Ok(request.write_to_bytes()?)
    }

    pub fn create_video_frame_request(
        hwnd: u64, 
        frame_data: Vec<u8>, 
        width: u32, 
        height: u32
    ) -> Result<Vec<u8>> {

        let mut video = windowcaster::Video::new();
        video.frame_data = frame_data;
        video.width = width;
        video.height = height;

        let mut render_command = windowcaster::RenderCommand::new();
        render_command.target_window = hwnd;
        render_command.set_video(video);

        let mut request = windowcaster::ClientRequest::new();
        request.set_render_command(render_command);

        Ok(request.write_to_bytes()?)
    }

    pub fn parse_server_response(data: &[u8]) -> Result<windowcaster::ServerResponse> {
        Ok(windowcaster::ServerResponse::parse_from_bytes(data)
            .context("Failed to parse server response")?)
    }
}
