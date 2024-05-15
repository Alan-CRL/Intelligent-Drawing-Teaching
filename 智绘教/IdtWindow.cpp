#include "IdtWindow.h"

#include "IdtDraw.h"
#include "IdtDrawpad.h"
#include "IdtRts.h"
#include "IdtText.h"

HWND floating_window = NULL; //悬浮窗窗口
HWND drawpad_window = NULL; //画板窗口
HWND ppt_window = NULL; //PPT控件窗口
HWND freeze_window = NULL; //定格背景窗口
HWND setting_window = NULL; //程序调测窗口

bool FreezePPT;
HWND ppt_show;
wstring ppt_title, ppt_software;
map<wstring, bool> ppt_title_recond;

//窗口是否置顶
bool IsWindowFocused(HWND hWnd)
{
	return GetForegroundWindow() == hWnd;
}

HWND GetLastFocusWindow()
{
	GUITHREADINFO guiThreadInfo;
	guiThreadInfo.cbSize = sizeof(GUITHREADINFO);
	if (GetGUIThreadInfo(0, &guiThreadInfo))
	{
		return guiThreadInfo.hwndFocus;
	}
	return NULL;
}
wstring GetWindowText(HWND hWnd)
{
	// 获取窗口标题的长度
	int length = GetWindowTextLength(hWnd);

	// 创建一个足够大的缓冲区来存储窗口标题
	std::vector<wchar_t> buffer(length + 1);

	// 获取窗口标题
	GetWindowText(hWnd, &buffer[0], buffer.size());

	// 返回窗口标题
	return &buffer[0];
}

IdtWindowsIsVisibleStruct IdtWindowsIsVisible;

//置顶程序窗口
void TopWindow()
{
	// 等待窗口绘制
	IDTLogger->info("[窗口置顶线程][TopWindow] 等待窗口初次绘制");
	for (int i = 1; i <= 20; i++)
	{
		this_thread::sleep_for(chrono::milliseconds(500));

		bool flag = true;
		if (flag && !IdtWindowsIsVisible.floatingWindow) flag = false;
		if (flag && !IdtWindowsIsVisible.pptWindow) flag = false;
		if (flag && !IdtWindowsIsVisible.drawpadWindow) flag = false;
		if (flag && !IdtWindowsIsVisible.freezeWindow) flag = false;

		if (flag)
		{
			IdtWindowsIsVisible.allCompleted = true;
			break;
		}
	}

	// 状态超时 -> 结束程序
	if (!IdtWindowsIsVisible.allCompleted)
	{
		IDTLogger->warn("[窗口置顶线程][TopWindow] 等待窗口初次绘制超时");

		offSignal = true;
		return;
	}
	IDTLogger->info("[窗口置顶线程][TopWindow] 等待窗口初次绘制完成");

	IDTLogger->info("[窗口置顶线程][TopWindow] 显示窗口");
	ShowWindow(floating_window, SW_SHOW);
	ShowWindow(ppt_window, SW_SHOW);
	ShowWindow(drawpad_window, SW_SHOW);
	ShowWindow(freeze_window, SW_SHOW);
	IDTLogger->info("[窗口置顶线程][TopWindow] 显示窗口完成");

	while (uRealTimeStylus != 1) this_thread::sleep_for(chrono::milliseconds(500));
	while (!offSignal)
	{
		// 检查窗口显示状态
		{
			if (!IsWindowVisible(floating_window)) ShowWindow(floating_window, SW_SHOW);
			if (!IsWindowVisible(ppt_window)) ShowWindow(ppt_window, SW_SHOW);
			if (!IsWindowVisible(drawpad_window)) ShowWindow(drawpad_window, SW_SHOW);
			if (!IsWindowVisible(freeze_window)) ShowWindow(freeze_window, SW_SHOW);
		}

		// 置顶窗口
		if (!choose.select)
		{
			std::shared_lock<std::shared_mutex> lock1(StrokeImageListSm);
			bool flag = StrokeImageList.empty();
			lock1.unlock();
			if (flag)
			{
				if (!SetWindowPos(freeze_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
					IDTLogger->warn("[窗口置顶线程][TopWindow] 置顶窗口时失败 Error" + to_string(GetLastError()));
			}
		}
		else
		{
			if (!SetWindowPos(freeze_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE))
				IDTLogger->warn("[窗口置顶线程][TopWindow] 置顶窗口时失败 Error" + to_string(GetLastError()));
		}

		// 检查置顶情况
		if (!(GetWindowLong(freeze_window, GWL_EXSTYLE) & WS_EX_TOPMOST))
		{
			IDTLogger->warn("[窗口置顶线程][TopWindow] 置顶窗口失败");
			IDTLogger->info("[窗口置顶线程][TopWindow] 强制置顶窗口");

			HWND hForeWnd = GetForegroundWindow();
			DWORD dwForeID = GetCurrentThreadId();
			DWORD dwCurID = GetWindowThreadProcessId(hForeWnd, NULL);
			AttachThreadInput(dwCurID, dwForeID, TRUE);
			SetWindowPos(freeze_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			SetWindowPos(freeze_window, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			SetForegroundWindow(freeze_window);
			AttachThreadInput(dwCurID, dwForeID, FALSE);

			SetWindowPos(freeze_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

			IDTLogger->info("[窗口置顶线程][TopWindow] 强制置顶窗口完成");
		}

		// 延迟等待
		for (int i = 1; i <= 10; i++)
		{
			if (offSignal) break;
			this_thread::sleep_for(chrono::milliseconds(100));
		}
	}

	IDTLogger->info("[窗口置顶线程][TopWindow] 隐藏窗口");
	ShowWindow(floating_window, SW_HIDE);
	ShowWindow(ppt_window, SW_HIDE);
	ShowWindow(drawpad_window, SW_HIDE);
	ShowWindow(freeze_window, SW_HIDE);
	IDTLogger->info("[窗口置顶线程][TopWindow] 隐藏窗口完成");

	return;
}