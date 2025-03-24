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
		std::cerr << "WSAStartup ʧ��" << std::endl;
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

	// �����������׽���
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "�����׽���ʧ��" << std::endl;
		return false;
	}

	// ���õ�ַ����
	int reuseAddr = 1;
	if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR,
		reinterpret_cast<char*>(&reuseAddr), sizeof(reuseAddr)) == SOCKET_ERROR) {
		std::cerr << "�����׽���ѡ��ʧ��" << std::endl;
		Cleanup();
		return false;
	}

	// �󶨵�ַ
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr),
		sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "�󶨵�ַʧ��" << std::endl;
		Cleanup();
		return false;
	}

	// ��ʼ����
	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "����ʧ��" << std::endl;
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
	// һ����ʱ���壬ÿ��recv���������µ�������
	std::vector<char> buffer(4096);
	// һ�������������壬����ƴ�Ӷ��recv�����ݣ�����һ��recv�����������Ϣ������ֻ��������Ϣ
	std::vector<char> pending;

	while (running) {
		sockaddr_in clientAddr;
		int clientAddrLen = sizeof(clientAddr);

		SOCKET newClient = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
		if (newClient == INVALID_SOCKET) {
			if (running) {
				std::cerr << "��������ʧ��" << std::endl;
			}
			continue;
		}

		// ���Ѵ��ھɿͻ��ˣ����ȹص���
		if (clientSocket != INVALID_SOCKET) {
			closesocket(clientSocket);
		}
		clientSocket = newClient;
		std::cout << "�¿ͻ�������" << std::endl;

		// ���ϴ��¿ͻ��˶�ȡ����
		while (running && clientSocket != INVALID_SOCKET) {
			int bytesReceived = recv(clientSocket, buffer.data(), static_cast<int>(buffer.size()), 0);
			if (bytesReceived > 0) {
				// �������յ������� append �� pending
				pending.insert(pending.end(), buffer.begin(), buffer.begin() + bytesReceived);

				// ����һ���յ��˶�����Ϣ������ֻ��һ����Ϣ��һ����
				// ����Ҫ��һ��ѭ�����������
				while (true) {
					// �ж� pending ���Ƿ����ٻ���4�ֽڿɶ� => ˵�������г���ǰ׺
					if (pending.size() < 4) {
						// ���ݲ�����ȡ������ǰ׺�����´� recv
						break;
					}

					// ����ǰ4�ֽڣ����ǿͻ��˷����� "��Ϣ�峤��"
					uint32_t msgLen = 0;
					// ����ͻ����õ���С���� to_le_bytes��
					// �ҷ�����ͬ����С�˻�������ֱ�� memcpy����Ҫ�����տ�����һ���ֽ���ת��
					std::memcpy(&msgLen, &pending[0], 4);
					// �����Ҫ���ã�msgLen = _byteswap_ulong(msgLen); �����ƺ���
					// �ӿͻ��˷��Ͷ˵Ĵ�С���������

					if (pending.size() < 4 + msgLen) {
						// ���ݻ�û��ȫ���ȵ�һ��
						break;
					}

					// �����˵���Ѿ������� (4 + msgLen) �ֽ� => ������ȡ��һ����������Ϣ
					std::string oneProtoMsg(pending.begin() + 4, pending.begin() + 4 + msgLen);

					// �Ƴ��Ѷ��ߵ�����
					pending.erase(pending.begin(), pending.begin() + 4 + msgLen);

					// �ص��� Handler������ȥ ParseFromString(oneProtoMsg)
					if (messageHandler) {
						messageHandler(oneProtoMsg);
					}

					// ����ѭ�������� pending ���Ƿ�ʣ�������ݣ����Խ�����һ��
				}

			}
			else if (bytesReceived == 0) {
				std::cout << "�ͻ��˶Ͽ�����" << std::endl;
				break;
			}
			else {
				std::cerr << "��������ʧ��" << std::endl;
				break;
			}
		}

		closesocket(clientSocket);
		clientSocket = INVALID_SOCKET;
		pending.clear();  // ����¸��ͻ������ӣ���Ҫ�ӿտ�ʼƴ��
	}
}


bool NetworkServer::SendMessage(const std::string& message) {
	if (clientSocket == INVALID_SOCKET) {
		return false;
	}

	// 1) ��׼��4�ֽڱ�ʾ��Ϣ�峤�ȣ�С����
	uint32_t len = static_cast<uint32_t>(message.size());
	// �������Ҫ����ȷ��תС�ˣ�Ҳ���ԣ�
	// uint32_t len_le = htole32(len);
	// ����� memcpy(&len_le, ...) 
	// �����������������С�ˣ�ֱ�� memcpy(len) Ҳ��
	char prefix[4];
	std::memcpy(prefix, &len, sizeof(len));

	// 2) �ȷ���4�ֽ�ǰ׺
	int bytesSent = send(clientSocket, prefix, 4, 0);
	if (bytesSent == SOCKET_ERROR) {
		std::cerr << "���ͳ���ǰ׺ʧ��" << std::endl;
		return false;
	}

	// 3) �ٷ���ʵ�ʵ���Ϣ����
	bytesSent = send(clientSocket, message.data(), static_cast<int>(message.size()), 0);
	if (bytesSent == SOCKET_ERROR) {
		std::cerr << "������Ϣ����ʧ��" << std::endl;
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
