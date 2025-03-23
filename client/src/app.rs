use eframe::egui;
use std::sync::Arc;
use tokio::sync::Mutex;

use crate::network::NetworkClient;
use crate::ui::WindowList;

pub struct WindowCasterApp {
    network_client: Arc<Mutex<NetworkClient>>,
    window_list: WindowList,
}

impl WindowCasterApp {
    pub fn new(cc: &eframe::CreationContext<'_>) -> Self {
        // 设置自定义字体和样式
        let mut style = (*cc.egui_ctx.style()).clone();
        style.text_styles = [
            (egui::TextStyle::Heading, egui::FontId::new(24.0, egui::FontFamily::Proportional)),
            (egui::TextStyle::Body, egui::FontId::new(16.0, egui::FontFamily::Proportional)),
            (egui::TextStyle::Button, egui::FontId::new(16.0, egui::FontFamily::Proportional)),
        ]
        .into();
        cc.egui_ctx.set_style(style);

        Self {
            network_client: Arc::new(Mutex::new(NetworkClient::new("127.0.0.1:12345"))),
            window_list: WindowList::default(),
        }
    }
}

impl eframe::App for WindowCasterApp {
    fn update(&mut self, ctx: &egui::Context, _frame: &mut eframe::Frame) {
        egui::CentralPanel::default().show(ctx, |ui| {
            ui.heading("WindowCaster");
            
            // 连接状态和控制
            ui.horizontal(|ui| {
                if ui.button("连接服务器").clicked() {
                    // TODO: 实现连接逻辑
                }
                
                if ui.button("刷新窗口列表").clicked() {
                    // TODO: 实现刷新逻辑
                }
            });
            
            // 显示窗口列表
            self.window_list.ui(ui);
        });
    }
} 