#include "window_manager.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

WindowManager::WindowManager() {}

WindowManager::~WindowManager() {}

std::vector<WindowManager::WindowInfo> WindowManager::EnumerateWindows() {
	windowList.clear();
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(this));
	return windowList;
}

HDC WindowManager::GetWindowDC(HWND hwnd) {
	if (!IsWindowValid(hwnd)) {
		return nullptr;
	}
	return ::GetDC(hwnd);
}

void WindowManager::ReleaseWindowDC(HWND hwnd, HDC hdc) {
	if (hwnd && hdc) {
		::ReleaseDC(hwnd, hdc);
	}
}

bool WindowManager::IsWindowValid(HWND hwnd) {
	if (!hwnd || !IsWindow(hwnd)) {
		return false;
	}

	// ��鴰���Ƿ�ɼ�
	if (!IsWindowVisible(hwnd)) {
		return false;
	}

	// ��ȡ���ڱ���
	wchar_t title[256];
	if (GetWindowTextW(hwnd, title, 256) == 0) {
		return false;
	}

	// ����Ƿ�Ϊ UWP Ӧ�ô���
	BOOL isUWP = FALSE;
	if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &isUWP, sizeof(isUWP))) && isUWP) {
		return false;
	}

	return true;
}

BOOL CALLBACK WindowManager::EnumWindowsProc(HWND hwnd, LPARAM lParam) {
	auto* self = reinterpret_cast<WindowManager*>(lParam);

	if (!self->IsWindowValid(hwnd)) {
		return TRUE; // ����ö��
	}

	WindowInfo info;
	info.handle = hwnd;

	// ��ȡ���ڱ���
	wchar_t title[256];
	GetWindowTextW(hwnd, title, 256);
	info.title = title;

	// ��ȡ��������
	wchar_t className[256];
	GetClassNameW(hwnd, className, 256);
	info.className = className;

	self->windowList.push_back(info);
	return TRUE;
}
