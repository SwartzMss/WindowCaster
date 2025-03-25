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

	// 初始化 GDI+
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	auto status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
	if (status != Gdiplus::Ok) {
		std::cout << "GDI+ 初始化失败: " << static_cast<int>(status) << std::endl;
		throw std::runtime_error("GDI+ 初始化失败");
	}
	std::cout << "渲染器初始化成功" << std::endl;
}

Renderer::~Renderer() {
	Cleanup();
	// 关闭 GDI+
	if (gdiplusToken != 0) {
		Gdiplus::GdiplusShutdown(gdiplusToken);
		std::cout << "GDI+ 已关闭" << std::endl;
	}
}

bool Renderer::Initialize(HWND targetWindow) {
	if (!targetWindow || !IsWindow(targetWindow)) {
		std::cout << "无效的窗口句柄" << std::endl;
		return false;
	}

	Cleanup();
	this->targetWindow = targetWindow;

	// 获取窗口 DC
	windowDC = GetDC(targetWindow);
	if (!windowDC) {
		std::cout << "获取窗口 DC 失败" << std::endl;
		return false;
	}

	// 创建内存 DC
	memoryDC = CreateCompatibleDC(windowDC);
	if (!memoryDC) {
		std::cout << "创建内存 DC 失败" << std::endl;
		ReleaseDC(targetWindow, windowDC);
		windowDC = nullptr;
		return false;
	}

	// 将目标窗口置前
	if (IsIconic(targetWindow)) {
		// 如果窗口被最小化，先恢复它
		ShowWindow(targetWindow, SW_RESTORE);
	}
	SetForegroundWindow(targetWindow);

	std::cout << "渲染器初始化完成，目标窗口: 0x"
		<< std::hex << reinterpret_cast<uintptr_t>(targetWindow)
		<< std::dec << std::endl;
	return true;
}

bool Renderer::CreateCompatibleBitmap(size_t width, size_t height) {
	if (!windowDC || !memoryDC) {
		return false;
	}

	// 如果尺寸没变且位图已存在，直接返回
	if (bitmap && width == currentWidth && height == currentHeight) {
		return true;
	}

	// 删除旧的位图
	if (bitmap) {
		DeleteObject(bitmap);
		bitmap = nullptr;
	}

	// 创建新的位图
	bitmap = ::CreateCompatibleBitmap(windowDC, static_cast<int>(width), static_cast<int>(height));
	if (!bitmap) {
		return false;
	}

	// 选择位图到内存 DC
	SelectObject(memoryDC, bitmap);
	currentWidth = width;
	currentHeight = height;

	return true;
}

bool Renderer::RenderImageFrame(const void* imageData, size_t width, size_t height) {
	if (!windowDC || !memoryDC || !targetWindow) {
		std::cout << "渲染器未正确初始化" << std::endl;
		return false;
	}

	// 创建或更新兼容位图
	if (!CreateCompatibleBitmap(width, height)) {
		std::cout << "创建兼容位图失败 width = " << width << " ,height = " << height << std::endl;
		return false;
	}

	// 将 imageData 复制到位图（假设 imageData 为 RGB24 格式）
	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = static_cast<LONG>(width);
	bmi.bmiHeader.biHeight = -static_cast<LONG>(height); // 负值表示自上而下
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;

	if (!SetDIBits(windowDC, bitmap, 0, height, imageData, &bmi, DIB_RGB_COLORS)) {
		std::cout << "复制图像数据失败 width = " << width << " ,height = " << height << std::endl;
		return false;
	}

	// 获取窗口客户区大小
	RECT rect;
	GetClientRect(targetWindow, &rect);
	int windowWidth = rect.right - rect.left;
	int windowHeight = rect.bottom - rect.top;

	// 设置拉伸模式
	SetStretchBltMode(windowDC, HALFTONE);
	SetBrushOrgEx(windowDC, 0, 0, nullptr);

	// 将内存 DC 中的图像拉伸绘制到窗口
	bool success = StretchBlt(
		windowDC, 0, 0, windowWidth, windowHeight,
		memoryDC, 0, 0, static_cast<int>(width), static_cast<int>(height),
		SRCCOPY
	) != 0;

	if (success) {
		std::cout << "图片渲染成功" << std::endl;
	}
	else {
		std::cout << "图片渲染失败" << std::endl;
	}
	return success;
}

bool Renderer::RenderVideoFrame(const void* frameData, size_t width, size_t height) {
	// 此处与 RenderImageFrame 类似，具体实现根据视频帧数据格式确定
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

	// 计算所需的宽字符数
	int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	if (size == 0) {
		return std::wstring();
	}

	// 转换字符串
	std::wstring result(size - 1, 0); // size-1 因为不需要结尾的 null 字符
	if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size) == 0) {
		return std::wstring();
	}

	return result;
}
