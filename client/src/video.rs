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
        // Initialize FFmpeg
        ffmpeg::init().context("Failed to initialize FFmpeg")?;

        // Open the video file
        let mut input = ffmpeg::format::input(&video_path)
            .with_context(|| format!("Failed to open video file: {}", video_path.display()))?;

        // Get the video stream
        let video_stream = input
            .streams()
            .best(ffmpeg::media::Type::Video)
            .context("No video stream found in the file")?;
        let video_stream_index = video_stream.index();

        // Create the decoder
        let context_decoder = ffmpeg::codec::context::Context::from_parameters(video_stream.parameters())?;
        let mut decoder = context_decoder.decoder().video()?;

        // Get video information
        let width = decoder.width();
        let height = decoder.height();
        info!("Video dimensions: {}x{}", width, height);

        // Create progress bar
        let total_frames = video_stream.frames() as u64;
        let pb = ProgressBar::new(total_frames);
        pb.set_style(ProgressStyle::default_bar()
            .template("{spinner:.green} [{elapsed_precise}] [{bar:40.cyan/blue}] {pos}/{len} ({eta})")
            .unwrap()
            .progress_chars("#>-"));

        // Create the scaler to convert frames to RGB format
        let mut scaler = ffmpeg::software::scaling::context::Context::get(
            decoder.format(),
            width,
            height,
            ffmpeg::format::Pixel::RGB24,
            width,
            height,
            ffmpeg::software::scaling::flag::Flags::BILINEAR,
        )?;

        // Read and process each frame
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
                    // Convert to RGB format
                    scaler.run(&receive_frame, &mut rgb_frame)?;
                    
                    // Get frame data
                    let frame_data = rgb_frame.data(0).to_vec();
                    
                    // Send frame data to the server using actual width and height
                    let request = Protocol::create_video_frame_request(
                        self.target_window,
                        frame_data,
                        width,  // pass video frame width
                        height, // pass video frame height
                    )?;
                    
                    self.client.send_message(&request).await?;
                    
                    // Wait for the server response
                    let response = self.client.receive_message().await?;
                    let server_response = Protocol::parse_server_response(&response)?;
                    if let Some(status) = server_response.status.as_ref() {
                        if !status.success {
                            warn!("Failed to render frame {}: {}", frame_index, status.message);
                        }
                    } else {
                        warn!("Server response has no status field");
                    }
                    
                    frame_index += 1;
                    pb.inc(1);
                    
                    // Add a small delay to control the frame rate (approximately 30fps)
                    tokio::time::sleep(Duration::from_millis(33)).await;
                }
            }
        }

        // Send the final frame (an empty frame) to signal the end of video
        let request = Protocol::create_video_frame_request(
            self.target_window,
            Vec::new(),
            width,
            height,
        )?;
        self.client.send_message(&request).await?;

        pb.finish_with_message("Video rendering completed");
        Ok(())
    }
}
