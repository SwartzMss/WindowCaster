use anyhow::{Result, Context};
use ffmpeg_next as ffmpeg;
use std::path::Path;
use std::time::Duration;
use indicatif::{ProgressBar, ProgressStyle};
use tracing::{info, warn};

use crate::network::NetworkClient;
use crate::proto::Protocol;

pub struct VideoRenderer {
    client: NetworkClient,
    target_window: u64,
}

impl VideoRenderer {
    pub fn new(client: NetworkClient, target_window: u64) -> Self {
        Self {
            client,
            target_window,
        }
    }

    pub async fn render_video(&mut self, video_path: &Path) -> Result<()> {
        // 初始化 FFmpeg
        ffmpeg::init().context("初始化 FFmpeg 失败")?;

        // 打开视频文件
        let mut input = ffmpeg::format::input(&video_path)
            .with_context(|| format!("无法打开视频文件: {}", video_path.display()))?;

        // 获取视频流
        let video_stream = input
            .streams()
            .best(ffmpeg::media::Type::Video)
            .context("视频文件中没有找到视频流")?;
        let video_stream_index = video_stream.index();

        // 创建解码器
        let context_decoder = ffmpeg::codec::context::Context::from_parameters(video_stream.parameters())?;
        let mut decoder = context_decoder.decoder().video()?;

        // 获取视频信息
        let width = decoder.width();
        let height = decoder.height();
        info!("视频尺寸: {}x{}", width, height);

        // 创建进度条
        let total_frames = video_stream.frames() as u64;
        let pb = ProgressBar::new(total_frames);
        pb.set_style(ProgressStyle::default_bar()
            .template("{spinner:.green} [{elapsed_precise}] [{bar:40.cyan/blue}] {pos}/{len} ({eta})")
            .unwrap()
            .progress_chars("#>-"));

        // 创建转换器
        let mut scaler = ffmpeg::software::scaling::context::Context::get(
            decoder.format(),
            width,
            height,
            ffmpeg::format::Pixel::RGB24,
            width,
            height,
            ffmpeg::software::scaling::flag::Flags::BILINEAR,
        )?;

        // 读取并处理每一帧
        let mut frame_index = 0u32;
        let mut receive_frame = ffmpeg::frame::Video::empty();
        let mut rgb_frame = ffmpeg::frame::Video::new(
            ffmpeg::format::Pixel::RGB24,
            width,
            height,
        );

        for (stream, packet) in input.packets() {
            if stream.index() == video_stream_index {
                decoder.send_packet(&packet)?;
                
                while decoder.receive_frame(&mut receive_frame).is_ok() {
                    // 转换为 RGB 格式
                    scaler.run(&receive_frame, &mut rgb_frame)?;
                    
                    // 获取帧数据
                    let frame_data = rgb_frame.data(0).to_vec();
                    
                    // 发送帧数据到服务器
                    let request = Protocol::create_video_frame_request(
                        self.target_window,
                        frame_data,
                        frame_index,
                        false,
                    )?;
                    
                    self.client.send_message(&request).await?;
                    
                    // 等待服务器响应
                    let response = self.client.receive_message().await?;
                    match Protocol::parse_server_response(&response)? {
                        crate::proto::ServerResponse::Status { success, message } => {
                            if !success {
                                warn!("渲染帧 {} 失败: {}", frame_index, message);
                            }
                        }
                        _ => warn!("收到意外的服务器响应"),
                    }
                    
                    frame_index += 1;
                    pb.inc(1);
                    
                    // 添加一个小延迟以控制帧率
                    tokio::time::sleep(Duration::from_millis(33)).await; // 约 30fps
                }
            }
        }

        // 发送最后一帧
        let request = Protocol::create_video_frame_request(
            self.target_window,
            Vec::new(),
            frame_index,
            true,
        )?;
        self.client.send_message(&request).await?;

        pb.finish_with_message("视频渲染完成");
        Ok(())
    }
} 