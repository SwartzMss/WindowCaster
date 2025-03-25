#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <csignal>
#include <windows.h>
#include "window_manager.h"
#include "renderer.h"
#include "network_server.h"
#include "google/protobuf/message.h"
#include "windowcaster.pb.h"

volatile std::sig_atomic_t gSignalStatus = 0;

void signalHandler(int signal) {
	gSignalStatus = signal;
}

class WindowCasterServer {
public:
	WindowCasterServer(uint16_t port)
		: windowManager(std::make_unique<WindowManager>())
		, renderer(std::make_unique<Renderer>())
		, server(std::make_unique<NetworkServer>(port)) {
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
	bool ValidateAndInitialize(HWND hwnd, windowcaster::Status* status) {
		if (!windowManager->IsWindowValid(hwnd)) {
			status->set_success(false);
			status->set_message("Invalid window handle");
			return false;
		}
		if (!renderer->Initialize(hwnd)) {
			status->set_success(false);
			status->set_message("Renderer initialization failed");
			return false;
		}
		return true;
	}

	void HandleMessage(const std::string& message) {
		windowcaster::ClientRequest request;
		if (!request.ParseFromString(message)) {
			std::cerr << "Failed to parse message" << std::endl;
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
			response.mutable_status()->set_message("Unknown request type");
			break;
		}

		std::string responseStr;
		if (response.SerializeToString(&responseStr)) {
			server->SendMessage(responseStr);
		}
	}

	void HandleGetWindowList(windowcaster::ServerResponse& response) {
		auto windows = windowManager->EnumerateWindows();
		auto* windowList = response.mutable_window_list();

		for (const auto& window : windows) {
			auto* windowInfo = windowList->add_windows();
			windowInfo->set_handle(reinterpret_cast<uint64_t>(window.handle));

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

	void HandleRenderCommand(const windowcaster::RenderCommand& command,
		windowcaster::ServerResponse& response) {
		HWND hwnd = reinterpret_cast<HWND>(command.target_window());
		auto* status = response.mutable_status();

		if (!ValidateAndInitialize(hwnd, status)) {
			return;
		}

		bool success = false;
		switch (command.content_case()) {
		case windowcaster::RenderCommand::kImage: {
			const auto& image = command.image();
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
			status->set_message("Unknown render content type");
			return;
		}

		status->set_success(success);
		if (!success) {
			status->set_message("Render failed");
		}
	}

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

int main(int argc, char* argv[]) {
	try {
		GOOGLE_PROTOBUF_VERIFY_VERSION;

		// Register signal handler for Ctrl+C (SIGINT)
		std::signal(SIGINT, signalHandler);

		uint16_t port = 12345;  // Default port
		if (argc > 1) {
			port = static_cast<uint16_t>(std::stoi(argv[1]));
		}

		WindowCasterServer server(port);
		if (!server.Start()) {
			std::cerr << "Server failed to start" << std::endl;
			return 1;
		}

		std::cout << "WindowCaster server started on port " << port << "..." << std::endl;
		std::cout << "Press Ctrl+C to exit" << std::endl;

		// Loop until Ctrl+C is pressed
		while (!gSignalStatus) {
			Sleep(100); // Sleep for 100 milliseconds
		}

		server.Stop();
		google::protobuf::ShutdownProtobufLibrary();
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

