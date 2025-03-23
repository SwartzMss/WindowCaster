#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <functional>

class WindowManager {
public:
	struct WindowInfo {
		HWND handle;
		std::wstring title;
		std::wstring className;
	};

	WindowManager();
	~WindowManager();

	// 枚举所有可见窗口
	std::vector<WindowInfo> EnumerateWindows();

	// 获取指定窗口的设备上下文
	HDC GetWindowDC(HWND hwnd);

	// 释放设备上下文
	void ReleaseWindowDC(HWND hwnd, HDC hdc);

	// 检查窗口是否有效
	bool IsWindowValid(HWND hwnd);

private:
	static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
	std::vector<WindowInfo> windowList;
};
