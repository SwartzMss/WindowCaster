#include "network_server.h"
#include <iostream>
#include <vector>

NetworkServer::NetworkServer(uint16_t port)
	: port(port)
	, running(false)
	, serverSocket(INVALID_SOCKET)
	, clientSocket(INVALID_SOCKET) {
}

NetworkServer::~NetworkServer() {
	Stop();
}

bool NetworkServer::InitializeWSA() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup 失败" << std::endl;
		return false;
	}
	return true;
}

bool NetworkServer::Start() {
	if (running) {
		return true;
	}

	if (!InitializeWSA()) {
		return false;
	}

	// 创建服务器套接字
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "创建套接字失败" << std::endl;
		return false;
	}

	// 设置地址重用
	int reuseAddr = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR,
		reinterpret_cast<char*>(&reuseAddr), sizeof(reuseAddr)) == SOCKET_ERROR) {
		std::cerr << "设置套接字选项失败" << std::endl;
		Cleanup();
		return false;
	}

	// 绑定地址
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr),
		sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "绑定地址失败" << std::endl;
		Cleanup();
		return false;
	}

	// 开始监听
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "监听失败" << std::endl;
		Cleanup();
		return false;
	}

	running = true;
	listenThread = std::thread(&NetworkServer::ListenThread, this);

	return true;
}

void NetworkServer::Stop() {
	running = false;

	if (serverSocket != INVALID_SOCKET) {
		closesocket(serverSocket);
		serverSocket = INVALID_SOCKET;
	}

	if (clientSocket != INVALID_SOCKET) {
		closesocket(clientSocket);
		clientSocket = INVALID_SOCKET;
	}

	if (listenThread.joinable()) {
		listenThread.join();
	}

	WSACleanup();
}

void NetworkServer::ListenThread() {
	std::vector<char> buffer(4096);

	while (running) {
		sockaddr_in clientAddr;
		int clientAddrLen = sizeof(clientAddr);

		SOCKET newClient = accept(serverSocket,
			reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);

		if (newClient == INVALID_SOCKET) {
			if (running) {
				std::cerr << "接受连接失败" << std::endl;
			}
			continue;
		}

		// 关闭之前的客户端连接
		if (clientSocket != INVALID_SOCKET) {
			closesocket(clientSocket);
		}

		clientSocket = newClient;
		std::cout << "新客户端连接" << std::endl;

		// 处理客户端消息
		while (running && clientSocket != INVALID_SOCKET) {
			int bytesReceived = recv(clientSocket, buffer.data(),
				static_cast<int>(buffer.size()), 0);

			if (bytesReceived > 0) {
				if (messageHandler) {
					messageHandler(std::string(buffer.data(), bytesReceived));
				}
			}
			else if (bytesReceived == 0) {
				std::cout << "客户端断开连接" << std::endl;
				break;
			}
			else {
				std::cerr << "接收数据失败" << std::endl;
				break;
			}
		}

		closesocket(clientSocket);
		clientSocket = INVALID_SOCKET;
	}
}

bool NetworkServer::SendMessage(const std::string& message) {
	if (clientSocket == INVALID_SOCKET) {
		return false;
	}

	int bytesSent = send(clientSocket, message.c_str(),
		static_cast<int>(message.length()), 0);

	return bytesSent != SOCKET_ERROR;
}

void NetworkServer::SetMessageHandler(std::function<void(const std::string&)> handler) {
	messageHandler = std::move(handler);
}

void NetworkServer::Cleanup() {
	if (serverSocket != INVALID_SOCKET) {
		closesocket(serverSocket);
		serverSocket = INVALID_SOCKET;
	}

	if (clientSocket != INVALID_SOCKET) {
		closesocket(clientSocket);
		clientSocket = INVALID_SOCKET;
	}

	WSACleanup();
}
