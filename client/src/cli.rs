use clap::{Parser, Subcommand};
use std::path::PathBuf;

#[derive(Parser)]
#[command(
    author,
    version,
    about = "A command-line tool for rendering image or video to a window.",
    long_about = "This tool allows you to list windows and render either an image or a video to a specified window. \
                  Use the 'List' command to view available windows, and the 'Image' or 'Video' commands to render \
                  content. The window handle should be provided in hexadecimal format (e.g., 0x12345678)."
)]
pub struct Cli {
    /// Server IP address to connect to.
    #[arg(
        short,
        long,
        default_value = "127.0.0.1",
        help = "The IP address of the server to connect to."
    )]
    pub ip: String,

    /// Server port number.
    #[arg(
        short,
        long,
        default_value = "12345",
        help = "The port number of the server to connect to."
    )]
    pub port: u16,

    #[command(subcommand)]
    pub command: Commands,
}

#[derive(Subcommand)]
pub enum Commands {
    /// List available windows.
    #[command(about = "Lists all available windows on the system. You can optionally filter windows by title or request detailed info.")]
    List {
        /// Filter windows by title.
        #[arg(
            short,
            long,
            help = "Provide a substring to filter windows by their title."
        )]
        filter: Option<String>,

        /// Show detailed window information.
        #[arg(
            short,
            long,
            help = "Include detailed information for each window (e.g., dimensions, position, etc.)."
        )]
        verbose: bool,
    },
    /// Render image to specified window.
    #[command(about = "Renders an image onto a specified window.")]
    Image {
        /// Target window handle (in hexadecimal format).
        #[arg(
            short = 'w',
            long,
            help = "The window handle where the image will be rendered. Use hexadecimal format (e.g., 0x12345678)."
        )]
        hwnd: String,

        /// Image file path.
        #[arg(
            short,
            long,
            help = "The file path of the image to render. Supported formats include PNG, JPG, etc."
        )]
        file: PathBuf,
    },
    /// Render video to specified window.
    #[command(about = "Renders a video onto a specified window.")]
    Video {
        /// Target window handle (in hexadecimal format).
        #[arg(
            short = 'w',
            long,
            help = "The window handle where the video will be rendered. Use hexadecimal format (e.g., 0x12345678)."
        )]
        hwnd: String,

        /// Video file path.
        #[arg(
            short,
            long,
            help = "The file path of the video to render. Supported formats include MP4, AVI, etc."
        )]
        file: PathBuf,
    },
}
