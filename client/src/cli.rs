use clap::{Parser, Subcommand};
use std::path::PathBuf;

#[derive(Parser)]
#[command(name = "windowcaster")]
#[command(author, version, about, long_about = None)]
pub struct Cli {
    /// 服务器地址
    #[arg(short, long, default_value = "127.0.0.1")]
    pub host: String,

    /// 服务器端口
    #[arg(short, long, default_value = "12345")]
    pub port: u16,

    #[command(subcommand)]
    pub command: Commands,
}

#[derive(Subcommand)]
pub enum Commands {
    /// 列出可用的窗口
    List {
        /// 按标题过滤窗口
        #[arg(short, long)]
        filter: Option<String>,

        /// 显示详细信息
        #[arg(short, long)]
        verbose: bool,
    },
    /// 渲染图片到指定窗口
    Image {
        /// 目标窗口句柄（十六进制格式）
        #[arg(short, long)]
        hwnd: String,

        /// 图片文件路径
        #[arg(short, long)]
        file: PathBuf,
    },
    /// 渲染视频到指定窗口
    Video {
        /// 目标窗口句柄（十六进制格式）
        #[arg(short, long)]
        hwnd: String,

        /// 视频文件路径
        #[arg(short, long)]
        file: PathBuf,
    },
    /// 交互模式
    Interactive,
} 