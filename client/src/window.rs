use serde::{Deserialize, Serialize};
use std::fmt;

#[derive(Debug, Serialize, Deserialize, Clone)]
pub struct WindowInfo {
    pub handle: u64,
    pub title: String,
    pub class_name: String,
}

impl fmt::Display for WindowInfo {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        // 不使用 colored，直接打印纯文本
        write!(
            f,
            "0x{:X} [{}] - {}",
            self.handle,
            self.class_name,
            self.title
        )
    }
}

impl WindowInfo {
    pub fn display_verbose(&self) -> String {
        // 同样去掉所有 .white()、.blue() 等调用，改为纯文本
        format!(
            "Window Handle: 0x{:X}\nTitle: {}\nClass: {}\n",
            self.handle,
            self.title,
            self.class_name
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
            // 同样去除 .yellow()
            println!("No matching windows found");
            return;
        }

        println!("Found {} windows:", filtered_windows.len());
        for window in filtered_windows {
            if verbose {
                println!("{}\n", window.display_verbose());
            } else {
                println!("{}", window);
            }
        }
    }
}
