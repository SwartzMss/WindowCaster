use serde::{Deserialize, Serialize};
use colored::*;
use std::fmt;

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct WindowInfo {
    pub handle: u64,
    pub title: String,
    pub class_name: String,
}

impl fmt::Display for WindowInfo {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "{} [{}] - {}",
            format!("0x{:X}", self.handle).green(),
            self.class_name.blue(),
            self.title.white()
        )
    }
}

impl WindowInfo {
    pub fn display_verbose(&self) -> String {
        format!(
            "窗口句柄: {}\n标题: {}\n类名: {}\n",
            format!("0x{:X}", self.handle).green(),
            self.title.white(),
            self.class_name.blue()
        )
    }
}

pub struct WindowManager {
    windows: Vec<WindowInfo>,
}

impl WindowManager {
    pub fn new() -> Self {
        Self {
            windows: Vec::new(),
        }
    }

    pub fn update_windows(&mut self, windows: Vec<WindowInfo>) {
        self.windows = windows;
    }

    pub fn list_windows(&self, filter: Option<&str>, verbose: bool) {
        let filtered_windows: Vec<_> = self.windows.iter()
            .filter(|w| {
                if let Some(filter) = filter {
                    w.title.to_lowercase().contains(&filter.to_lowercase())
                } else {
                    true
                }
            })
            .collect();

        if filtered_windows.is_empty() {
            println!("{}", "没有找到匹配的窗口".yellow());
            return;
        }

        println!("找到 {} 个窗口:", filtered_windows.len());
        for window in filtered_windows {
            if verbose {
                println!("{}\n", window.display_verbose());
            } else {
                println!("{}", window);
            }
        }
    }

    pub fn find_window(&self, hwnd: u64) -> Option<&WindowInfo> {
        self.windows.iter().find(|w| w.handle == hwnd)
    }
} 