use clap::{Parser, Subcommand};
use std::path::PathBuf;

#[derive(Parser)]
#[command(author, version, about, long_about = None)]
pub struct Cli {
    /// Server IP address
    #[arg(short, long, default_value = "127.0.0.1")]
    pub ip: String,

    /// Server port
    #[arg(short, long, default_value = "12345")]
    pub port: u16,

    #[command(subcommand)]
    pub command: Commands,
}

#[derive(Subcommand)]
pub enum Commands {
    /// List available windows
    List {
        /// Filter windows by title
        #[arg(short, long)]
        filter: Option<String>,

        /// Show detailed information
        #[arg(short, long)]
        verbose: bool,
    },
    /// Render image to specified window
    Image {
        /// Target window handle (in hexadecimal format)
        #[arg(short = 'w', long)]
        hwnd: String,

        /// Image file path
        #[arg(short, long)]
        file: PathBuf,
    },
    /// Render video to specified window
    Video {
        /// Target window handle (in hexadecimal format)
        #[arg(short = 'w', long)]
        hwnd: String,

        /// Video file path
        #[arg(short, long)]
        file: PathBuf,
    },
}
