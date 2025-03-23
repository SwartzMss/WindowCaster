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

	// ����������
	bool Start();

	// ֹͣ������
	void Stop();

	// ������Ϣ����ص�
	void SetMessageHandler(std::function<void(const std::string&)> handler);

	// ������Ϣ���ͻ���
	bool SendMessage(const std::string& message);

private:
	uint16_t port;
	bool running;
	SOCKET serverSocket;
	SOCKET clientSocket;
	std::thread listenThread;
	std::function<void(const std::string&)> messageHandler;

	// �����̺߳���
	void ListenThread();

	// ��ʼ�� WSA
	bool InitializeWSA();

	// ������Դ
	void Cleanup();
};
