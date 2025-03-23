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

	// ö�����пɼ�����
	std::vector<WindowInfo> EnumerateWindows();

	// ��ȡָ�����ڵ��豸������
	HDC GetWindowDC(HWND hwnd);

	// �ͷ��豸������
	void ReleaseWindowDC(HWND hwnd, HDC hdc);

	// ��鴰���Ƿ���Ч
	bool IsWindowValid(HWND hwnd);

private:
	static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
	std::vector<WindowInfo> windowList;
};
