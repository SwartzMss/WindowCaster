#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <string>
#include <thread>
#include <functional>
#include <memory>


class NetworkServer {
public:
	NetworkServer(uint16_t port = 12345);
	~NetworkServer();

	// 启动服务器
	bool Start();

	// 停止服务器
	void Stop();

	// 设置消息处理回调
	void SetMessageHandler(std::function<void(const std::string&)> handler);

	// 发送消息到客户端
	bool SendMessage(const std::string& message);

private:
	uint16_t port;
	bool running;
	SOCKET serverSocket;
	SOCKET clientSocket;
	std::thread listenThread;
	std::function<void(const std::string&)> messageHandler;

	// 监听线程函数
	void ListenThread();

	// 初始化 WSA
	bool InitializeWSA();

	// 清理资源
	void Cleanup();
};
