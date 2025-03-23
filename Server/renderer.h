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

	// 初始化渲染器
	bool Initialize(HWND targetWindow);

	// 渲染图片（假设图片数据为二进制数据，需要实现 RenderImageFrame 接口）
	bool RenderImageFrame(const void* imageData, size_t width, size_t height);

	// 渲染视频帧
	bool RenderVideoFrame(const void* frameData, size_t width, size_t height);

	// 清除渲染内容
	void Clear();

private:
	HWND targetWindow;
	HDC windowDC;
	HDC memoryDC;
	HBITMAP bitmap;
	size_t currentWidth;
	size_t currentHeight;
	ULONG_PTR gdiplusToken;

	// 创建兼容的位图
	bool CreateCompatibleBitmap(size_t width, size_t height);

	// 清理资源
	void Cleanup();

	// 将 std::string 转换为 std::wstring
	std::wstring StringToWString(const std::string& str);
};
