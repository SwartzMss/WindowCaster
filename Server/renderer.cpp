#include "renderer.h"
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <windows.h>

Renderer::Renderer()
	: targetWindow(nullptr)
	, windowDC(nullptr)
	, memoryDC(nullptr)
	, bitmap(nullptr)
	, currentWidth(0)
	, currentHeight(0)
	, gdiplusToken(0) {

	// ��ʼ�� GDI+
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	auto status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
	if (status != Gdiplus::Ok) {
		std::cout << "GDI+ ��ʼ��ʧ��: " << static_cast<int>(status) << std::endl;
		throw std::runtime_error("GDI+ ��ʼ��ʧ��");
	}
	std::cout << "��Ⱦ����ʼ���ɹ�" << std::endl;
}

Renderer::~Renderer() {
	Cleanup();
	// �ر� GDI+
	if (gdiplusToken != 0) {
		Gdiplus::GdiplusShutdown(gdiplusToken);
		std::cout << "GDI+ �ѹر�" << std::endl;
	}
}

bool Renderer::Initialize(HWND targetWindow) {
	if (!targetWindow || !IsWindow(targetWindow)) {
		std::cout << "��Ч�Ĵ��ھ��" << std::endl;
		return false;
	}

	Cleanup();
	this->targetWindow = targetWindow;

	// ��ȡ���� DC
	windowDC = GetDC(targetWindow);
	if (!windowDC) {
		std::cout << "��ȡ���� DC ʧ��" << std::endl;
		return false;
	}

	// �����ڴ� DC
	memoryDC = CreateCompatibleDC(windowDC);
	if (!memoryDC) {
		std::cout << "�����ڴ� DC ʧ��" << std::endl;
		ReleaseDC(targetWindow, windowDC);
		windowDC = nullptr;
		return false;
	}

	// ��Ŀ�괰����ǰ
	if (IsIconic(targetWindow)) {
		// ������ڱ���С�����Ȼָ���
		ShowWindow(targetWindow, SW_RESTORE);
	}
	SetForegroundWindow(targetWindow);

	std::cout << "��Ⱦ����ʼ����ɣ�Ŀ�괰��: 0x"
		<< std::hex << reinterpret_cast<uintptr_t>(targetWindow)
		<< std::dec << std::endl;
	return true;
}

bool Renderer::CreateCompatibleBitmap(size_t width, size_t height) {
	if (!windowDC || !memoryDC) {
		return false;
	}

	// ����ߴ�û����λͼ�Ѵ��ڣ�ֱ�ӷ���
	if (bitmap && width == currentWidth && height == currentHeight) {
		return true;
	}

	// ɾ���ɵ�λͼ
	if (bitmap) {
		DeleteObject(bitmap);
		bitmap = nullptr;
	}

	// �����µ�λͼ
	bitmap = ::CreateCompatibleBitmap(windowDC, static_cast<int>(width), static_cast<int>(height));
	if (!bitmap) {
		return false;
	}

	// ѡ��λͼ���ڴ� DC
	SelectObject(memoryDC, bitmap);
	currentWidth = width;
	currentHeight = height;

	return true;
}

bool Renderer::RenderImageFrame(const void* imageData, size_t width, size_t height) {
	if (!windowDC || !memoryDC || !targetWindow) {
		std::cout << "��Ⱦ��δ��ȷ��ʼ��" << std::endl;
		return false;
	}

	// ��������¼���λͼ
	if (!CreateCompatibleBitmap(width, height)) {
		std::cout << "��������λͼʧ�� width = " << width << " ,height = " << height << std::endl;
		return false;
	}

	// �� imageData ���Ƶ�λͼ������ imageData Ϊ RGB24 ��ʽ��
	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = static_cast<LONG>(width);
	bmi.bmiHeader.biHeight = -static_cast<LONG>(height); // ��ֵ��ʾ���϶���
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;

	if (!SetDIBits(windowDC, bitmap, 0, height, imageData, &bmi, DIB_RGB_COLORS)) {
		std::cout << "����ͼ������ʧ�� width = " << width << " ,height = " << height << std::endl;
		return false;
	}

	// ��ȡ���ڿͻ�����С
	RECT rect;
	GetClientRect(targetWindow, &rect);
	int windowWidth = rect.right - rect.left;
	int windowHeight = rect.bottom - rect.top;

	// ��������ģʽ
	SetStretchBltMode(windowDC, HALFTONE);
	SetBrushOrgEx(windowDC, 0, 0, nullptr);

	// ���ڴ� DC �е�ͼ��������Ƶ�����
	bool success = StretchBlt(
		windowDC, 0, 0, windowWidth, windowHeight,
		memoryDC, 0, 0, static_cast<int>(width), static_cast<int>(height),
		SRCCOPY
	) != 0;

	if (success) {
		std::cout << "ͼƬ��Ⱦ�ɹ�" << std::endl;
	}
	else {
		std::cout << "ͼƬ��Ⱦʧ��" << std::endl;
	}
	return success;
}

bool Renderer::RenderVideoFrame(const void* frameData, size_t width, size_t height) {
	// �˴��� RenderImageFrame ���ƣ�����ʵ�ָ�����Ƶ֡���ݸ�ʽȷ��
	return RenderImageFrame(frameData, width, height);
}

void Renderer::Clear() {
	if (windowDC && targetWindow) {
		RECT rect;
		GetClientRect(targetWindow, &rect);
		FillRect(windowDC, &rect, (HBRUSH)GetStockObject(BLACK_BRUSH));
	}
}

void Renderer::Cleanup() {
	if (bitmap) {
		DeleteObject(bitmap);
		bitmap = nullptr;
	}

	if (memoryDC) {
		DeleteDC(memoryDC);
		memoryDC = nullptr;
	}

	if (windowDC && targetWindow) {
		ReleaseDC(targetWindow, windowDC);
		windowDC = nullptr;
	}

	targetWindow = nullptr;
	currentWidth = 0;
	currentHeight = 0;
}

std::wstring Renderer::StringToWString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	// ��������Ŀ��ַ���
	int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	if (size == 0) {
		return std::wstring();
	}

	// ת���ַ���
	std::wstring result(size - 1, 0); // size-1 ��Ϊ����Ҫ��β�� null �ַ�
	if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size) == 0) {
		return std::wstring();
	}

	return result;
}
