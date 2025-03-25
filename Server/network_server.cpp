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
		std::cerr << "WSAStartup failed" << std::endl;
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

	// Create server socket
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Failed to create socket" << std::endl;
		return false;
	}

	// Set address reuse
	int reuseAddr = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR,
		reinterpret_cast<char*>(&reuseAddr), sizeof(reuseAddr)) == SOCKET_ERROR) {
		std::cerr << "Failed to set socket options" << std::endl;
		Cleanup();
		return false;
	}

	// Bind address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr),
		sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Failed to bind address" << std::endl;
		Cleanup();
		return false;
	}

	// Start listening
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Failed to listen" << std::endl;
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
	// Temporary buffer for each recv call
	std::vector<char> buffer(4096);
	// Pending buffer to accumulate data (in case one recv contains multiple or partial messages)
	std::vector<char> pending;

	while (running) {
		sockaddr_in clientAddr;
		int clientAddrLen = sizeof(clientAddr);

		SOCKET newClient = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
		if (newClient == INVALID_SOCKET) {
			if (running) {
				std::cerr << "Failed to accept connection" << std::endl;
			}
			continue;
		}

		// If there's an existing client, close it first
		if (clientSocket != INVALID_SOCKET) {
			closesocket(clientSocket);
		}
		clientSocket = newClient;
		std::cout << "New client connected" << std::endl;

		// Continuously read data from the new client
		while (running && clientSocket != INVALID_SOCKET) {
			int bytesReceived = recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
			if (bytesReceived > 0) {
				// Append received data to pending
				pending.insert(pending.end(), buffer.begin(), buffer.begin() + bytesReceived);

				// Process the pending buffer for complete messages
				while (true) {
					// Check if there are at least 4 bytes (the length prefix)
					if (pending.size() < 4) {
						break;
					}

					// Read the first 4 bytes as message length
					uint32_t msgLen = 0;
					std::memcpy(&msgLen, &pending[0], 4);

					if (pending.size() < 4 + msgLen) {
						// Not enough data for a complete message, wait for more data
						break;
					}

					// Extract a complete message
					std::string oneProtoMsg(pending.begin() + 4, pending.begin() + 4 + msgLen);

					// Remove processed data from pending
					pending.erase(pending.begin(), pending.begin() + 4 + msgLen);

					// Callback to the message handler to process the message
					if (messageHandler) {
						messageHandler(oneProtoMsg);
					}
				}

			}
			else if (bytesReceived == 0) {
				std::cout << "Client disconnected" << std::endl;
				break;
			}
			else {
				std::cerr << "Failed to receive data" << std::endl;
				break;
			}
		}

		closesocket(clientSocket);
		clientSocket = INVALID_SOCKET;
		pending.clear();  // Clear pending data for new client connections
	}
}

bool NetworkServer::SendMessage(const std::string& message) {
	if (clientSocket == INVALID_SOCKET) {
		return false;
	}

	// Prepare a 4-byte length prefix (little endian)
	uint32_t len = static_cast<uint32_t>(message.size());
	char prefix[4];
	std::memcpy(prefix, &len, sizeof(len));

	// Send the length prefix
	int bytesSent = send(clientSocket, prefix, 4, 0);
	if (bytesSent == SOCKET_ERROR) {
		std::cerr << "Failed to send length prefix" << std::endl;
		return false;
	}

	// Send the actual message content
	bytesSent = send(clientSocket, message.data(), static_cast<int>(message.size()), 0);
	if (bytesSent == SOCKET_ERROR) {
		std::cerr << "Failed to send message content" << std::endl;
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
