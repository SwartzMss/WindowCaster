use anyhow::{Result, Context};
use std::path::Path;
use protobuf::Message;

// 包含 build.rs 生成的 Protobuf 模块代码，模块名与 .proto 文件中的 package 对应
include!(concat!(env!("OUT_DIR"), "/protos/mod.rs"));

pub struct Protocol;

impl Protocol {
    /// 创建获取窗口列表请求，并序列化为字节数组
    pub fn create_get_window_list_request() -> Result<Vec<u8>> {
        let mut request = windowcaster::ClientRequest::new();
        // 为 oneof 字段设置 get_window_list（空消息）
        let get_list = windowcaster::GetWindowList::new();
        request.set_get_window_list(get_list);

        Ok(request.write_to_bytes()?)
    }

    /// 创建图片渲染请求
    ///
    /// 此处读取图片文件全部数据，并构造 Image 消息
    pub fn create_image_render_request(hwnd: u64, file_path: &Path) -> Result<Vec<u8>> {
        let image_data = std::fs::read(file_path)
            .context("Failed to read image file")?;

        // 根据 windowcaster.rs 可知，Image 结构里直接有 pub data / width / height
        let mut image = windowcaster::Image::new();
        image.data = image_data;
        image.width = 0;    // 你可以按实际需求改成正确的宽高
        image.height = 0;   // 同上

        // RenderCommand 里有 target_window 字段和 set_image()
        let mut render_command = windowcaster::RenderCommand::new();
        render_command.target_window = hwnd;  // 直接赋值
        render_command.set_image(image);      // oneof 里有 set_image()

        let mut request = windowcaster::ClientRequest::new();
        request.set_render_command(render_command);

        Ok(request.write_to_bytes()?)
    }

    /// 创建视频帧渲染请求
    pub fn create_video_frame_request(
        hwnd: u64, 
        frame_data: Vec<u8>, 
        width: u32, 
        height: u32
    ) -> Result<Vec<u8>> {

        let mut video = windowcaster::Video::new();
        // Video 结构里是 pub frame_data / width / height
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

    /// 解析服务器响应
    pub fn parse_server_response(data: &[u8]) -> Result<windowcaster::ServerResponse> {
        // 这里保持不变即可
        Ok(windowcaster::ServerResponse::parse_from_bytes(data)
            .context("Failed to parse server response")?)
    }
}
