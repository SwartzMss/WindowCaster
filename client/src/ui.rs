use eframe::egui;

#[derive(Default)]
pub struct WindowList {
    windows: Vec<WindowInfo>,
    selected_window: Option<usize>,
}

#[derive(Clone)]
pub struct WindowInfo {
    pub id: u64,
    pub title: String,
    pub class_name: String,
}

impl WindowList {
    pub fn ui(&mut self, ui: &mut egui::Ui) {
        ui.group(|ui| {
            ui.heading("可用窗口");
            
            egui::ScrollArea::vertical().show(ui, |ui| {
                for (index, window) in self.windows.iter().enumerate() {
                    let is_selected = self.selected_window == Some(index);
                    
                    if ui.selectable_label(is_selected, &window.title).clicked() {
                        self.selected_window = Some(index);
                    }
                    
                    ui.small(&window.class_name);
                    ui.separator();
                }
            });
        });
        
        if let Some(index) = self.selected_window {
            ui.group(|ui| {
                ui.heading("窗口操作");
                if ui.button("开始渲染").clicked() {
                    // TODO: 实现渲染逻辑
                }
                
                if ui.button("停止渲染").clicked() {
                    // TODO: 实现停止渲染逻辑
                }
            });
        }
    }

    pub fn update_windows(&mut self, windows: Vec<WindowInfo>) {
        self.windows = windows;
        self.selected_window = None;
    }
} 