use crate::window::{WindowManager, WindowInfo};
use anyhow::Result;
use dialoguer::{theme::ColorfulTheme, Select, Input};
use std::path::PathBuf;

pub struct InteractiveMode {
    window_manager: WindowManager,
}

impl InteractiveMode {
    pub fn new(window_manager: WindowManager) -> Self {
        Self { window_manager }
    }

    pub async fn run(&self) -> Result<()> {
        let theme = ColorfulTheme::default();

        loop {
            let actions = &[
                "列出窗口",
                "渲染图片",
                "渲染视频",
                "退出"
            ];

            let selection = Select::with_theme(&theme)
                .with_prompt("请选择操作")
                .items(actions)
                .default(0)
                .interact()?;

            match selection {
                0 => self.list_windows()?,
                1 => self.render_image().await?,
                2 => self.render_video().await?,
                3 => break,
                _ => unreachable!(),
            }

            println!();
        }

        Ok(())
    }

    fn list_windows(&self) -> Result<()> {
        let filter: String = Input::with_theme(&ColorfulTheme::default())
            .with_prompt("输入窗口标题过滤条件（可选）")
            .allow_empty(true)
            .interact_text()?;

        let verbose = dialoguer::Confirm::with_theme(&ColorfulTheme::default())
            .with_prompt("显示详细信息？")
            .default(false)
            .interact()?;

        self.window_manager.list_windows(
            if filter.is_empty() { None } else { Some(&filter) },
            verbose
        );

        Ok(())
    }

    async fn render_image(&self) -> Result<()> {
        let hwnd = self.select_window()?;
        let file_path: String = Input::with_theme(&ColorfulTheme::default())
            .with_prompt("输入图片文件路径")
            .interact_text()?;

        println!("准备渲染图片 {} 到窗口 0x{:X}", file_path, hwnd);
        // TODO: 实现图片渲染逻辑

        Ok(())
    }

    async fn render_video(&self) -> Result<()> {
        let hwnd = self.select_window()?;
        let file_path: String = Input::with_theme(&ColorfulTheme::default())
            .with_prompt("输入视频文件路径")
            .interact_text()?;

        println!("准备渲染视频 {} 到窗口 0x{:X}", file_path, hwnd);
        // TODO: 实现视频渲染逻辑

        Ok(())
    }

    fn select_window(&self) -> Result<u64> {
        let hwnd_str: String = Input::with_theme(&ColorfulTheme::default())
            .with_prompt("输入目标窗口句柄（十六进制格式）")
            .validate_with(|input: &String| -> Result<(), &str> {
                if input.starts_with("0x") || input.starts_with("0X") {
                    let hex_str = &input[2..];
                    if u64::from_str_radix(hex_str, 16).is_ok() {
                        Ok(())
                    } else {
                        Err("无效的十六进制格式")
                    }
                } else {
                    Err("请使用0x前缀的十六进制格式")
                }
            })
            .interact_text()?;

        let hex_str = &hwnd_str[2..];
        let hwnd = u64::from_str_radix(hex_str, 16)?;

        if self.window_manager.find_window(hwnd).is_none() {
            anyhow::bail!("找不到指定的窗口");
        }

        Ok(hwnd)
    }
} 