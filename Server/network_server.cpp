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
	// 一个临时缓冲，每次recv用它接收新到的数据
	std::vector<char> buffer(4096);
	// 一个“待处理”缓冲，用来拼接多次recv的数据，可能一次recv里包含多条消息，或者只含部分消息
	std::vector<char> pending;

	while (running) {
		sockaddr_in clientAddr;
		int clientAddrLen = sizeof(clientAddr);

		SOCKET newClient = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
		if (newClient == INVALID_SOCKET) {
			if (running) {
				std::cerr << "接受连接失败" << std::endl;
			}
			continue;
		}

		// 若已存在旧客户端，则先关掉它
		if (clientSocket != INVALID_SOCKET) {
			closesocket(clientSocket);
		}
		clientSocket = newClient;
		std::cout << "新客户端连接" << std::endl;

		// 不断从新客户端读取数据
		while (running && clientSocket != INVALID_SOCKET) {
			int bytesReceived = recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
			if (bytesReceived > 0) {
				// 将本次收到的数据 append 到 pending
				pending.insert(pending.end(), buffer.begin(), buffer.begin() + bytesReceived);

				// 可能一次收到了多条消息，或者只是一条消息的一部分
				// 所以要用一个循环来“拆包”
				while (true) {
					// 判断 pending 里是否至少还有4字节可读 => 说明可能有长度前缀
					if (pending.size() < 4) {
						// 数据不足以取出长度前缀，等下次 recv
						break;
					}

					// 读出前4字节：它是客户端发来的 "消息体长度"
					uint32_t msgLen = 0;
					// 假设客户端用的是小端序 to_le_bytes，
					// 且服务器同样是小端机器，可直接 memcpy。若要更保险可以做一次字节序转换
					std::memcpy(&msgLen, &pending[0], 4);
					// 如果需要可用：msgLen = _byteswap_ulong(msgLen); 或类似函数
					// 视客户端发送端的大小端情况而定

					if (pending.size() < 4 + msgLen) {
						// 数据还没收全，先等一下
						break;
					}

					// 到这里，说明已经至少有 (4 + msgLen) 字节 => 可以提取出一条完整的消息
					std::string oneProtoMsg(pending.begin() + 4, pending.begin() + 4 + msgLen);

					// 移除已读走的数据
					pending.erase(pending.begin(), pending.begin() + 4 + msgLen);

					// 回调给 Handler，让它去 ParseFromString(oneProtoMsg)
					if (messageHandler) {
						messageHandler(oneProtoMsg);
					}

					// 继续循环，看看 pending 里是否还剩多余数据，可以解析下一条
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
		pending.clear();  // 如果下个客户端连接，需要从空开始拼包
	}
}


bool NetworkServer::SendMessage(const std::string& message) {
	if (clientSocket == INVALID_SOCKET) {
		return false;
	}

	// 1) 先准备4字节表示消息体长度（小端序）
	uint32_t len = static_cast<uint32_t>(message.size());
	// 如果你需要更明确地转小端，也可以：
	// uint32_t len_le = htole32(len);
	// 下面就 memcpy(&len_le, ...) 
	// 但若服务器本身就是小端，直接 memcpy(len) 也行
	char prefix[4];
	std::memcpy(prefix, &len, sizeof(len));

	// 2) 先发送4字节前缀
	int bytesSent = send(clientSocket, prefix, 4, 0);
	if (bytesSent == SOCKET_ERROR) {
		std::cerr << "发送长度前缀失败" << std::endl;
		return false;
	}

	// 3) 再发送实际的消息内容
	bytesSent = send(clientSocket, message.data(), static_cast<int>(message.size()), 0);
	if (bytesSent == SOCKET_ERROR) {
		std::cerr << "发送消息内容失败" << std::endl;
		return false;
	}

	return true;
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
