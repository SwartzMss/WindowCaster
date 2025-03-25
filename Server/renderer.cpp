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

	// Initialize GDI+
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	auto status = Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
	if (status != Gdiplus::Ok) {
		std::cout << "GDI+ initialization failed: " << static_cast<int>(status) << std::endl;
		throw std::runtime_error("GDI+ initialization failed");
	}
	std::cout << "Renderer initialized successfully" << std::endl;
}

Renderer::~Renderer() {
	Cleanup();
	// Shutdown GDI+
	if (gdiplusToken != 0) {
		Gdiplus::GdiplusShutdown(gdiplusToken);
		std::cout << "GDI+ has been shutdown" << std::endl;
	}
}

bool Renderer::Initialize(HWND targetWindow) {
	if (!targetWindow || !IsWindow(targetWindow)) {
		std::cout << "Invalid window handle" << std::endl;
		return false;
	}

	Cleanup();
	this->targetWindow = targetWindow;

	// Get window DC
	windowDC = GetDC(targetWindow);
	if (!windowDC) {
		std::cout << "Failed to get window DC" << std::endl;
		return false;
	}

	// Create memory DC
	memoryDC = CreateCompatibleDC(windowDC);
	if (!memoryDC) {
		std::cout << "Failed to create memory DC" << std::endl;
		ReleaseDC(targetWindow, windowDC);
		windowDC = nullptr;
		return false;
	}

	// Attempt to attach the current thread to the target window's thread
	DWORD currentThreadID = GetCurrentThreadId();
	DWORD targetThreadID = GetWindowThreadProcessId(targetWindow, nullptr);
	if (currentThreadID != targetThreadID) {
		AttachThreadInput(currentThreadID, targetThreadID, TRUE);
	}

	if (IsIconic(targetWindow)) {
		ShowWindow(targetWindow, SW_RESTORE);
	}

	// Temporarily set the window as topmost
	SetWindowPos(targetWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	// Attempt to bring the window to the foreground
	SetForegroundWindow(targetWindow);
	// Restore the window to non-topmost
	SetWindowPos(targetWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	if (currentThreadID != targetThreadID) {
		AttachThreadInput(currentThreadID, targetThreadID, FALSE);
	}

	std::cout << "Renderer initialization completed, target window: 0x"
		<< std::hex << reinterpret_cast<uintptr_t>(targetWindow)
		<< std::dec << std::endl;
	return true;
}

bool Renderer::CreateCompatibleBitmap(size_t width, size_t height) {
	if (!windowDC || !memoryDC) {
		return false;
	}

	// If dimensions haven't changed and bitmap already exists, just return
	if (bitmap && width == currentWidth && height == currentHeight) {
		return true;
	}

	// Delete the old bitmap
	if (bitmap) {
		DeleteObject(bitmap);
		bitmap = nullptr;
	}

	// Create a new bitmap
	bitmap = ::CreateCompatibleBitmap(windowDC, static_cast<int>(width), static_cast<int>(height));
	if (!bitmap) {
		return false;
	}

	// Select the bitmap into the memory DC
	SelectObject(memoryDC, bitmap);
	currentWidth = width;
	currentHeight = height;

	return true;
}

bool Renderer::RenderImageFrame(const void* imageData, size_t width, size_t height) {
	if (!windowDC || !memoryDC || !targetWindow) {
		std::cout << "Renderer not properly initialized" << std::endl;
		return false;
	}

	// Create or update the compatible bitmap
	if (!CreateCompatibleBitmap(width, height)) {
		std::cout << "Failed to create compatible bitmap, width = " << width << " ,height = " << height << std::endl;
		return false;
	}

	// Copy imageData to bitmap (assuming imageData is in RGB24 format)
	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = static_cast<LONG>(width);
	bmi.bmiHeader.biHeight = -static_cast<LONG>(height); // Negative indicates top-down DIB
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biCompression = BI_RGB;

	if (!SetDIBits(windowDC, bitmap, 0, height, imageData, &bmi, DIB_RGB_COLORS)) {
		std::cout << "Failed to copy image data, width = " << width << " ,height = " << height << std::endl;
		return false;
	}

	// Get the client area size of the window
	RECT rect;
	GetClientRect(targetWindow, &rect);
	int windowWidth = rect.right - rect.left;
	int windowHeight = rect.bottom - rect.top;

	// Set stretch mode
	SetStretchBltMode(windowDC, HALFTONE);
	SetBrushOrgEx(windowDC, 0, 0, nullptr);

	// Stretch-blit the image from memory DC to the window
	bool success = StretchBlt(
		windowDC, 0, 0, windowWidth, windowHeight,
		memoryDC, 0, 0, static_cast<int>(width), static_cast<int>(height),
		SRCCOPY
	) != 0;

	if (success) {
		std::cout << "Image rendered successfully" << std::endl;
	}
	else {
		std::cout << "Image rendering failed" << std::endl;
	}
	return success;
}

bool Renderer::RenderVideoFrame(const void* frameData, size_t width, size_t height) {
	// This implementation is similar to RenderImageFrame; actual implementation depends on the video frame data format
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

	// Calculate the number of wide characters required
	int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
	if (size == 0) {
		return std::wstring();
	}

	// Convert the string
	std::wstring result(size - 1, 0); // size-1 because the terminating null is not needed
	if (MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size) == 0) {
		return std::wstring();
	}

	return result;
}
