#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
#include <memory>
#include <string>
#include "window_manager.h"
#include "renderer.h"
#include "network_server.h"
#include "google/protobuf/message.h"
#include "windowcaster.pb.h"



class WindowCasterServer {
public:
	WindowCasterServer()
		: windowManager(std::make_unique<WindowManager>())
		, renderer(std::make_unique<Renderer>())
		, server(std::make_unique<NetworkServer>()) {
		server->SetMessageHandler([this](const std::string& message) {
			HandleMessage(message);
			});
	}

	bool Start() {
		return server->Start();
	}

	void Stop() {
		server->Stop();
	}

private:
	// 辅助函数：验证窗口句柄和初始化渲染器
	bool ValidateAndInitialize(HWND hwnd, windowcaster::Status* status) {
		if (!windowManager->IsWindowValid(hwnd)) {
			status->set_success(false);
			status->set_message("无效的窗口句柄");
			return false;
		}
		if (!renderer->Initialize(hwnd)) {
			status->set_success(false);
			status->set_message("初始化渲染器失败");
			return false;
		}
		return true;
	}

	// 处理来自客户端的消息
	void HandleMessage(const std::string& message) {
		windowcaster::ClientRequest request;
		if (!request.ParseFromString(message)) {
			std::cerr << "解析消息失败" << std::endl;
			return;
		}

		windowcaster::ServerResponse response;
		switch (request.request_case()) {
		case windowcaster::ClientRequest::kGetWindowList:
			HandleGetWindowList(response);
			break;
		case windowcaster::ClientRequest::kRenderCommand:
			HandleRenderCommand(request.render_command(), response);
			break;
		case windowcaster::ClientRequest::kStopRender:
			HandleStopRender(request.stop_render(), response);
			break;
		default:
			response.mutable_status()->set_success(false);
			response.mutable_status()->set_message("未知请求类型");
			break;
		}

		std::string responseStr;
		if (response.SerializeToString(&responseStr)) {
			server->SendMessage(responseStr);
		}
	}

	// 处理获取窗口列表的请求
	void HandleGetWindowList(windowcaster::ServerResponse& response) {
		auto windows = windowManager->EnumerateWindows();
		auto* windowList = response.mutable_window_list();

		for (const auto& window : windows) {
			auto* windowInfo = windowList->add_windows();
			windowInfo->set_handle(reinterpret_cast<uint64_t>(window.handle));

			// 将宽字符转换为 UTF-8 字符串
			int size = WideCharToMultiByte(CP_UTF8, 0, window.title.c_str(), -1, nullptr, 0, nullptr, nullptr);
			std::string title(size, 0);
			WideCharToMultiByte(CP_UTF8, 0, window.title.c_str(), -1, &title[0], size, nullptr, nullptr);
			windowInfo->set_title(title);

			size = WideCharToMultiByte(CP_UTF8, 0, window.className.c_str(), -1, nullptr, 0, nullptr, nullptr);
			std::string className(size, 0);
			WideCharToMultiByte(CP_UTF8, 0, window.className.c_str(), -1, &className[0], size, nullptr, nullptr);
			windowInfo->set_class_name(className);
		}
	}

	// 处理渲染命令请求：支持图像或视频帧渲染
	void HandleRenderCommand(const windowcaster::RenderCommand& command,
		windowcaster::ServerResponse& response) {
		HWND hwnd = reinterpret_cast<HWND>(command.target_window());
		auto* status = response.mutable_status();

		// 统一检查窗口有效性和初始化渲染器
		if (!ValidateAndInitialize(hwnd, status)) {
			return;
		}

		bool success = false;
		switch (command.content_case()) {
		case windowcaster::RenderCommand::kImage: {
			const auto& image = command.image();
			// 这里假设 renderer 实现了 RenderImageFrame 接口，用于处理二进制图片数据
			success = renderer->RenderImageFrame(
				image.data().data(),
				image.width(),
				image.height()
			);
			break;
		}
		case windowcaster::RenderCommand::kVideo: {
			const auto& video = command.video();
			success = renderer->RenderVideoFrame(
				video.frame_data().data(),
				video.width(),
				video.height()
			);
			break;
		}
		default:
			status->set_success(false);
			status->set_message("未知的渲染内容类型");
			return;
		}

		status->set_success(success);
		if (!success) {
			status->set_message("渲染失败");
		}
	}

	// 处理停止渲染的请求
	void HandleStopRender(const windowcaster::StopRender& command,
		windowcaster::ServerResponse& response) {
		HWND hwnd = reinterpret_cast<HWND>(command.target_window());
		auto* status = response.mutable_status();

		if (!ValidateAndInitialize(hwnd, status)) {
			return;
		}

		renderer->Clear();
		status->set_success(true);
	}

private:
	std::unique_ptr<WindowManager> windowManager;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<NetworkServer> server;
};

int main() {
	try {
		GOOGLE_PROTOBUF_VERIFY_VERSION;

		WindowCasterServer server;
		if (!server.Start()) {
			std::cerr << "服务器启动失败" << std::endl;
			return 1;
		}

		std::cout << "WindowCaster 服务端已启动..." << std::endl;
		std::cout << "按 Enter 键退出" << std::endl;
		std::cin.get();

		server.Stop();
		google::protobuf::ShutdownProtobufLibrary();
	}
	catch (const std::exception& e) {
		std::cerr << "错误: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
