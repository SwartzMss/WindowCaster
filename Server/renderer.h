#pragma once

#include <string>
#define GDIPVER 0x0110
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

class Renderer {
public:
	Renderer();
	~Renderer();

	// ��ʼ����Ⱦ��
	bool Initialize(HWND targetWindow);

	// ��ȾͼƬ������ͼƬ����Ϊ���������ݣ���Ҫʵ�� RenderImageFrame �ӿڣ�
	bool RenderImageFrame(const void* imageData, size_t width, size_t height);

	// ��Ⱦ��Ƶ֡
	bool RenderVideoFrame(const void* frameData, size_t width, size_t height);

	// �����Ⱦ����
	void Clear();

private:
	HWND targetWindow;
	HDC windowDC;
	HDC memoryDC;
	HBITMAP bitmap;
	size_t currentWidth;
	size_t currentHeight;
	ULONG_PTR gdiplusToken;

	// �������ݵ�λͼ
	bool CreateCompatibleBitmap(size_t width, size_t height);

	// ������Դ
	void Cleanup();

	// �� std::string ת��Ϊ std::wstring
	std::wstring StringToWString(const std::string& str);
};
