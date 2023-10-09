#include "main.h"

int floating_main();
int drawpad_main();
int test_main();
void FreezeFrameWindow();
int main()
{
	//全局路径预处理
	{
		global_path = wstring_to_string(GetCurrentExeDirectory() + L"\\");
	}

	//DPI 初始化
	{
		HINSTANCE hUser32 = LoadLibrary(L"User32.dll");
		if (hUser32)
		{
			typedef BOOL(WINAPI* LPSetProcessDPIAware)(void);
			LPSetProcessDPIAware pSetProcessDPIAware = (LPSetProcessDPIAware)GetProcAddress(hUser32, "SetProcessDPIAware");
			if (pSetProcessDPIAware)
			{
				pSetProcessDPIAware();
			}
			FreeLibrary(hUser32);
		}
	}
	//字体初始化部分
	{
		HMODULE hModule = GetModuleHandle(NULL);
		HRSRC hResource = FindResource(hModule, MAKEINTRESOURCE(198), L"TTF");
		HGLOBAL hMemory = LoadResource(hModule, hResource);
		PVOID pResourceData = LockResource(hMemory);
		DWORD dwResourceSize = SizeofResource(hModule, hResource);
		fontCollection.AddMemoryFont(pResourceData, dwResourceSize);

		//filesystem::create_directory(string_to_wstring(global_path) + L"ttf");
		//ExtractResource((string_to_wstring(global_path) + L"ttf\\HarmonyOS_Sans_SC_Regular.ttf").c_str(), L"TTF", MAKEINTRESOURCE(198));
		//fontCollection.AddFontFile((string_to_wstring(global_path) + L"ttf\\HarmonyOS_Sans_SC_Regular.ttf").c_str());
		//filesystem::path directory((string_to_wstring(global_path) + L"ttf").c_str());
		//filesystem::remove_all(directory);

		INT numFound = 0;
		fontCollection.GetFamilies(1, &HarmonyOS_fontFamily, &numFound);

		//AddFontResourceEx((string_to_wstring(global_path) + L"ttf\\HarmonyOS_Sans_SC_Regular.ttf").c_str(), FR_PRIVATE, NULL);
		//AddFontResourceEx((string_to_wstring(global_path) + L"ttf\\Douyu_Font.otf").c_str(), FR_PRIVATE, NULL);
		//AddFontResourceEx((string_to_wstring(global_path) + L"ttf\\SmileySans-Oblique.ttf").c_str(), FR_PRIVATE, NULL);

		//wcscpy(font.lfFaceName, L"HarmonyOS Sans SC");
		//wcscpy(font.lfFaceName, L"DOUYU Gdiplus::Font");
		//wcscpy(font.lfFaceName, L"得意黑");

		stringFormat.SetAlignment(StringAlignmentCenter);
		stringFormat.SetLineAlignment(StringAlignmentCenter);

		stringFormat_left.SetAlignment(StringAlignmentNear);
		stringFormat_left.SetLineAlignment(StringAlignmentNear);
	}

	//程序更新判断
	{
		//当前程序为新版本
		if (_waccess((string_to_wstring(global_path) + L"update.json").c_str(), 4) == 0)
		{
			wstring tedition, representation;

			Json::Reader reader;
			Json::Value root;

			ifstream readjson;
			readjson.imbue(locale("zh_CN.UTF8"));
			readjson.open(wstring_to_string(string_to_wstring(global_path) + L"update.json").c_str());

			if (reader.parse(readjson, root))
			{
				tedition = string_to_wstring(convert_to_gbk(root["edition"].asString()));
				representation = string_to_wstring(convert_to_gbk(root["representation"].asString()));
			}

			readjson.close();

			if (tedition == string_to_wstring(edition_date))
			{
				//符合条件，开始替换版本

				Sleep(1000);

				filesystem::path directory(global_path);
				string main_path = directory.parent_path().parent_path().string();

				error_code ec;
				filesystem::remove(string_to_wstring(main_path) + L"\\智绘教.exe", ec);
				filesystem::copy_file(string_to_wstring(global_path) + representation, string_to_wstring(main_path) + L"\\智绘教.exe", std::filesystem::copy_options::overwrite_existing, ec);

				STARTUPINFOA si = { 0 };
				si.cb = sizeof(si);
				PROCESS_INFORMATION pi = { 0 };
				CreateProcessA(NULL, (main_path + "\\智绘教.exe").data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				return 0;
			}
		}
		//当前程序为旧版本
		if (_waccess((string_to_wstring(global_path) + L"installer\\update.json").c_str(), 4) == 0)
		{
			wstring tedition, path;
			string thash_md5, thash_sha256;

			Json::Reader reader;
			Json::Value root;

			ifstream readjson;
			readjson.imbue(locale("zh_CN.UTF8"));
			readjson.open(wstring_to_string(string_to_wstring(global_path) + L"installer\\update.json").c_str());

			if (reader.parse(readjson, root))
			{
				tedition = string_to_wstring(convert_to_gbk(root["edition"].asString()));
				path = string_to_wstring(convert_to_gbk(root["path"].asString()));

				thash_md5 = convert_to_gbk(root["hash"]["md5"].asString());
				thash_sha256 = convert_to_gbk(root["hash"]["sha256"].asString());
			}

			readjson.close();

			string hash_md5, hash_sha256;
			{
				hashwrapper* myWrapper = new md5wrapper();
				hash_md5 = myWrapper->getHashFromFile(global_path + wstring_to_string(path));
				delete myWrapper;
			}
			{
				hashwrapper* myWrapper = new sha256wrapper();
				hash_sha256 = myWrapper->getHashFromFile(global_path + wstring_to_string(path));
				delete myWrapper;
			}

			if (tedition > string_to_wstring(edition_date) && _waccess((string_to_wstring(global_path) + path).c_str(), 0) == 0 && hash_md5 == thash_md5 && hash_sha256 == thash_sha256)
			{
				//符合条件，开始替换版本

				STARTUPINFOA si = { 0 };
				si.cb = sizeof(si);
				PROCESS_INFORMATION pi = { 0 };
				CreateProcessA(NULL, (global_path + wstring_to_string(path)).data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);

				return 0;
			}
			else if (tedition == string_to_wstring(edition_date))
			{
				std::error_code ec;
				filesystem::remove_all(string_to_wstring(global_path) + L"installer", ec);
				filesystem::remove_all(string_to_wstring(global_path) + L"api", ec);

				filesystem::remove(string_to_wstring(global_path) + L"PptInterface.dll", ec);
				filesystem::remove(string_to_wstring(global_path) + L"PptInterface.tlb", ec);

				MessageBox(NULL, L"智绘教(屏幕画板程序) 已更新至带有实验功能的测试版本。\n\n测试功能默认开启，可能会导致程序不稳定或出错。\n如需关闭请至选项界面关闭。", (L"智绘教 已更新到" + string_to_wstring(edition_date)).c_str(), MB_OK | MB_ICONINFORMATION | MB_SYSTEMMODAL);
			}
		}
	}

	CoInitialize(NULL);

	userid = GetMainHardDiskSerialNumber();
	if (!isValidString(userid) || (userid.find(L"[") != userid.npos || userid.find(L"]") != userid.npos || userid.find(L";") != userid.npos || userid.find(L",") != userid.npos)) userid = L"无法正确识别";

	hiex::PreSetWindowShowState(SW_HIDE);
	floating_window = initgraph(background.getwidth(), background.getheight());
	hiex::PreSetWindowShowState(SW_HIDE);
	drawpad_window = initgraph(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	hiex::PreSetWindowShowState(SW_HIDE);
	ppt_window = initgraph(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	hiex::PreSetWindowShowState(SW_HIDE);
	freeze_window = initgraph(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	hiex::PreSetWindowShowState(SW_HIDE);
	test_window = initgraph(1010, 750);

	//初始化信息获取
	{
		if (_waccess((string_to_wstring(global_path) + L"opt\\deploy.json").c_str(), 4) == -1)
		{
			CreateDirectory((string_to_wstring(global_path) + L"opt").c_str(), NULL);

			Json::StyledWriter outjson;
			Json::Value root;

			root["edition"] = Json::Value(edition_date);
			root["startup"] = Json::Value(true);
			root["experimental_functions"] = Json::Value(true);

			ofstream writejson;
			writejson.imbue(locale("zh_CN.UTF8"));
			writejson.open(wstring_to_string(string_to_wstring(global_path) + L"opt\\deploy.json").c_str());
			writejson << outjson.write(root);
			writejson.close();
		}

		{
			Json::Reader reader;
			Json::Value root;

			ifstream readjson;
			readjson.imbue(locale("zh_CN.UTF8"));
			readjson.open(wstring_to_string(string_to_wstring(global_path) + L"opt\\deploy.json").c_str());

			if (reader.parse(readjson, root))
			{
				if (root.isMember("startup")) setlist.startup = root["startup"].asBool();
				if (root.isMember("experimental_functions")) setlist.experimental_functions = root["experimental_functions"].asBool();
			}

			readjson.close();
		}
		{
			Json::StyledWriter outjson;
			Json::Value root;

			root["edition"] = Json::Value(edition_date);
			root["startup"] = Json::Value(setlist.startup);
			root["experimental_functions"] = Json::Value(setlist.experimental_functions);

			ofstream writejson;
			writejson.imbue(locale("zh_CN.UTF8"));
			writejson.open(wstring_to_string(string_to_wstring(global_path) + L"opt\\deploy.json").c_str());
			writejson << outjson.write(root);
			writejson.close();
		}

		if (setlist.startup == true) ModifyRegedit(true);
		else ModifyRegedit(false);

		if (setlist.experimental_functions)
		{
			thread MagnifierThread_thread(MagnifierThread);
			MagnifierThread_thread.detach();
		}
	}
	// 初始化 RealTimeStylus 触控库
	{
		// Create RTS object
		g_pRealTimeStylus = CreateRealTimeStylus(drawpad_window);
		if (g_pRealTimeStylus == NULL)
		{
			uRealTimeStylus = -1;
			goto RealTimeStylusEnd;
		}

		// Create EventHandler object
		g_pSyncEventHandlerRTS = CSyncEventHandlerRTS::Create(g_pRealTimeStylus);
		if (g_pSyncEventHandlerRTS == NULL)
		{
			g_pRealTimeStylus->Release();
			g_pRealTimeStylus = NULL;

			uRealTimeStylus = -2;
			goto RealTimeStylusEnd;
		}

		// Enable RTS
		if (!EnableRealTimeStylus(g_pRealTimeStylus))
		{
			g_pSyncEventHandlerRTS->Release();
			g_pSyncEventHandlerRTS = NULL;

			g_pRealTimeStylus->Release();
			g_pRealTimeStylus = NULL;

			uRealTimeStylus = -3;
			goto RealTimeStylusEnd;
		}

		if (uRealTimeStylus == 0) uRealTimeStylus = 1;

	RealTimeStylusEnd:

		if (uRealTimeStylus <= 0)
		{
			MessageBox(NULL, (L"触控库 RTS 初始化失败，程序停止运行！\nRTS_Err" + to_wstring(-uRealTimeStylus)).c_str(), L"错误", MB_OK | MB_SYSTEMMODAL);

			off_signal = true;

			// 反初始化 COM 环境
			CoUninitialize();

			return 0;
		}

		thread RTSSpeed_thread(RTSSpeed);
		RTSSpeed_thread.detach();
	}

	thread floating_main_thread(floating_main);
	floating_main_thread.detach();
	thread drawpad_main_thread(drawpad_main);
	drawpad_main_thread.detach();
	thread test_main_thread(test_main);
	test_main_thread.detach();
	thread FreezeFrameWindow_thread(FreezeFrameWindow);
	FreezeFrameWindow_thread.detach();

	thread NetUpdate_thread(NetUpdate);
	NetUpdate_thread.detach();

	while (!off_signal) Sleep(500);
	while (1) if (!thread_status[L"floating_main"] && !thread_status[L"drawpad_main"] && !thread_status[L"test_main"] && !thread_status[L"NetUpdate"]) break;

	// 反初始化 COM 环境
	CoUninitialize();

	return 0;
}

// 悬浮窗部分 ==========

//窗口控制集
struct
{
	int x, y;
	int height, width;
	int translucent;
} floating_windows;

IMAGE floating_icon[20], sign;

double state;
double target_status;

bool RestoreSketchpad, empty_drawpad = true;
bool smallcard_refresh = true;

//置顶程序窗口
void TopWindow()
{
	while (1)
	{
		//if (xmg_api_pipe.enable_windows == false) SetWindowPos(floating_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		//else SetWindowPos(floating_window, xmg_api_pipe.windows, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		SetWindowPos(floating_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

		Sleep(100);
	}
}

//UI 控件

int BackgroundColorMode;
struct UIControlStruct
{
	float v, s, e;
};
map<wstring, UIControlStruct> UIControl, UIControlTarget;
map<wstring, UIControlStruct>& map<wstring, UIControlStruct>::operator=(const map<wstring, UIControlStruct>& m)
{
	//判断自赋值
	if (this == &m) return *this;
	//清空当前map
	this->clear();
	//遍历参数map，把每个键值对赋值给当前map
	for (auto it = m.begin(); it != m.end(); it++) this->insert(*it);
	//返回当前map的引用
	return *this;
}

struct UIControlColorStruct
{
	COLORREF v;
	float s, e;
};
map<wstring, UIControlColorStruct> UIControlColor, UIControlColorTarget;
map<wstring, UIControlColorStruct>& map<wstring, UIControlColorStruct>::operator=(const map<wstring, UIControlColorStruct>& m)
{
	//判断自赋值
	if (this == &m) return *this;
	//清空当前map
	this->clear();
	//遍历参数map，把每个键值对赋值给当前map
	for (auto it = m.begin(); it != m.end(); it++) this->insert(*it);
	//返回当前map的引用
	return *this;
}

//选色盘
Color ColorFromHSV(float hue, float saturation, float value) {
	float c = value * saturation;
	float x = c * (1 - abs(fmod(hue / 60.0f, 2) - 1));
	float m = value - c;

	float r, g, b;
	if (hue < 60) {
		r = c; g = x; b = 0;
	}
	else if (hue < 120) {
		r = x; g = c; b = 0;
	}
	else if (hue < 180) {
		r = 0; g = c; b = x;
	}
	else if (hue < 240) {
		r = 0; g = x; b = c;
	}
	else if (hue < 300) {
		r = x; g = 0; b = c;
	}
	else {
		r = c; g = 0; b = x;
	}

	return Color(255, (BYTE)((r + m) * 255), (BYTE)((g + m) * 255), (BYTE)((b + m) * 255));
}
IMAGE DrawHSVWheel(int r, int z = 0, int angle = 0)
{
	IMAGE ret = CreateImageColor(r, r, RGB(0, 0, 0, 0), true);
	r--;

	Graphics g(GetImageHDC(&ret));
	g.SetSmoothingMode(SmoothingModeHighQuality);

	for (int i = 0; i < 360; i++)
	{
		float currentAngle = angle + i;  // 当前段的起始角度
		if (currentAngle >= 360) currentAngle -= 360;  // 保持currentAngle在0到360之间

		Color color = ColorFromHSV(i, 1.0f, 1.0f);
		SolidBrush brush(color);
		g.FillPie(&brush, 0, 0, r, r, currentAngle, 4);
	}

	GraphicsPath path;
	path.AddEllipse(0 + z, 0 + z, r - z * 2, r - z * 2);
	PathGradientBrush pgb(&path);
	Color colors[] = { Color(0, 255, 255, 255) };  // 圆边缘
	INT count = 1;
	pgb.SetSurroundColors(colors, &count);
	pgb.SetCenterColor(Color(255, 255, 255, 255));  // 圆心
	pgb.SetCenterPoint(PointF(r / 2.0f, r / 2.0f));

	// 使用径向渐变刷绘制渐变圆形
	g.FillEllipse(&pgb, 0, 0, r, r);

	return ret;
}

//绘制屏幕
void DrawScreen()
{
	thread_status[L"DrawScreen"] = true;
	//初始化
	{
		//画笔配置初始化
		{
			choose.select = true;
			brush.color = brush.primary_colour = RGBA(50, 110, 217, 255);
		}
		//媒体资源读取
		{
			loadimage(&floating_icon[0], L"PNG", L"icon0");

			loadimage(&floating_icon[1], L"PNG", L"icon1");
			loadimage(&floating_icon[2], L"PNG", L"icon2");
			loadimage(&floating_icon[3], L"PNG", L"icon3", 30, 30, true);
			loadimage(&floating_icon[4], L"PNG", L"icon4");
			loadimage(&floating_icon[5], L"PNG", L"icon5");

			loadimage(&floating_icon[7], L"PNG", L"icon7");
			loadimage(&floating_icon[10], L"PNG", L"icon10");

			loadimage(&floating_icon[6], L"PNG", L"icon6", 30, 30, true);
			loadimage(&floating_icon[8], L"PNG", L"icon8", 30, 30, true);

			loadimage(&floating_icon[11], L"PNG", L"icon11", 20, 20, true);
			loadimage(&floating_icon[12], L"PNG", L"icon12", 20, 20, true);
			loadimage(&floating_icon[13], L"PNG", L"icon13", 20, 20, true);
			loadimage(&floating_icon[14], L"PNG", L"icon14", 20, 20, true);
			loadimage(&floating_icon[15], L"PNG", L"icon15", 20, 20, true);

			loadimage(&floating_icon[16], L"PNG", L"icon16");
			loadimage(&floating_icon[18], L"PNG", L"icon18");
			loadimage(&floating_icon[17], L"PNG", L"icon17", 20, 20, true);

			loadimage(&sign, L"PNG", L"sign1", 30, 30, true);
		}

		//窗口初始化
		{
			//SetWindowTransparent(true, 0);

			setbkmode(TRANSPARENT);
			setbkcolor(RGB(255, 255, 255));

			HiBeginDraw();
			cleardevice();
			HiEndDraw();

			DisableResizing(floating_window, true);//禁止窗口拉伸
			SetWindowLong(floating_window, GWL_STYLE, GetWindowLong(floating_window, GWL_STYLE) & ~WS_CAPTION);//隐藏标题栏
			SetWindowPos(floating_window, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_DRAWFRAME);
			MoveWindow(floating_window, floating_windows.x = GetSystemMetrics(SM_CXSCREEN) - 594, floating_windows.y = GetSystemMetrics(SM_CYSCREEN) - 557, floating_windows.width = background.getwidth(), floating_windows.height = background.getheight(), SWP_NOSIZE);
			SetWindowLong(floating_window, GWL_EXSTYLE, WS_EX_TOOLWINDOW);//隐藏任务栏

			//SetWindowTransparent(true, floating_windows.translucent = 200);
		}
		thread TopWindow_thread(TopWindow);
		TopWindow_thread.detach();
		SetImageColor(background, RGBA(0, 0, 0, 0), true);

		//UI 初始化
		{
			//主栏
			{
				//圆形
				{
					UIControl[L"Ellipse/Ellipse1/x"] = { (float)floating_windows.width - 96, 5, 1 };
					UIControl[L"Ellipse/Ellipse1/y"] = { (float)floating_windows.height - 155, 5, 1 };
					UIControl[L"Ellipse/Ellipse1/width"] = { 94, 5, 1 };
					UIControl[L"Ellipse/Ellipse1/height"] = { 94, 5, 1 };
					UIControl[L"Image/Sign1/frame_transparency"] = { 255, 300, 1 };

					UIControlColor[L"Ellipse/Ellipse1/fill"] = { RGBA(0, 0, 0, 150), 10, 1 };
					UIControlColor[L"Ellipse/Ellipse1/frame"] = { RGB(255, 255, 255), 10, 1 };
				}
				//圆角矩形
				{
					{
						UIControl[L"RoundRect/RoundRect1/x"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 26, 5, 1 };
						UIControl[L"RoundRect/RoundRect1/y"] = { (float)floating_windows.height - 155, 5, 1 };
						UIControl[L"RoundRect/RoundRect1/width"] = { 30, 5, 1 };
						UIControl[L"RoundRect/RoundRect1/height"] = { 94, 5, 1 };
						UIControl[L"RoundRect/RoundRect1/ellipseheight"] = { 25, 5, 1 };
						UIControl[L"RoundRect/RoundRect1/ellipsewidth"] = { 25, 5, 1 };

						UIControlColor[L"RoundRect/RoundRect1/fill"] = { RGBA(255, 255, 255, 0), 10, 1 };
						UIControlColor[L"RoundRect/RoundRect1/frame"] = { RGBA(150, 150, 150, 0), 10, 1 };
					}
					{
						UIControl[L"RoundRect/RoundRect2/x"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 33, 5, 1 };
						UIControl[L"RoundRect/RoundRect2/y"] = { (float)floating_windows.height - 156 + 8, 5, 1 };
						UIControl[L"RoundRect/RoundRect2/width"] = { 80, 5, 1 };
						UIControl[L"RoundRect/RoundRect2/height"] = { 80, 5, 1 };
						UIControl[L"RoundRect/RoundRect2/ellipseheight"] = { 25, 5, 1 };
						UIControl[L"RoundRect/RoundRect2/ellipsewidth"] = { 25, 5, 1 };

						UIControlColor[L"RoundRect/RoundRect2/frame"] = { RGBA(98, 175, 82, 0), 10, 1 };
					}

					{
						UIControl[L"RoundRect/BrushTop/x"] = { (float)floating_windows.width - 48, 3, 1 };
						UIControl[L"RoundRect/BrushTop/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v, 3, 1 };
						UIControl[L"RoundRect/BrushTop/width"] = { 80, 3, 1 };
						UIControl[L"RoundRect/BrushTop/height"] = { 90, 3, 1 };
						UIControl[L"RoundRect/BrushTop/ellipseheight"] = { 25, 3, 1 };
						UIControl[L"RoundRect/BrushTop/ellipsewidth"] = { 25, 3, 1 };

						UIControlColor[L"RoundRect/BrushTop/fill"] = { RGBA(255, 255, 255, 0), 10, 1 };
						UIControlColor[L"RoundRect/BrushTop/frame"] = { RGBA(150, 150, 150, 0), 10, 1 };
					}
					{
						{
							UIControl[L"RoundRect/BrushColor1/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColor1/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColor1/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor1/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor1/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor1/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor1/transparency"] = { 0, 10, 1 };
							UIControlColor[L"RoundRect/BrushColor1/fill"] = { RGB(255, 255, 255), 5, 1 };

							UIControl[L"RoundRect/BrushColorFrame1/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame1/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame1/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame1/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame1/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame1/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame1/thickness"] = { 1, 20, 0.1f };
							UIControlColor[L"RoundRect/BrushColorFrame1/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };

							UIControl[L"Image/BrushColor1/x"] = { (float)floating_windows.width - 48 + 10, 3, 1 };
							UIControl[L"Image/BrushColor1/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10, 3, 1 };
							UIControl[L"Image/BrushColor1/transparency"] = { 0, 10, 1 };
						}
						{
							UIControl[L"RoundRect/BrushColor2/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColor2/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColor2/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor2/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor2/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor2/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor2/transparency"] = { 0, 10, 1 };
							UIControlColor[L"RoundRect/BrushColor2/fill"] = { RGB(0, 0, 0), 5, 1 };

							UIControl[L"RoundRect/BrushColorFrame2/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame2/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame2/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame2/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame2/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame2/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame2/thickness"] = { 1, 20, 0.1f };
							UIControlColor[L"RoundRect/BrushColorFrame2/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };

							UIControl[L"Image/BrushColor2/x"] = { (float)floating_windows.width - 48 + 10, 3, 1 };
							UIControl[L"Image/BrushColor2/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10, 3, 1 };
							UIControl[L"Image/BrushColor2/transparency"] = { 0, 10, 1 };
						}
						{
							UIControl[L"RoundRect/BrushColor3/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColor3/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColor3/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor3/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor3/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor3/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor3/transparency"] = { 0, 10, 1 };
							UIControlColor[L"RoundRect/BrushColor3/fill"] = { RGB(255, 139, 0), 5, 1 };

							UIControl[L"RoundRect/BrushColorFrame3/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame3/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame3/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame3/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame3/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame3/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame3/thickness"] = { 1, 20, 0.1f };
							UIControlColor[L"RoundRect/BrushColorFrame3/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };

							UIControl[L"Image/BrushColor3/x"] = { (float)floating_windows.width - 48 + 10, 3, 1 };
							UIControl[L"Image/BrushColor3/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10, 3, 1 };
							UIControl[L"Image/BrushColor3/transparency"] = { 0, 10, 1 };
						}
						{
							UIControl[L"RoundRect/BrushColor4/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColor4/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColor4/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor4/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor4/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor4/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor4/transparency"] = { 0, 10, 1 };
							UIControlColor[L"RoundRect/BrushColor4/fill"] = { RGB(50, 30, 181), 5, 1 };

							UIControl[L"RoundRect/BrushColorFrame4/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame4/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame4/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame4/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame4/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame4/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame4/thickness"] = { 1, 20, 0.1f };
							UIControlColor[L"RoundRect/BrushColorFrame4/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };

							UIControl[L"Image/BrushColor4/x"] = { (float)floating_windows.width - 48 + 10, 3, 1 };
							UIControl[L"Image/BrushColor4/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10, 3, 1 };
							UIControl[L"Image/BrushColor4/transparency"] = { 0, 10, 1 };
						}
						{
							UIControl[L"RoundRect/BrushColor5/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColor5/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColor5/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor5/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor5/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor5/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor5/transparency"] = { 0, 10, 1 };
							UIControlColor[L"RoundRect/BrushColor5/fill"] = { RGB(255, 197, 16), 5, 1 };

							UIControl[L"RoundRect/BrushColorFrame5/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame5/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame5/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame5/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame5/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame5/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame5/thickness"] = { 1, 20, 0.1f };
							UIControlColor[L"RoundRect/BrushColorFrame5/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };

							UIControl[L"Image/BrushColor5/x"] = { (float)floating_windows.width - 48 + 10, 3, 1 };
							UIControl[L"Image/BrushColor5/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10, 3, 1 };
							UIControl[L"Image/BrushColor5/transparency"] = { 0, 10, 1 };
						}
						{
							UIControl[L"RoundRect/BrushColor6/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColor6/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColor6/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor6/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor6/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor6/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor6/transparency"] = { 0, 10, 1 };
							UIControlColor[L"RoundRect/BrushColor6/fill"] = { RGB(255, 16, 0), 5, 1 };

							UIControl[L"RoundRect/BrushColorFrame6/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame6/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame6/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame6/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame6/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame6/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame6/thickness"] = { 1, 20, 0.1f };
							UIControlColor[L"RoundRect/BrushColorFrame6/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };

							UIControl[L"Image/BrushColor6/x"] = { (float)floating_windows.width - 48 + 10, 3, 1 };
							UIControl[L"Image/BrushColor6/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10, 3, 1 };
							UIControl[L"Image/BrushColor6/transparency"] = { 0, 10, 1 };
						}
						{
							UIControl[L"RoundRect/BrushColor7/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColor7/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColor7/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor7/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor7/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor7/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor7/transparency"] = { 0, 10, 1 };
							UIControlColor[L"RoundRect/BrushColor7/fill"] = { RGB(78,161,183), 5, 1 };

							UIControl[L"RoundRect/BrushColorFrame7/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame7/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame7/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame7/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame7/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame7/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame7/thickness"] = { 1, 20, 0.1f };
							UIControlColor[L"RoundRect/BrushColorFrame7/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };

							UIControl[L"Image/BrushColor7/x"] = { (float)floating_windows.width - 48 + 10, 3, 1 };
							UIControl[L"Image/BrushColor7/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10, 3, 1 };
							UIControl[L"Image/BrushColor7/transparency"] = { 0, 10, 1 };
						}
						{
							UIControl[L"RoundRect/BrushColor8/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColor8/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColor8/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor8/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor8/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor8/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor8/transparency"] = { 0, 10, 1 };
							UIControlColor[L"RoundRect/BrushColor8/fill"] = { RGB(50, 110, 217), 5, 1 };

							UIControl[L"RoundRect/BrushColorFrame8/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame8/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame8/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame8/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame8/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame8/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame8/thickness"] = { 1, 20, 0.1f };
							UIControlColor[L"RoundRect/BrushColorFrame8/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };

							UIControl[L"Image/BrushColor8/x"] = { (float)floating_windows.width - 48 + 10, 3, 1 };
							UIControl[L"Image/BrushColor8/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10, 3, 1 };
							UIControl[L"Image/BrushColor8/transparency"] = { 0, 10, 1 };
						}
						{
							UIControl[L"RoundRect/BrushColor9/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColor9/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColor9/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor9/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor9/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor9/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor9/transparency"] = { 0, 10, 1 };
							UIControlColor[L"RoundRect/BrushColor9/fill"] = { RGB(102, 213, 82), 5, 1 };

							UIControl[L"RoundRect/BrushColorFrame9/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame9/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame9/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame9/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame9/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame9/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame9/thickness"] = { 1, 20, 0.1f };
							UIControlColor[L"RoundRect/BrushColorFrame9/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };

							UIControl[L"Image/BrushColor9/x"] = { (float)floating_windows.width - 48 + 10, 3, 1 };
							UIControl[L"Image/BrushColor9/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10, 3, 1 };
							UIControl[L"Image/BrushColor9/transparency"] = { 0, 10, 1 };
						}
						{
							UIControl[L"RoundRect/BrushColor10/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColor10/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColor10/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor10/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor10/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor10/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor10/transparency"] = { 0, 10, 1 };
							UIControlColor[L"RoundRect/BrushColor10/fill"] = { RGB(48, 108, 0), 5, 1 };

							UIControl[L"RoundRect/BrushColorFrame10/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame10/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame10/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame10/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame10/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame10/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame10/thickness"] = { 1, 20, 0.1f };
							UIControlColor[L"RoundRect/BrushColorFrame10/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };

							UIControl[L"Image/BrushColor10/x"] = { (float)floating_windows.width - 48 + 10, 3, 1 };
							UIControl[L"Image/BrushColor10/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10, 3, 1 };
							UIControl[L"Image/BrushColor10/transparency"] = { 0, 10, 1 };
						}
						{
							UIControl[L"RoundRect/BrushColor11/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColor11/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColor11/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor11/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor11/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor11/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColor11/transparency"] = { 0, 10, 1 };
							UIControlColor[L"RoundRect/BrushColor11/fill"] = { RGB(255, 30, 207), 5, 1 };

							UIControl[L"RoundRect/BrushColorFrame11/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame11/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame11/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame11/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame11/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame11/ellipsewidth"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame11/thickness"] = { 1, 20, 0.1f };
							UIControlColor[L"RoundRect/BrushColorFrame11/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };

							UIControl[L"Image/BrushColor11/x"] = { (float)floating_windows.width - 48 + 10, 3, 1 };
							UIControl[L"Image/BrushColor11/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10, 3, 1 };
							UIControl[L"Image/BrushColor11/transparency"] = { 0, 10, 1 };
						}
						{
							UIControl[L"RoundRect/BrushColor12/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColor12/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColor12/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor12/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColor12/transparency"] = { 0, 10, 1 };

							UIControl[L"RoundRect/BrushColorFrame12/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame12/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame12/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame12/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame12/ellipseheight"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame12/ellipsewidth"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorFrame12/angle"] = { 0, 1, 0.1f };
							UIControl[L"RoundRect/BrushColorFrame12/thickness"] = { 1, 20, 0.1f };
							UIControlColor[L"RoundRect/BrushColorFrame12/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };
						}
					}
					{
						UIControl[L"RoundRect/PaintThickness/x"] = { (float)floating_windows.width - 38, 3, 1 };
						UIControl[L"RoundRect/PaintThickness/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 10, 3, 1 };
						UIControl[L"RoundRect/PaintThickness/width"] = { 28, 3, 1 };
						UIControl[L"RoundRect/PaintThickness/height"] = { 70, 3, 1 };
						UIControl[L"RoundRect/PaintThickness/ellipseheight"] = { 25, 3, 1 };
						UIControl[L"RoundRect/PaintThickness/ellipsewidth"] = { 25, 3, 1 };

						UIControlColor[L"RoundRect/PaintThickness/fill"] = { RGBA(230, 230, 230, 0), 5, 1 };

						UIControl[L"RoundRect/PaintThicknessPrompt/x"] = { (float)floating_windows.width - 10, 3, 1 };
						UIControl[L"RoundRect/PaintThicknessPrompt/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 10, 3, 1 };
						UIControl[L"RoundRect/PaintThicknessPrompt/width"] = { 5, 3, 1 };
						UIControl[L"RoundRect/PaintThicknessPrompt/height"] = { 5, 3, 1 };
						UIControl[L"RoundRect/PaintThicknessPrompt/ellipseheight"] = { 5, 3, 1 };
						UIControl[L"RoundRect/PaintThicknessPrompt/ellipsewidth"] = { 5, 3, 1 };

						UIControlColor[L"RoundRect/PaintThicknessPrompt/fill"] = { RGBA(255, 255, 255, 0), 5, 1 };

						{
							UIControl[L"RoundRect/PaintThicknessAdjust/x"] = { (float)floating_windows.width - 38, 3, 1 };
							UIControl[L"RoundRect/PaintThicknessAdjust/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 10, 3, 1 };
							UIControl[L"RoundRect/PaintThicknessAdjust/width"] = { 28, 3, 1 };
							UIControl[L"RoundRect/PaintThicknessAdjust/height"] = { 70, 3, 1 };
							UIControl[L"RoundRect/PaintThicknessAdjust/ellipseheight"] = { 25, 3, 1 };
							UIControl[L"RoundRect/PaintThicknessAdjust/ellipsewidth"] = { 25, 3, 1 };

							UIControlColor[L"RoundRect/PaintThicknessAdjust/fill"] = { RGBA(255, 255, 255, 0), 10, 1 };
							UIControlColor[L"RoundRect/PaintThicknessAdjust/frame"] = { RGBA(150, 150, 150, 0), 10, 1 };

							//调节底
							{
								UIControl[L"RoundRect/PaintThicknessSchedule1/x"] = { (float)floating_windows.width - 38, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule1/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 42, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule1/width"] = { 28, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule1/height"] = { 6, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule1/ellipseheight"] = { 6, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule1/ellipsewidth"] = { 6, 3, 1 };
								UIControlColor[L"RoundRect/PaintThicknessSchedule1/fill"] = { RGBA(230, 230, 230, 0), 10, 1 };

								UIControl[L"RoundRect/PaintThicknessSchedule2/x"] = { (float)floating_windows.width - 38, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule2/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 42, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule2/width"] = { 28, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule2/height"] = { 6, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule2/ellipseheight"] = { 6, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule2/ellipsewidth"] = { 6, 3, 1 };
								UIControlColor[L"RoundRect/PaintThicknessSchedule2/fill"] = { SET_ALPHA(brush.color, 0), 10, 1 };

								UIControl[L"RoundRect/PaintThicknessSchedule3/x"] = { (float)floating_windows.width - 32, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule3/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 25, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule3/width"] = { 20, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule3/height"] = { 20, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule3/ellipseheight"] = { 20, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule3/ellipsewidth"] = { 20, 3, 1 };
								UIControlColor[L"RoundRect/PaintThicknessSchedule3/fill"] = { SET_ALPHA(brush.color, 0), 10, 1 };

								UIControl[L"RoundRect/PaintThicknessSchedule4a/x"] = { (float)floating_windows.width - 32, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule4a/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 25, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule4a/width"] = { 20, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule4a/height"] = { 20, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule4a/ellipse"] = { 20, 3, 1 };
								UIControlColor[L"RoundRect/PaintThicknessSchedule4a/fill"] = { SET_ALPHA(brush.color, 0), 3, 1 };

								UIControl[L"RoundRect/PaintThicknessSchedule5a/x"] = { (float)floating_windows.width - 32, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule5a/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 25, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule5a/width"] = { 20, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule5a/height"] = { 20, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule5a/ellipse"] = { 20, 3, 1 };
								UIControlColor[L"RoundRect/PaintThicknessSchedule5a/fill"] = { SET_ALPHA(brush.color, 0), 3, 1 };

								UIControl[L"RoundRect/PaintThicknessSchedule6a/x"] = { (float)floating_windows.width - 32, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule6a/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 25, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule6a/width"] = { 20, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule6a/height"] = { 20, 3, 1 };
								UIControl[L"RoundRect/PaintThicknessSchedule6a/ellipse"] = { 20, 3, 1 };
								UIControlColor[L"RoundRect/PaintThicknessSchedule6a/fill"] = { SET_ALPHA(brush.color, 0), 3, 1 };
							}
						}
					}
					{
						{
							UIControl[L"RoundRect/BrushColorChoose/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorChoose/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorChoose/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChoose/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChoose/ellipseheight"] = { 5, 3, 1 };
							UIControl[L"RoundRect/BrushColorChoose/ellipsewidth"] = { 5, 3, 1 };

							UIControlColor[L"RoundRect/BrushColorChoose/fill"] = { RGBA(255, 255, 255, 0), 10, 1 };
							UIControlColor[L"RoundRect/BrushColorChoose/frame"] = { RGBA(130, 130, 130, 0), 10, 1 };
						}
						{
							UIControl[L"RoundRect/BrushColorChooseWheel/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseWheel/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseWheel/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseWheel/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseWheel/transparency"] = { 0, 10, 1 };

							UIControl[L"RoundRect/BrushColorChooseMark/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMark/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMark/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMark/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMark/ellipseheight"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMark/ellipsewidth"] = { 40, 3, 1 };
							UIControlColor[L"RoundRect/BrushColorChooseMark/fill"] = { SET_ALPHA(brush.color,0), 5, 1 };
							UIControlColor[L"RoundRect/BrushColorChooseMark/frame"] = { RGBA(130, 130, 130, 0), 5, 1 };

							UIControl[L"RoundRect/BrushColorChooseMarkR/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkR/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkR/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkR/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkR/ellipseheight"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkR/ellipsewidth"] = { 40, 3, 1 };
							UIControlColor[L"RoundRect/BrushColorChooseMarkR/fill"] = { RGBA(255, 0, 0, 0) , 5, 1 };
							UIControlColor[L"RoundRect/BrushColorChooseMarkR/frame"] = { RGBA(255, 0, 0, 0) , 5, 1 };
							UIControlColor[L"RoundRect/BrushColorChooseMarkR/text"] = { RGBA(255, 0, 0, 0) , 5, 1 };

							UIControl[L"RoundRect/BrushColorChooseMarkG/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkG/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkG/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkG/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkG/ellipseheight"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkG/ellipsewidth"] = { 40, 3, 1 };
							UIControlColor[L"RoundRect/BrushColorChooseMarkG/fill"] = { RGBA(0, 255, 0, 0) , 5, 1 };
							UIControlColor[L"RoundRect/BrushColorChooseMarkG/frame"] = { RGBA(0, 255, 0, 0) , 5, 1 };
							UIControlColor[L"RoundRect/BrushColorChooseMarkG/text"] = { RGBA(0, 255, 0, 0) , 5, 1 };

							UIControl[L"RoundRect/BrushColorChooseMarkB/x"] = { (float)floating_windows.width - 48, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkB/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + 30, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkB/width"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkB/height"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkB/ellipseheight"] = { 40, 3, 1 };
							UIControl[L"RoundRect/BrushColorChooseMarkB/ellipsewidth"] = { 40, 3, 1 };
							UIControlColor[L"RoundRect/BrushColorChooseMarkB/fill"] = { RGBA(0, 0, 255, 0) , 5, 1 };
							UIControlColor[L"RoundRect/BrushColorChooseMarkB/frame"] = { RGBA(0, 0, 255, 0) , 5, 1 };
							UIControlColor[L"RoundRect/BrushColorChooseMarkB/text"] = { RGBA(0, 0,255, 0) , 5, 1 };
						}
					}

					{
						UIControl[L"RoundRect/BrushBottom/x"] = { (float)floating_windows.width - 48, 3, 1 };
						UIControl[L"RoundRect/BrushBottom/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + UIControl[L"RoundRect/RoundRect1/height"].v - 40, 3, 1 };
						UIControl[L"RoundRect/BrushBottom/width"] = { 48, 3, 1 };
						UIControl[L"RoundRect/BrushBottom/height"] = { 40, 3, 1 };
						UIControl[L"RoundRect/BrushBottom/ellipseheight"] = { 25, 3, 1 };
						UIControl[L"RoundRect/BrushBottom/ellipsewidth"] = { 25, 3, 1 };

						UIControlColor[L"RoundRect/BrushBottom/fill"] = { RGBA(255, 255, 255, 0), 10, 1 };
						UIControlColor[L"RoundRect/BrushBottom/frame"] = { RGBA(150, 150, 150, 0), 10, 1 };
					}
					{
						UIControl[L"RoundRect/BrushChoose/x"] = { (float)floating_windows.width - 48, 3, 1 };
						UIControl[L"RoundRect/BrushChoose/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + UIControl[L"RoundRect/RoundRect1/height"].v - 35, 3, 1 };
						UIControl[L"RoundRect/BrushChoose/width"] = { 48, 3, 1 };
						UIControl[L"RoundRect/BrushChoose/height"] = { 30, 3, 1 };
						UIControl[L"RoundRect/BrushChoose/ellipseheight"] = { 15, 3, 1 };
						UIControl[L"RoundRect/BrushChoose/ellipsewidth"] = { 15, 3, 1 };

						UIControlColor[L"RoundRect/BrushChoose/frame"] = { SET_ALPHA(brush.color,0), 5, 1 };
					}
					{
						UIControl[L"RoundRect/BrushMode/x"] = { (float)floating_windows.width - 48, 3, 1 };
						UIControl[L"RoundRect/BrushMode/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + UIControl[L"RoundRect/RoundRect1/height"].v - 35, 3, 1 };
						UIControl[L"RoundRect/BrushMode/width"] = { 48, 3, 1 };
						UIControl[L"RoundRect/BrushMode/height"] = { 30, 3, 1 };
						UIControl[L"RoundRect/BrushMode/ellipseheight"] = { 15, 3, 1 };
						UIControl[L"RoundRect/BrushMode/ellipsewidth"] = { 15, 3, 1 };

						UIControlColor[L"RoundRect/BrushMode/frame"] = { SET_ALPHA(brush.color,0), 5, 1 };
					}
					{
						UIControl[L"RoundRect/BrushInterval/x"] = { (float)floating_windows.width - 48 + 35, 3, 1 };
						UIControl[L"RoundRect/BrushInterval/y"] = { UIControl[L"RoundRect/RoundRect1/y"].v + UIControl[L"RoundRect/RoundRect1/height"].v - 35 + 10, 3, 1 };
						UIControl[L"RoundRect/BrushInterval/width"] = { 3, 3, 1 };
						UIControl[L"RoundRect/BrushInterval/height"] = { 20, 3, 1 };

						UIControlColor[L"RoundRect/BrushInterval/fill"] = { RGBA(150,150,150,0), 5, 1 };
					}
				}
				//图像
				{
					{
						UIControl[L"Image/Sign1/x"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 33, 5, 1 };
						UIControl[L"Image/Sign1/y"] = { UIControl[L"Ellipse/Ellipse1/y"].v + 33, 5, 1 };
						UIControl[L"Image/Sign1/transparency"] = { 255, 300, 1 };
					}
					//选择
					{
						UIControl[L"Image/choose/x"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 53, 5, 1 };
						UIControl[L"Image/choose/y"] = { (float)floating_windows.height - 140, 5, 1 };
						UIControlColor[L"Image/choose/fill"] = { RGBA(98, 175, 82, 0), 5, 1 };
					}
					//画笔
					{
						UIControl[L"Image/brush/x"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 53, 5, 1 };
						UIControl[L"Image/brush/y"] = { (float)floating_windows.height - 140, 5, 1 };
						UIControlColor[L"Image/brush/fill"] = { RGBA(130, 130, 130, 0), 5, 1 };

						//画笔底部栏
						{
							{
								UIControl[L"Image/PaintBrush/x"] = { UIControl[L"RoundRect/BrushChoose/x"].v + 10, 3, 1 };
								UIControl[L"Image/PaintBrush/y"] = { UIControl[L"RoundRect/BrushChoose/y"].v + 10, 3, 1 };
								UIControlColor[L"Image/PaintBrush/words_color"] = { SET_ALPHA(brush.color, 0), 5, 1 };
							}
							{
								UIControl[L"Image/FluorescentBrush/x"] = { UIControl[L"RoundRect/BrushChoose/x"].v + 10, 3, 1 };
								UIControl[L"Image/FluorescentBrush/y"] = { UIControl[L"RoundRect/BrushChoose/y"].v + 10, 3, 1 };
								UIControlColor[L"Image/FluorescentBrush/words_color"] = { SET_ALPHA(brush.color, 0), 5, 1 };
							}

							{
								UIControl[L"Image/WriteBrush/x"] = { UIControl[L"RoundRect/BrushChoose/x"].v + 10, 3, 1 };
								UIControl[L"Image/WriteBrush/y"] = { UIControl[L"RoundRect/BrushChoose/y"].v + 10, 3, 1 };
								UIControlColor[L"Image/WriteBrush/words_color"] = { SET_ALPHA(brush.color, 0), 5, 1 };
							}
							{
								UIControl[L"Image/LineBrush/x"] = { UIControl[L"RoundRect/BrushChoose/x"].v + 10, 3, 1 };
								UIControl[L"Image/LineBrush/y"] = { UIControl[L"RoundRect/BrushChoose/y"].v + 10, 3, 1 };
								UIControlColor[L"Image/LineBrush/words_color"] = { SET_ALPHA(brush.color, 0), 5, 1 };
							}
							{
								UIControl[L"Image/RectangleBrush/x"] = { UIControl[L"RoundRect/BrushChoose/x"].v + 10, 3, 1 };
								UIControl[L"Image/RectangleBrush/y"] = { UIControl[L"RoundRect/BrushChoose/y"].v + 10, 3, 1 };
								UIControlColor[L"Image/RectangleBrush/words_color"] = { SET_ALPHA(brush.color, 0), 5, 1 };
							}
						}
					}
					//橡皮
					{
						UIControl[L"Image/rubber/x"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 53, 5, 1 };
						UIControl[L"Image/rubber/y"] = { (float)floating_windows.height - 140, 5, 1 };
						UIControlColor[L"Image/rubber/fill"] = { RGBA(130, 130, 130, 0), 5, 1 };
					}
					//程序调测
					{
						UIControl[L"Image/test/x"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 53, 5, 1 };
						UIControl[L"Image/test/y"] = { (float)floating_windows.height - 140, 5, 1 };
						UIControlColor[L"Image/test/fill"] = { RGBA(130, 130, 130, 0), 5, 1 };
					}
				}
				//文字
				{
					//选择
					{
						UIControl[L"Words/choose/height"] = { 18, 5, 1 };
						UIControl[L"Words/choose/left"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 33, 5, 1 };
						UIControl[L"Words/choose/top"] = { (float)floating_windows.height - 155 + 48, 5, 1 };
						UIControl[L"Words/choose/right"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 33 + 83, 5, 1 };
						UIControl[L"Words/choose/bottom"] = { (float)floating_windows.height - 155 + 48 + 48, 5, 1 };
						UIControlColor[L"Words/choose/words_color"] = { RGBA(98, 175, 82, 0), 5, 1 };
					}
					//画笔
					{
						UIControl[L"Words/brush/height"] = { 18, 5, 1 };
						UIControl[L"Words/brush/left"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 33, 5, 1 };
						UIControl[L"Words/brush/top"] = { (float)floating_windows.height - 155 + 48, 5, 1 };
						UIControl[L"Words/brush/right"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 33 + 83, 5, 1 };
						UIControl[L"Words/brush/bottom"] = { (float)floating_windows.height - 155 + 48 + 48, 5, 1 };
						UIControlColor[L"Words/brush/words_color"] = { RGBA(130, 130, 130, 0), 5, 1 };

						{
							UIControl[L"Words/brushSize/left"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 33 + 45, 5, 1 };
							UIControl[L"Words/brushSize/top"] = { (float)floating_windows.height - 155 + 48 - 12, 5, 1 };

							UIControlColor[L"Words/brushSize/words_color"] = { SET_ALPHA(brush.color, 0), 5, 1 };
						}

						//画笔顶部栏
						{
							//画笔粗细
							{
								UIControl[L"Words/PaintThickness/size"] = { 20, 3, 1 };
								UIControl[L"Words/PaintThickness/left"] = { (float)floating_windows.width - 48, 3, 1 };
								UIControl[L"Words/PaintThickness/top"] = { UIControl[L"RoundRect/BrushTop/y"].v , 3, 1 };
								UIControl[L"Words/PaintThickness/width"] = { 50, 3, 1 };
								UIControl[L"Words/PaintThickness/height"] = { 96, 3, 1 };
								UIControlColor[L"Words/PaintThickness/words_color"] = { SET_ALPHA(brush.color, 255), 5, 1 };

								UIControl[L"Words/PaintThicknessValue/size"] = { 20, 3, 1 };
								UIControl[L"Words/PaintThicknessValue/left"] = { (float)floating_windows.width - 48, 3, 1 };
								UIControl[L"Words/PaintThicknessValue/top"] = { UIControl[L"RoundRect/BrushTop/y"].v , 3, 1 };
								UIControl[L"Words/PaintThicknessValue/width"] = { 50, 3, 1 };
								UIControl[L"Words/PaintThicknessValue/height"] = { 96, 3, 1 };
								UIControlColor[L"Words/PaintThicknessValue/words_color"] = { SET_ALPHA(brush.color, 255), 5, 1 };
							}
						}
						//画笔底部栏
						{
							{
								UIControl[L"Words/PaintBrush/size"] = { 18, 3, 1 };
								UIControl[L"Words/PaintBrush/left"] = { UIControl[L"RoundRect/BrushChoose/x"].v + 20, 3, 1 };
								UIControl[L"Words/PaintBrush/top"] = { UIControl[L"RoundRect/BrushChoose/y"].v + 5, 3, 1 };
								UIControl[L"Words/PaintBrush/width"] = { 65, 3, 1 };
								UIControl[L"Words/PaintBrush/height"] = { 33, 3, 1 };
								UIControlColor[L"Words/PaintBrush/words_color"] = { SET_ALPHA(brush.color,0), 5, 1 };
							}
							{
								UIControl[L"Words/FluorescentBrush/size"] = { 18, 3, 1 };
								UIControl[L"Words/FluorescentBrush/left"] = { UIControl[L"RoundRect/BrushChoose/x"].v + 20, 3, 1 };
								UIControl[L"Words/FluorescentBrush/top"] = { UIControl[L"RoundRect/BrushChoose/y"].v + 5, 3, 1 };
								UIControl[L"Words/FluorescentBrush/width"] = { 65, 3, 1 };
								UIControl[L"Words/FluorescentBrush/height"] = { 33, 3, 1 };
								UIControlColor[L"Words/FluorescentBrush/words_color"] = { SET_ALPHA(brush.color,0), 5, 1 };
							}

							{
								UIControl[L"Words/WriteBrush/size"] = { 18, 3, 1 };
								UIControl[L"Words/WriteBrush/left"] = { UIControl[L"RoundRect/BrushChoose/x"].v + 20, 3, 1 };
								UIControl[L"Words/WriteBrush/top"] = { UIControl[L"RoundRect/BrushChoose/y"].v + 5, 3, 1 };
								UIControl[L"Words/WriteBrush/width"] = { 65, 3, 1 };
								UIControl[L"Words/WriteBrush/height"] = { 33, 3, 1 };
								UIControlColor[L"Words/WriteBrush/words_color"] = { SET_ALPHA(brush.color,0), 5, 1 };
							}
							{
								UIControl[L"Words/LineBrush/size"] = { 18, 3, 1 };
								UIControl[L"Words/LineBrush/left"] = { UIControl[L"RoundRect/BrushChoose/x"].v + 20, 3, 1 };
								UIControl[L"Words/LineBrush/top"] = { UIControl[L"RoundRect/BrushChoose/y"].v + 5, 3, 1 };
								UIControl[L"Words/LineBrush/width"] = { 65, 3, 1 };
								UIControl[L"Words/LineBrush/height"] = { 33, 3, 1 };
								UIControlColor[L"Words/LineBrush/words_color"] = { SET_ALPHA(brush.color,0), 5, 1 };
							}
							{
								UIControl[L"Words/RectangleBrush/size"] = { 18, 3, 1 };
								UIControl[L"Words/RectangleBrush/left"] = { UIControl[L"RoundRect/BrushChoose/x"].v + 20, 3, 1 };
								UIControl[L"Words/RectangleBrush/top"] = { UIControl[L"RoundRect/BrushChoose/y"].v + 5, 3, 1 };
								UIControl[L"Words/RectangleBrush/width"] = { 65, 3, 1 };
								UIControl[L"Words/RectangleBrush/height"] = { 33, 3, 1 };
								UIControlColor[L"Words/RectangleBrush/words_color"] = { SET_ALPHA(brush.color,0), 5, 1 };
							}
						}
					}
					//橡皮
					{
						UIControl[L"Words/rubber/height"] = { 18, 5, 1 };
						UIControl[L"Words/rubber/left"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 33, 5, 1 };
						UIControl[L"Words/rubber/top"] = { (float)floating_windows.height - 155 + 48, 5, 1 };
						UIControl[L"Words/rubber/right"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 33 + 83, 5, 1 };
						UIControl[L"Words/rubber/bottom"] = { (float)floating_windows.height - 155 + 48 + 48, 5, 1 };
						UIControlColor[L"Words/rubber/words_color"] = { RGBA(98, 175, 82, 0), 5, 1 };
					}
					//程序调测
					{
						UIControl[L"Words/test/height"] = { 18, 5, 1 };
						UIControl[L"Words/test/left"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 33, 5, 1 };
						UIControl[L"Words/test/top"] = { (float)floating_windows.height - 155 + 48, 5, 1 };
						UIControl[L"Words/test/right"] = { UIControl[L"Ellipse/Ellipse1/x"].v + 33 + 83, 5, 1 };
						UIControl[L"Words/test/bottom"] = { (float)floating_windows.height - 155 + 48 + 48, 5, 1 };
						UIControlColor[L"Words/test/words_color"] = { RGBA(98, 175, 82, 0), 5, 1 };
					}
				}
			}

			UIControlTarget = UIControl;
			UIControlColorTarget = UIControlColor;
		}
		//插件加载
		{
			if (_waccess((string_to_wstring(global_path) + L"plug-in\\随机点名\\随机点名.exe").c_str(), 4) == 0) plug_in_RandomRollCall.select = 1;
		}
	}

	OnForceShow(floating_window);
	SetWindowPos(floating_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	{
		LONG style = GetWindowLong(floating_window, GWL_EXSTYLE);
		style |= WS_EX_NOACTIVATE;
		SetWindowLong(floating_window, GWL_EXSTYLE, style);
	}

	// 设置BLENDFUNCTION结构体
	BLENDFUNCTION blend;
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.SourceConstantAlpha = 255; // 设置透明度，0为全透明，255为不透明
	blend.AlphaFormat = AC_SRC_ALPHA; // 使用源图像的alpha通道
	HDC hdcScreen = GetDC(NULL);
	// 调用UpdateLayeredWindow函数更新窗口
	POINT ptSrc = { 0,0 };
	SIZE sizeWnd = { background.getwidth(),background.getheight() };
	POINT ptDst = { 0,0 }; // 设置窗口位置
	UPDATELAYEREDWINDOWINFO ulwi = { 0 };
	ulwi.cbSize = sizeof(ulwi);
	ulwi.hdcDst = hdcScreen;
	ulwi.pptDst = &ptDst;
	ulwi.psize = &sizeWnd;
	ulwi.pptSrc = &ptSrc;
	ulwi.crKey = RGB(255, 255, 255);
	ulwi.pblend = &blend;
	ulwi.dwFlags = ULW_ALPHA;
	LONG nRet = ::GetWindowLong(floating_window, GWL_EXSTYLE);
	nRet |= WS_EX_LAYERED;
	::SetWindowLong(floating_window, GWL_EXSTYLE, nRet);

	already = true;
	magnificationWindowReady++;

	for (int for_num = 1; !off_signal; for_num = 2)
	{
		SetImageColor(background, RGBA(0, 0, 0, 0), true);

		//UI绘制部分
		{
			if ((int)state == 0)
			{
				//圆形
				{
					UIControlColorTarget[L"Ellipse/Ellipse1/fill"].v = RGBA(0, 0, 0, 150);
					UIControlColorTarget[L"Ellipse/Ellipse1/frame"].v = !choose.select && !rubber.select ? brush.color : RGB(255, 255, 255);
					UIControlTarget[L"Image/Sign1/frame_transparency"].v = 255;
				}
				//圆角矩形
				{
					{
						UIControlTarget[L"RoundRect/RoundRect1/x"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 26;
						UIControlTarget[L"RoundRect/RoundRect1/y"].v = floating_windows.height - 155;
						UIControlTarget[L"RoundRect/RoundRect1/width"].v = 30;
						UIControlTarget[L"RoundRect/RoundRect1/height"].v = 94;

						if (BackgroundColorMode == 0)
						{
							UIControlColorTarget[L"RoundRect/RoundRect1/fill"].v = RGBA(255, 255, 255, 0);
							UIControlColorTarget[L"RoundRect/RoundRect1/frame"].v = RGBA(150, 150, 150, 0);
						}
						else if (BackgroundColorMode == 1)
						{
							UIControlColorTarget[L"RoundRect/RoundRect1/fill"].v = RGBA(30, 33, 41, 0);
							UIControlColorTarget[L"RoundRect/RoundRect1/frame"].v = RGBA(150, 150, 150, 0);
						}
					}
					{
						UIControlTarget[L"RoundRect/RoundRect2/x"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 33;
						UIControlTarget[L"RoundRect/RoundRect2/y"].v = floating_windows.height - 156 + 8;
						UIControlTarget[L"RoundRect/RoundRect2/width"].v = 80;
						UIControlTarget[L"RoundRect/RoundRect2/height"].v = 80;
						UIControlTarget[L"RoundRect/RoundRect2/ellipseheight"].v = 25;
						UIControlTarget[L"RoundRect/RoundRect2/ellipsewidth"].v = 25;

						UIControlColorTarget[L"RoundRect/RoundRect2/frame"].v = SET_ALPHA(UIControlColorTarget[L"RoundRect/RoundRect2/frame"].v, 0);
					}

					{
						UIControl[L"RoundRect/BrushTop/x"].s = 5;

						UIControlTarget[L"RoundRect/BrushTop/x"].v = floating_windows.width - 48;
						UIControlTarget[L"RoundRect/BrushTop/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v;
						UIControlTarget[L"RoundRect/BrushTop/width"].v = 80;
						UIControlTarget[L"RoundRect/BrushTop/height"].v = 90;
						UIControlTarget[L"RoundRect/BrushTop/ellipseheight"].v = 25;
						UIControlTarget[L"RoundRect/BrushTop/ellipsewidth"].v = 25;

						if (BackgroundColorMode == 0)
						{
							UIControlColorTarget[L"RoundRect/BrushTop/fill"].v = RGBA(255, 255, 255, 0);
							UIControlColorTarget[L"RoundRect/BrushTop/frame"].v = RGBA(150, 150, 150, 0);
						}
						else if (BackgroundColorMode == 1)
						{
							UIControlColorTarget[L"RoundRect/BrushTop/fill"].v = RGBA(30, 33, 41, 0);
							UIControlColorTarget[L"RoundRect/BrushTop/frame"].v = RGBA(150, 150, 150, 0);
						}
					}
					{
						{
							UIControl[L"RoundRect/BrushColor1/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColor1/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColor1/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColor1/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor1/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor1/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor1/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor1/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorFrame1/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorFrame1/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorFrame1/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorFrame1/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame1/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame1/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame1/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame1/thickness"].v = 1;
							UIControlColorTarget[L"RoundRect/BrushColorFrame1/frame"].v = RGBA(130, 130, 130, 0);

							UIControl[L"Image/BrushColor1/x"].s = 5;
							UIControlTarget[L"Image/BrushColor1/x"].v = floating_windows.width - 48 + 10;
							UIControlTarget[L"Image/BrushColor1/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10;
							UIControlTarget[L"Image/BrushColor1/transparency"].v = 0;
						}
						{
							UIControl[L"RoundRect/BrushColor2/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColor2/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColor2/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColor2/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor2/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor2/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor2/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor2/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorFrame2/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorFrame2/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorFrame2/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorFrame2/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame2/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame2/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame2/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame2/thickness"].v = 1;
							UIControlColorTarget[L"RoundRect/BrushColorFrame2/frame"].v = RGBA(130, 130, 130, 0);

							UIControl[L"Image/BrushColor2/x"].s = 5;
							UIControlTarget[L"Image/BrushColor2/x"].v = floating_windows.width - 48 + 10;
							UIControlTarget[L"Image/BrushColor2/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10;
							UIControlTarget[L"Image/BrushColor2/transparency"].v = 0;
						}
						{
							UIControl[L"RoundRect/BrushColor3/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColor3/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColor3/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColor3/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor3/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor3/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor3/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor3/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorFrame3/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorFrame3/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorFrame3/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorFrame3/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame3/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame3/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame3/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame3/thickness"].v = 1;
							UIControlColorTarget[L"RoundRect/BrushColorFrame3/frame"].v = RGBA(130, 130, 130, 0);

							UIControl[L"Image/BrushColor3/x"].s = 5;
							UIControlTarget[L"Image/BrushColor3/x"].v = floating_windows.width - 48 + 10;
							UIControlTarget[L"Image/BrushColor3/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10;
							UIControlTarget[L"Image/BrushColor3/transparency"].v = 0;
						}
						{
							UIControl[L"RoundRect/BrushColor4/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColor4/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColor4/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColor4/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor4/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor4/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor4/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor4/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorFrame4/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorFrame4/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorFrame4/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorFrame4/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame4/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame4/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame4/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame4/thickness"].v = 1;
							UIControlColorTarget[L"RoundRect/BrushColorFrame4/frame"].v = RGBA(130, 130, 130, 0);

							UIControl[L"Image/BrushColor4/x"].s = 5;
							UIControlTarget[L"Image/BrushColor4/x"].v = floating_windows.width - 48 + 10;
							UIControlTarget[L"Image/BrushColor4/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10;
							UIControlTarget[L"Image/BrushColor4/transparency"].v = 0;
						}
						{
							UIControl[L"RoundRect/BrushColor5/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColor5/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColor5/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColor5/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor5/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor5/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor5/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor5/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorFrame5/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorFrame5/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorFrame5/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorFrame5/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame5/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame5/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame5/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame5/thickness"].v = 1;
							UIControlColorTarget[L"RoundRect/BrushColorFrame5/frame"].v = RGBA(130, 130, 130, 0);

							UIControl[L"Image/BrushColor5/x"].s = 5;
							UIControlTarget[L"Image/BrushColor5/x"].v = floating_windows.width - 48 + 10;
							UIControlTarget[L"Image/BrushColor5/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10;
							UIControlTarget[L"Image/BrushColor5/transparency"].v = 0;
						}
						{
							UIControl[L"RoundRect/BrushColor6/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColor6/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColor6/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColor6/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor6/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor6/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor6/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor6/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorFrame6/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorFrame6/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorFrame6/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorFrame6/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame6/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame6/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame6/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame6/thickness"].v = 1;
							UIControlColorTarget[L"RoundRect/BrushColorFrame6/frame"].v = RGBA(130, 130, 130, 0);

							UIControl[L"Image/BrushColor6/x"].s = 5;
							UIControlTarget[L"Image/BrushColor6/x"].v = floating_windows.width - 48 + 10;
							UIControlTarget[L"Image/BrushColor6/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10;
							UIControlTarget[L"Image/BrushColor6/transparency"].v = 0;
						}
						{
							UIControl[L"RoundRect/BrushColor7/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColor7/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColor7/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColor7/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor7/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor7/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor7/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor7/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorFrame7/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorFrame7/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorFrame7/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorFrame7/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame7/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame7/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame7/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame7/thickness"].v = 1;
							UIControlColorTarget[L"RoundRect/BrushColorFrame7/frame"].v = RGBA(130, 130, 130, 0);

							UIControl[L"Image/BrushColor7/x"].s = 5;
							UIControlTarget[L"Image/BrushColor7/x"].v = floating_windows.width - 48 + 10;
							UIControlTarget[L"Image/BrushColor7/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10;
							UIControlTarget[L"Image/BrushColor7/transparency"].v = 0;
						}
						{
							UIControl[L"RoundRect/BrushColor8/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColor8/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColor8/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColor8/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor8/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor8/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor8/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor8/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorFrame8/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorFrame8/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorFrame8/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorFrame8/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame8/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame8/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame8/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame8/thickness"].v = 1;
							UIControlColorTarget[L"RoundRect/BrushColorFrame8/frame"].v = RGBA(130, 130, 130, 0);

							UIControl[L"Image/BrushColor8/x"].s = 5;
							UIControlTarget[L"Image/BrushColor8/x"].v = floating_windows.width - 48 + 10;
							UIControlTarget[L"Image/BrushColor8/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10;
							UIControlTarget[L"Image/BrushColor8/transparency"].v = 0;
						}
						{
							UIControl[L"RoundRect/BrushColor9/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColor9/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColor9/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColor9/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor9/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor9/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor9/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor9/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorFrame9/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorFrame9/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorFrame9/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorFrame9/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame9/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame9/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame9/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame9/thickness"].v = 1;
							UIControlColorTarget[L"RoundRect/BrushColorFrame9/frame"].v = RGBA(130, 130, 130, 0);

							UIControl[L"Image/BrushColor9/x"].s = 5;
							UIControlTarget[L"Image/BrushColor9/x"].v = floating_windows.width - 48 + 10;
							UIControlTarget[L"Image/BrushColor9/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10;
							UIControlTarget[L"Image/BrushColor9/transparency"].v = 0;
						}
						{
							UIControl[L"RoundRect/BrushColor10/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColor10/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColor10/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColor10/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor10/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor10/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor10/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor10/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorFrame10/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorFrame10/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorFrame10/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorFrame10/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame10/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame10/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame10/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame10/thickness"].v = 1;
							UIControlColorTarget[L"RoundRect/BrushColorFrame10/frame"].v = RGBA(130, 130, 130, 0);

							UIControl[L"Image/BrushColor10/x"].s = 5;
							UIControlTarget[L"Image/BrushColor10/x"].v = floating_windows.width - 48 + 10;
							UIControlTarget[L"Image/BrushColor10/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10;
							UIControlTarget[L"Image/BrushColor10/transparency"].v = 0;
						}
						{
							UIControl[L"RoundRect/BrushColor11/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColor11/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColor11/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColor11/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor11/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor11/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor11/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColor11/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorFrame11/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorFrame11/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorFrame11/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorFrame11/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame11/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame11/ellipseheight"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame11/ellipsewidth"].v = 10;
							UIControlTarget[L"RoundRect/BrushColorFrame11/thickness"].v = 1;
							UIControlColorTarget[L"RoundRect/BrushColorFrame11/frame"].v = RGBA(130, 130, 130, 0);

							UIControl[L"Image/BrushColor11/x"].s = 5;
							UIControlTarget[L"Image/BrushColor11/x"].v = floating_windows.width - 48 + 10;
							UIControlTarget[L"Image/BrushColor11/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 30 + 10;
							UIControlTarget[L"Image/BrushColor11/transparency"].v = 0;
						}
						{
							UIControl[L"RoundRect/BrushColor12/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColor12/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColor12/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColor12/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor12/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColor12/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorFrame12/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorFrame12/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorFrame12/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorFrame12/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame12/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame12/ellipseheight"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame12/ellipsewidth"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorFrame12/thickness"].v = 1;
							UIControlColorTarget[L"RoundRect/BrushColorFrame12/frame"].v = RGBA(130, 130, 130, 0);
						}
					}
					{
						UIControl[L"RoundRect/PaintThickness/x"].s = 5;

						UIControlTarget[L"RoundRect/PaintThickness/x"].v = floating_windows.width - 38;
						UIControlTarget[L"RoundRect/PaintThickness/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 10;
						UIControlTarget[L"RoundRect/PaintThickness/width"].v = 60;
						UIControlTarget[L"RoundRect/PaintThickness/height"].v = 70;
						UIControlTarget[L"RoundRect/PaintThickness/ellipseheight"].v = 25;
						UIControlTarget[L"RoundRect/PaintThickness/ellipsewidth"].v = 25;

						if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/PaintThickness/fill"].v = RGBA(230, 230, 230, 0);
						else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/PaintThickness/fill"].v = RGBA(70, 70, 70, 0);

						UIControl[L"RoundRect/PaintThicknessPrompt/x"].s = 5;

						UIControlTarget[L"RoundRect/PaintThicknessPrompt/x"].v = floating_windows.width - 10;
						UIControlTarget[L"RoundRect/PaintThicknessPrompt/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 10;
						UIControlTarget[L"RoundRect/PaintThicknessPrompt/width"].v = 5;
						UIControlTarget[L"RoundRect/PaintThicknessPrompt/height"].v = 5;
						UIControlTarget[L"RoundRect/PaintThicknessPrompt/ellipseheight"].v = 5;
						UIControlTarget[L"RoundRect/PaintThicknessPrompt/ellipsewidth"].v = 5;

						UIControlColorTarget[L"RoundRect/PaintThicknessPrompt/fill"].v = SET_ALPHA(brush.color, 0);

						{
							UIControl[L"RoundRect/PaintThicknessAdjust/x"].s = 5;

							UIControlTarget[L"RoundRect/PaintThicknessAdjust/x"].v = floating_windows.width - 38;
							UIControlTarget[L"RoundRect/PaintThicknessAdjust/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 10;
							UIControlTarget[L"RoundRect/PaintThicknessAdjust/width"].v = 60;
							UIControlTarget[L"RoundRect/PaintThicknessAdjust/height"].v = 70;
							UIControlTarget[L"RoundRect/PaintThicknessAdjust/ellipseheight"].v = 25;
							UIControlTarget[L"RoundRect/PaintThicknessAdjust/ellipsewidth"].v = 25;

							if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/PaintThicknessAdjust/fill"].v = RGBA(255, 255, 255, 0);
							else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/PaintThicknessAdjust/fill"].v = RGBA(30, 33, 41, 0);
							UIControlColorTarget[L"RoundRect/PaintThicknessAdjust/frame"].v = RGBA(150, 150, 150, 0);

							{
								UIControl[L"RoundRect/PaintThicknessSchedule1/x"].s = 5;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule1/x"].v = floating_windows.width - 38;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule1/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 42;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule1/width"].v = 60;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule1/height"].v = 6;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule1/ellipseheight"].v = 6;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule1/ellipsewidth"].v = 6;
								if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/PaintThicknessSchedule1/fill"].v = RGBA(230, 230, 230, 0);
								else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/PaintThicknessSchedule1/fill"].v = RGBA(70, 70, 70, 0);

								UIControl[L"RoundRect/PaintThicknessSchedule2/x"].s = 5;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule2/x"].v = floating_windows.width - 38;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule2/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 42;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule2/width"].v = 60;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule2/height"].v = 6;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule2/ellipseheight"].v = 6;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule2/ellipsewidth"].v = 6;
								UIControlColorTarget[L"RoundRect/PaintThicknessSchedule2/fill"].v = SET_ALPHA(brush.color, 0);

								UIControl[L"RoundRect/PaintThicknessSchedule3/x"].s = 5;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule3/x"].v = floating_windows.width - 8 - UIControlTarget[L"RoundRect/PaintThicknessSchedule3/width"].v / 2;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule3/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 35 - UIControlTarget[L"RoundRect/PaintThicknessSchedule3/width"].v / 2;
								UIControlColorTarget[L"RoundRect/PaintThicknessSchedule3/fill"].v = SET_ALPHA(brush.color, 0);

								UIControl[L"RoundRect/PaintThicknessSchedule4a/x"].s = 5;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/x"].v = floating_windows.width - 8 - UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v / 2;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 35 - UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v / 2;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v = 20;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/height"].v = 20;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v = 20;
								UIControlColorTarget[L"RoundRect/PaintThicknessSchedule4a/fill"].v = SET_ALPHA(brush.color, 0);

								UIControl[L"RoundRect/PaintThicknessSchedule5a/x"].s = 5;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/x"].v = floating_windows.width - 8 - UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/width"].v / 2;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 35 - UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/width"].v / 2;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/width"].v = 20;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/height"].v = 20;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/ellipse"].v = 20;
								UIControlColorTarget[L"RoundRect/PaintThicknessSchedule5a/fill"].v = SET_ALPHA(brush.color, 0);

								UIControl[L"RoundRect/PaintThicknessSchedule6a/x"].s = 5;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/x"].v = floating_windows.width - 8 - UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/width"].v / 2;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 35 - UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/width"].v / 2;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/width"].v = 20;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/height"].v = 20;
								UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v = 20;
								UIControlColorTarget[L"RoundRect/PaintThicknessSchedule6a/fill"].v = SET_ALPHA(brush.color, 0);
							}
						}
					}
					{
						{
							UIControl[L"RoundRect/BrushColorChoose/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorChoose/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorChoose/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorChoose/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChoose/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChoose/ellipseheight"].v = 5;
							UIControlTarget[L"RoundRect/BrushColorChoose/ellipsewidth"].v = 5;

							if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v = RGBA(255, 255, 255, 0);
							else UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v = RGBA(30, 33, 41, 0);
							UIControlColorTarget[L"RoundRect/BrushColorChoose/frame"].v = RGBA(130, 130, 130, 0);
						}
						{
							UIControl[L"RoundRect/BrushColorChooseWheel/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorChooseWheel/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorChooseWheel/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorChooseWheel/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseWheel/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseWheel/transparency"].v = 0;

							UIControl[L"RoundRect/BrushColorChooseMark/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorChooseMark/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorChooseMark/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorChooseMark/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseMark/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseMark/ellipseheight"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseMark/ellipsewidth"].v = 40;
							UIControlColorTarget[L"RoundRect/BrushColorChooseMark/fill"].v = SET_ALPHA(brush.color, 0);
							UIControlColorTarget[L"RoundRect/BrushColorChooseMark/frame"].v = RGBA(130, 130, 130, 0);

							UIControl[L"RoundRect/BrushColorChooseMarkR/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkR/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkR/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkR/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkR/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkR/ellipseheight"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkR/ellipsewidth"].v = 40;
							UIControlColorTarget[L"RoundRect/BrushColorChooseMarkR/fill"].v = RGBA(255, 0, 0, 0);
							UIControlColorTarget[L"RoundRect/BrushColorChooseMarkR/frame"].v = RGBA(255, 0, 0, 0);
							UIControlColorTarget[L"RoundRect/BrushColorChooseMarkR/text"].v = RGBA(255, 0, 0, 0);

							UIControl[L"RoundRect/BrushColorChooseMarkG/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkG/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkG/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkG/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkG/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkG/ellipseheight"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkG/ellipsewidth"].v = 40;
							UIControlColorTarget[L"RoundRect/BrushColorChooseMarkG/fill"].v = RGBA(0, 255, 0, 0);
							UIControlColorTarget[L"RoundRect/BrushColorChooseMarkG/frame"].v = RGBA(0, 255, 0, 0);
							UIControlColorTarget[L"RoundRect/BrushColorChooseMarkG/text"].v = RGBA(0, 255, 0, 0);

							UIControl[L"RoundRect/BrushColorChooseMarkB/x"].s = 5;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkB/x"].v = floating_windows.width - 48 + 20;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkB/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkB/width"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkB/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkB/ellipseheight"].v = 40;
							UIControlTarget[L"RoundRect/BrushColorChooseMarkB/ellipsewidth"].v = 40;
							UIControlColorTarget[L"RoundRect/BrushColorChooseMarkB/fill"].v = RGBA(0, 0, 255, 0);
							UIControlColorTarget[L"RoundRect/BrushColorChooseMarkB/frame"].v = RGBA(0, 0, 255, 0);
							UIControlColorTarget[L"RoundRect/BrushColorChooseMarkB/text"].v = RGBA(0, 0, 255, 0);
						}
					}

					{
						UIControl[L"RoundRect/BrushBottom/x"].s = 5;

						UIControlTarget[L"RoundRect/BrushBottom/x"].v = floating_windows.width - 48;
						UIControlTarget[L"RoundRect/BrushBottom/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + UIControlTarget[L"RoundRect/RoundRect1/height"].v - 40;
						UIControlTarget[L"RoundRect/BrushBottom/width"].v = 80;
						UIControlTarget[L"RoundRect/BrushBottom/height"].v = 40;
						UIControlTarget[L"RoundRect/BrushBottom/ellipseheight"].v = 25;
						UIControlTarget[L"RoundRect/BrushBottom/ellipsewidth"].v = 25;

						if (BackgroundColorMode == 0)
						{
							UIControlColorTarget[L"RoundRect/BrushBottom/fill"].v = RGBA(255, 255, 255, 0);
							UIControlColorTarget[L"RoundRect/BrushBottom/frame"].v = RGBA(150, 150, 150, 0);
						}
						else if (BackgroundColorMode == 1)
						{
							UIControlColorTarget[L"RoundRect/BrushBottom/fill"].v = RGBA(30, 33, 41, 0);
							UIControlColorTarget[L"RoundRect/BrushBottom/frame"].v = RGBA(150, 150, 150, 0);
						}
					}
					{
						UIControl[L"RoundRect/BrushChoose/x"].s = 5;

						UIControlTarget[L"RoundRect/BrushChoose/x"].v = floating_windows.width - 48;
						UIControlTarget[L"RoundRect/BrushChoose/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + UIControl[L"RoundRect/RoundRect1/height"].v - 35;
						UIControlTarget[L"RoundRect/BrushChoose/width"].v = 70;
						UIControlTarget[L"RoundRect/BrushChoose/height"].v = 30;
						UIControlTarget[L"RoundRect/BrushChoose/ellipseheight"].v = 15;
						UIControlTarget[L"RoundRect/BrushChoose/ellipsewidth"].v = 15;

						UIControlColorTarget[L"RoundRect/BrushChoose/frame"].v = SET_ALPHA(brush.color, 0);
					}
					{
						UIControl[L"RoundRect/BrushMode/x"].s = 5;

						UIControlTarget[L"RoundRect/BrushMode/x"].v = floating_windows.width - 48;
						UIControlTarget[L"RoundRect/BrushMode/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + UIControl[L"RoundRect/RoundRect1/height"].v - 35;
						UIControlTarget[L"RoundRect/BrushMode/width"].v = 70;
						UIControlTarget[L"RoundRect/BrushMode/height"].v = 30;
						UIControlTarget[L"RoundRect/BrushMode/ellipseheight"].v = 15;
						UIControlTarget[L"RoundRect/BrushMode/ellipsewidth"].v = 15;

						UIControlColorTarget[L"RoundRect/BrushMode/frame"].v = SET_ALPHA(brush.color, 0);
					}
					{
						UIControl[L"RoundRect/BrushInterval/x"].s = 5;

						UIControlTarget[L"RoundRect/BrushInterval/x"].v = (float)floating_windows.width - 48 + 34;
						UIControlTarget[L"RoundRect/BrushInterval/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + UIControl[L"RoundRect/RoundRect1/height"].v - 40 + 10;
						UIControlTarget[L"RoundRect/BrushInterval/width"].v = 3;
						UIControlTarget[L"RoundRect/BrushInterval/height"].v = 20;
						UIControlColorTarget[L"RoundRect/BrushInterval/fill"].v = RGBA(150, 150, 150, 0);
					}
				}
				//图像
				{
					{
						if (UIControl[L"Image/Sign1/transparency"].v <= 150) UIControlTarget[L"Image/Sign1/transparency"].v = 255;
						else if (UIControl[L"Image/Sign1/transparency"].v >= 255) UIControlTarget[L"Image/Sign1/transparency"].v = 150;
					}
					//选择
					{
						UIControlTarget[L"Image/choose/x"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 53;
						UIControlTarget[L"Image/choose/y"].v = floating_windows.height - 140;
						UIControlColorTarget[L"Image/choose/fill"].v = RGBA(98, 175, 82, 0);
					}
					//画笔
					{
						UIControlTarget[L"Image/brush/x"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 53;
						UIControlTarget[L"Image/brush/y"].v = floating_windows.height - 140;
						UIControlColorTarget[L"Image/brush/fill"].v = RGBA(130, 130, 130, 0);

						//画笔底部栏
						{
							{
								UIControl[L"Image/PaintBrush/x"].s = 5;

								UIControlTarget[L"Image/PaintBrush/x"].v = floating_windows.width - 48 + 10;
								UIControlTarget[L"Image/PaintBrush/y"].v = UIControlTarget[L"RoundRect/BrushBottom/y"].v + 10;

								if (brush.mode == 1 || brush.mode == 3 || brush.mode == 4) UIControlColorTarget[L"Image/PaintBrush/words_color"].v = SET_ALPHA(brush.color, 0);
								else UIControlColorTarget[L"Image/PaintBrush/words_color"].v = RGBA(130, 130, 130, 0);
							}
							{
								UIControl[L"Image/FluorescentBrush/x"].s = 5;

								UIControlTarget[L"Image/FluorescentBrush/x"].v = floating_windows.width - 48 + 10;
								UIControlTarget[L"Image/FluorescentBrush/y"].v = UIControlTarget[L"RoundRect/BrushBottom/y"].v + 10;

								if (brush.mode == 2) UIControlColorTarget[L"Image/FluorescentBrush/words_color"].v = SET_ALPHA(brush.color, 0);
								else UIControlColorTarget[L"Image/FluorescentBrush/words_color"].v = RGBA(130, 130, 130, 0);
							}

							{
								UIControl[L"Image/WriteBrush/x"].s = 5;

								UIControlTarget[L"Image/WriteBrush/x"].v = floating_windows.width - 48 + 10;
								UIControlTarget[L"Image/WriteBrush/y"].v = UIControlTarget[L"RoundRect/BrushBottom/y"].v + 10;

								if (brush.mode == 1 || brush.mode == 2) UIControlColorTarget[L"Image/WriteBrush/words_color"].v = SET_ALPHA(brush.color, 0);
								else UIControlColorTarget[L"Image/WriteBrush/words_color"].v = RGBA(130, 130, 130, 0);
							}
							{
								UIControl[L"Image/LineBrush/x"].s = 5;

								UIControlTarget[L"Image/LineBrush/x"].v = floating_windows.width - 48 + 10;
								UIControlTarget[L"Image/LineBrush/y"].v = UIControlTarget[L"RoundRect/BrushBottom/y"].v + 10;

								if (brush.mode == 1 || brush.mode == 2) UIControlColorTarget[L"Image/LineBrush/words_color"].v = SET_ALPHA(brush.color, 0);
								else UIControlColorTarget[L"Image/LineBrush/words_color"].v = RGBA(130, 130, 130, 0);
							}
							{
								UIControl[L"Image/RectangleBrush/x"].s = 5;

								UIControlTarget[L"Image/RectangleBrush/x"].v = floating_windows.width - 48 + 10;
								UIControlTarget[L"Image/RectangleBrush/y"].v = UIControlTarget[L"RoundRect/BrushBottom/y"].v + 10;

								if (brush.mode == 1 || brush.mode == 2) UIControlColorTarget[L"Image/RectangleBrush/words_color"].v = SET_ALPHA(brush.color, 0);
								else UIControlColorTarget[L"Image/RectangleBrush/words_color"].v = RGBA(130, 130, 130, 0);
							}
						}
					}
					//橡皮
					{
						UIControlTarget[L"Image/rubber/x"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 53;
						UIControlTarget[L"Image/rubber/y"].v = floating_windows.height - 140;
						UIControlColorTarget[L"Image/rubber/fill"].v = RGBA(130, 130, 130, 0);
					}
					//程序调测
					{
						UIControlTarget[L"Image/test/x"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 53;
						UIControlTarget[L"Image/test/y"].v = floating_windows.height - 140;
						UIControlColorTarget[L"Image/test/fill"].v = RGBA(130, 130, 130, 0);
					}
				}
				//文字
				{
					//选择
					{
						UIControlTarget[L"Words/choose/height"].v = 18;
						UIControlTarget[L"Words/choose/left"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 33;
						UIControlTarget[L"Words/choose/top"].v = floating_windows.height - 155 + 48;
						UIControlTarget[L"Words/choose/right"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 33 + 83;
						UIControlTarget[L"Words/choose/bottom"].v = floating_windows.height - 155 + 48 + 48;
						UIControlColorTarget[L"Words/choose/words_color"].v = SET_ALPHA(UIControlColorTarget[L"Words/choose/words_color"].v, 0);
					}
					//画笔
					{
						UIControlTarget[L"Words/brush/height"].v = 18;
						UIControlTarget[L"Words/brush/left"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 33;
						UIControlTarget[L"Words/brush/top"].v = floating_windows.height - 155 + 48;
						UIControlTarget[L"Words/brush/right"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 33 + 83;
						UIControlTarget[L"Words/brush/bottom"].v = floating_windows.height - 155 + 48 + 48;
						UIControlColorTarget[L"Words/brush/words_color"].v = SET_ALPHA(UIControlColorTarget[L"Words/brush/words_color"].v, 0);

						{
							UIControlTarget[L"Words/brushSize/left"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 33 + 45;
							UIControlTarget[L"Words/brushSize/top"].v = floating_windows.height - 155 + 48 - 12;

							UIControlColorTarget[L"Words/brushSize/words_color"].v = SET_ALPHA(brush.color, 0);
						}

						//画笔顶部栏
						{
							//画笔粗细
							{
								UIControlTarget[L"Words/PaintThickness/left"].s = 5;

								UIControlTarget[L"Words/PaintThickness/size"].v = 20;
								UIControlTarget[L"Words/PaintThickness/left"].v = floating_windows.width - 48;
								UIControlTarget[L"Words/PaintThickness/top"].v = UIControlTarget[L"RoundRect/BrushTop/y"].v;
								UIControlTarget[L"Words/PaintThickness/width"].v = 50;
								UIControlTarget[L"Words/PaintThickness/height"].v = 96;
								UIControlColorTarget[L"Words/PaintThickness/words_color"].v = SET_ALPHA(brush.color, 255);

								UIControlTarget[L"Words/PaintThicknessValue/left"].s = 5;

								UIControlTarget[L"Words/PaintThicknessValue/size"].v = 20;
								UIControlTarget[L"Words/PaintThicknessValue/left"].v = floating_windows.width - 48;
								UIControlTarget[L"Words/PaintThicknessValue/top"].v = UIControlTarget[L"RoundRect/BrushTop/y"].v;
								UIControlTarget[L"Words/PaintThicknessValue/width"].v = 50;
								UIControlTarget[L"Words/PaintThicknessValue/height"].v = 96;
								UIControlColorTarget[L"Words/PaintThicknessValue/words_color"].v = SET_ALPHA(brush.color, 255);
							}
						}
						//画笔底部栏
						{
							{
								UIControlTarget[L"Words/PaintBrush/left"].s = 5;

								UIControlTarget[L"Words/PaintBrush/size"].v = 18;
								UIControlTarget[L"Words/PaintBrush/left"].v = UIControlTarget[L"RoundRect/BrushChoose/x"].v + 20;
								UIControlTarget[L"Words/PaintBrush/top"].v = UIControlTarget[L"RoundRect/BrushChoose/y"].v;
								UIControlTarget[L"Words/PaintBrush/width"].v = 60;
								UIControlTarget[L"Words/PaintBrush/height"].v = 33;

								if (brush.mode == 1 || brush.mode == 3 || brush.mode == 4) UIControlColorTarget[L"Words/PaintBrush/words_color"].v = SET_ALPHA(brush.color, 0);
								else UIControlColorTarget[L"Words/PaintBrush/words_color"].v = RGBA(130, 130, 130, 0);
							}
							{
								UIControlTarget[L"Words/FluorescentBrush/left"].s = 5;

								UIControlTarget[L"Words/FluorescentBrush/size"].v = 18;
								UIControlTarget[L"Words/FluorescentBrush/left"].v = UIControlTarget[L"RoundRect/BrushChoose/x"].v + 20;
								UIControlTarget[L"Words/FluorescentBrush/top"].v = UIControlTarget[L"RoundRect/BrushChoose/y"].v;
								UIControlTarget[L"Words/FluorescentBrush/width"].v = 65;
								UIControlTarget[L"Words/FluorescentBrush/height"].v = 33;

								if (brush.mode == 2) UIControlColorTarget[L"Words/FluorescentBrush/words_color"].v = SET_ALPHA(brush.color, 0);
								else UIControlColorTarget[L"Words/FluorescentBrush/words_color"].v = RGBA(130, 130, 130, 0);
							}

							{
								UIControlTarget[L"Words/WriteBrush/left"].s = 5;

								UIControlTarget[L"Words/WriteBrush/size"].v = 18;
								UIControlTarget[L"Words/WriteBrush/left"].v = UIControlTarget[L"RoundRect/BrushMode/x"].v + 20;
								UIControlTarget[L"Words/WriteBrush/top"].v = UIControlTarget[L"RoundRect/BrushMode/y"].v;
								UIControlTarget[L"Words/WriteBrush/width"].v = 60;
								UIControlTarget[L"Words/WriteBrush/height"].v = 33;

								if (brush.mode == 1 || brush.mode == 2) UIControlColorTarget[L"Words/WriteBrush/words_color"].v = SET_ALPHA(brush.color, 0);
								else UIControlColorTarget[L"Words/WriteBrush/words_color"].v = RGBA(130, 130, 130, 0);
							}
							{
								UIControlTarget[L"Words/LineBrush/left"].s = 5;

								UIControlTarget[L"Words/LineBrush/size"].v = 18;
								UIControlTarget[L"Words/LineBrush/left"].v = UIControlTarget[L"RoundRect/BrushMode/x"].v + 20;
								UIControlTarget[L"Words/LineBrush/top"].v = UIControlTarget[L"RoundRect/BrushMode/y"].v;
								UIControlTarget[L"Words/LineBrush/width"].v = 60;
								UIControlTarget[L"Words/LineBrush/height"].v = 33;

								if (brush.mode == 3) UIControlColorTarget[L"Words/LineBrush/words_color"].v = SET_ALPHA(brush.color, 0);
								else UIControlColorTarget[L"Words/LineBrush/words_color"].v = RGBA(130, 130, 130, 0);
							}
							{
								UIControlTarget[L"Words/RectangleBrush/left"].s = 5;

								UIControlTarget[L"Words/RectangleBrush/size"].v = 18;
								UIControlTarget[L"Words/RectangleBrush/left"].v = UIControlTarget[L"RoundRect/BrushMode/x"].v + 20;
								UIControlTarget[L"Words/RectangleBrush/top"].v = UIControlTarget[L"RoundRect/BrushMode/y"].v;
								UIControlTarget[L"Words/RectangleBrush/width"].v = 60;
								UIControlTarget[L"Words/RectangleBrush/height"].v = 33;

								if (brush.mode == 3) UIControlColorTarget[L"Words/RectangleBrush/words_color"].v = SET_ALPHA(brush.color, 0);
								else UIControlColorTarget[L"Words/RectangleBrush/words_color"].v = RGBA(130, 130, 130, 0);
							}
						}
					}
					//橡皮
					{
						UIControlTarget[L"Words/rubber/height"].v = 18;
						UIControlTarget[L"Words/rubber/left"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 33;
						UIControlTarget[L"Words/rubber/top"].v = floating_windows.height - 155 + 48;
						UIControlTarget[L"Words/rubber/right"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 33 + 83;
						UIControlTarget[L"Words/rubber/bottom"].v = floating_windows.height - 155 + 48 + 48;
						UIControlColorTarget[L"Words/rubber/words_color"].v = SET_ALPHA(UIControlColorTarget[L"Words/rubber/words_color"].v, 0);
					}
					//程序调测
					{
						UIControlTarget[L"Words/test/height"].v = 18;
						UIControlTarget[L"Words/test/left"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 33;
						UIControlTarget[L"Words/test/top"].v = floating_windows.height - 155 + 48;
						UIControlTarget[L"Words/test/right"].v = UIControl[L"Ellipse/Ellipse1/x"].v + 33 + 83;
						UIControlTarget[L"Words/test/bottom"].v = floating_windows.height - 155 + 48 + 48;
						UIControlColorTarget[L"Words/test/words_color"].v = SET_ALPHA(UIControlColorTarget[L"Words/test/words_color"].v, 0);
					}
				}
			}
			else if ((int)state == 1)
			{
				//圆形
				{
					UIControlColorTarget[L"Ellipse/Ellipse1/fill"].v = RGBA(0, 111, 225, 255);
					UIControlColorTarget[L"Ellipse/Ellipse1/frame"].v = RGBA(0, 111, 225, 255);
					UIControlTarget[L"Image/Sign1/frame_transparency"].v = 255;
				}
				//圆角矩形
				{
					{
						UIControlTarget[L"RoundRect/RoundRect1/x"].v = 1;
						UIControlTarget[L"RoundRect/RoundRect1/y"].v = floating_windows.height - 155;
						UIControlTarget[L"RoundRect/RoundRect1/width"].v = floating_windows.width - 48 + 8;
						UIControlTarget[L"RoundRect/RoundRect1/height"].v = 94;

						if (BackgroundColorMode == 0)
						{
							UIControlColorTarget[L"RoundRect/RoundRect1/fill"].v = RGBA(255, 255, 255, 255);
							UIControlColorTarget[L"RoundRect/RoundRect1/frame"].v = RGBA(150, 150, 150, 255);
						}
						else if (BackgroundColorMode == 1)
						{
							UIControlColorTarget[L"RoundRect/RoundRect1/fill"].v = RGBA(30, 33, 41, 255);
							UIControlColorTarget[L"RoundRect/RoundRect1/frame"].v = RGBA(150, 150, 150, 255);
						}
					}
					{
						if (choose.select == true)
						{
							UIControlTarget[L"RoundRect/RoundRect2/x"].v = 0 + 8;
							UIControlTarget[L"RoundRect/RoundRect2/y"].v = floating_windows.height - 156 + 8;
							UIControlTarget[L"RoundRect/RoundRect2/width"].v = 80;
							UIControlTarget[L"RoundRect/RoundRect2/height"].v = 80;
							UIControlTarget[L"RoundRect/RoundRect2/ellipseheight"].v = 25;
							UIControlTarget[L"RoundRect/RoundRect2/ellipsewidth"].v = 25;

							if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/RoundRect2/frame"].v = RGBA(98, 175, 82, 255);
							else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/RoundRect2/frame"].v = RGBA(98, 175, 82, 255);
						}
						else if (brush.select == true)
						{
							UIControlTarget[L"RoundRect/RoundRect2/x"].v = 96 + 8;
							UIControlTarget[L"RoundRect/RoundRect2/y"].v = floating_windows.height - 156 + 8;
							UIControlTarget[L"RoundRect/RoundRect2/width"].v = 80;
							UIControlTarget[L"RoundRect/RoundRect2/height"].v = 80;
							UIControlTarget[L"RoundRect/RoundRect2/ellipseheight"].v = 25;
							UIControlTarget[L"RoundRect/RoundRect2/ellipsewidth"].v = 25;

							UIControlColorTarget[L"RoundRect/RoundRect2/frame"].v = SET_ALPHA(brush.color, 255);
						}
						else if (rubber.select == true)
						{
							UIControlTarget[L"RoundRect/RoundRect2/x"].v = 192 + 8;
							UIControlTarget[L"RoundRect/RoundRect2/y"].v = floating_windows.height - 156 + 8;
							UIControlTarget[L"RoundRect/RoundRect2/width"].v = 80;
							UIControlTarget[L"RoundRect/RoundRect2/height"].v = 80;
							UIControlTarget[L"RoundRect/RoundRect2/ellipseheight"].v = 25;
							UIControlTarget[L"RoundRect/RoundRect2/ellipsewidth"].v = 25;

							if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/RoundRect2/frame"].v = RGBA(98, 175, 82, 255);
							else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/RoundRect2/frame"].v = RGBA(98, 175, 82, 255);
						}
						else if (test.select == true)
						{
							UIControlTarget[L"RoundRect/RoundRect2/x"].v = 288 + 8;
							UIControlTarget[L"RoundRect/RoundRect2/y"].v = floating_windows.height - 156 + 8;
							UIControlTarget[L"RoundRect/RoundRect2/width"].v = 80;
							UIControlTarget[L"RoundRect/RoundRect2/height"].v = 80;
							UIControlTarget[L"RoundRect/RoundRect2/ellipseheight"].v = 25;
							UIControlTarget[L"RoundRect/RoundRect2/ellipsewidth"].v = 25;

							if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/RoundRect2/frame"].v = RGBA(98, 175, 82, 255);
							else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/RoundRect2/frame"].v = RGBA(98, 175, 82, 255);
						}
					}

					{
						UIControl[L"RoundRect/BrushTop/x"].s = 3;

						if (state == 1.1 || state == 1.11 || state == 1.12)
						{
							UIControlTarget[L"RoundRect/BrushTop/x"].v = 1;
							UIControlTarget[L"RoundRect/BrushTop/y"].v = floating_windows.height - 257;
							UIControlTarget[L"RoundRect/BrushTop/width"].v = floating_windows.width - 106;
							UIControlTarget[L"RoundRect/BrushTop/height"].v = 96;
							UIControlTarget[L"RoundRect/BrushTop/ellipseheight"].v = 25;
							UIControlTarget[L"RoundRect/BrushTop/ellipsewidth"].v = 25;

							if (BackgroundColorMode == 0)
							{
								UIControlColorTarget[L"RoundRect/BrushTop/fill"].v = RGBA(255, 255, 255, 255);
								UIControlColorTarget[L"RoundRect/BrushTop/frame"].v = RGBA(150, 150, 150, 255);
							}
							else if (BackgroundColorMode == 1)
							{
								UIControlColorTarget[L"RoundRect/BrushTop/fill"].v = RGBA(30, 33, 41, 255);
								UIControlColorTarget[L"RoundRect/BrushTop/frame"].v = RGBA(150, 150, 150, 255);
							}
						}
						else
						{
							UIControlTarget[L"RoundRect/BrushTop/x"].v = 96 + 8;
							UIControlTarget[L"RoundRect/BrushTop/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v;
							UIControlTarget[L"RoundRect/BrushTop/width"].v = UIControl[L"RoundRect/RoundRect2/width"].v;
							UIControlTarget[L"RoundRect/BrushTop/height"].v = 90;
							UIControlTarget[L"RoundRect/BrushTop/ellipseheight"].v = 25;
							UIControlTarget[L"RoundRect/BrushTop/ellipsewidth"].v = 25;

							if (BackgroundColorMode == 0)
							{
								UIControlColorTarget[L"RoundRect/BrushTop/fill"].v = RGBA(255, 255, 255, 0);
								UIControlColorTarget[L"RoundRect/BrushTop/frame"].v = RGBA(150, 150, 150, 0);
							}
							else if (BackgroundColorMode == 1)
							{
								UIControlColorTarget[L"RoundRect/BrushTop/fill"].v = RGBA(30, 33, 41, 0);
								UIControlColorTarget[L"RoundRect/BrushTop/frame"].v = RGBA(150, 150, 150, 0);
							}
						}
					}
					{
						{
							UIControl[L"RoundRect/BrushColor1/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColor1/x"].v = 1 + 10;
								UIControlTarget[L"RoundRect/BrushColor1/y"].v = (floating_windows.height - 257 + 6);
								UIControlTarget[L"RoundRect/BrushColor1/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor1/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor1/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor1/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor1/transparency"].v = 255;
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColor1/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColor1/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColor1/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor1/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor1/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor1/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor1/transparency"].v = 0;
							}

							UIControl[L"RoundRect/BrushColorFrame1/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColorFrame1/x"].v = UIControlTarget[L"RoundRect/BrushColor1/x"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame1/y"].v = UIControlTarget[L"RoundRect/BrushColor1/y"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame1/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame1/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame1/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame1/ellipsewidth"].v = 10;
								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor1/fill"].v, 255))
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame1/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame1/thickness"].v = 3;
								}
								else
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame1/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame1/thickness"].v = 1;
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorFrame1/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorFrame1/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorFrame1/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame1/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame1/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame1/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame1/thickness"].v = 1;
								UIControlColorTarget[L"RoundRect/BrushColorFrame1/frame"].v = RGBA(130, 130, 130, 0);
							}

							UIControl[L"Image/BrushColor1/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"Image/BrushColor1/x"].v = UIControlTarget[L"RoundRect/BrushColor1/x"].v + 10;
								UIControlTarget[L"Image/BrushColor1/y"].v = UIControlTarget[L"RoundRect/BrushColor1/y"].v + 10;

								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor1/fill"].v, 255)) UIControlTarget[L"Image/BrushColor1/transparency"].v = 255;
								else UIControlTarget[L"Image/BrushColor1/transparency"].v = 0;
							}
							else
							{
								UIControlTarget[L"Image/BrushColor1/x"].v = 96 + 8 + 20 + 10;
								UIControlTarget[L"Image/BrushColor1/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25 + 10;
								UIControlTarget[L"Image/BrushColor1/transparency"].v = 0;
							}
						}
						{
							UIControl[L"RoundRect/BrushColor2/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColor2/x"].v = 1 + 10;
								UIControlTarget[L"RoundRect/BrushColor2/y"].v = (floating_windows.height - 257 + 6) + 44;
								UIControlTarget[L"RoundRect/BrushColor2/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor2/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor2/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor2/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor2/transparency"].v = 255;
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColor2/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColor2/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColor2/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor2/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor2/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor2/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor2/transparency"].v = 0;
							}

							UIControl[L"RoundRect/BrushColorFrame2/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColorFrame2/x"].v = UIControlTarget[L"RoundRect/BrushColor2/x"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame2/y"].v = UIControlTarget[L"RoundRect/BrushColor2/y"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame2/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame2/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame2/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame2/ellipsewidth"].v = 10;
								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor2/fill"].v, 255))
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame2/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame2/thickness"].v = 3;
								}
								else
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame2/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame2/thickness"].v = 1;
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorFrame2/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorFrame2/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorFrame2/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame2/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame2/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame2/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame2/thickness"].v = 1;
								UIControlColorTarget[L"RoundRect/BrushColorFrame2/frame"].v = RGBA(130, 130, 130, 0);
							}

							UIControl[L"Image/BrushColor2/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"Image/BrushColor2/x"].v = UIControlTarget[L"RoundRect/BrushColor2/x"].v + 10;
								UIControlTarget[L"Image/BrushColor2/y"].v = UIControlTarget[L"RoundRect/BrushColor2/y"].v + 10;

								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor2/fill"].v, 255)) UIControlTarget[L"Image/BrushColor2/transparency"].v = 255;
								else UIControlTarget[L"Image/BrushColor2/transparency"].v = 0;
							}
							else
							{
								UIControlTarget[L"Image/BrushColor2/x"].v = 96 + 8 + 20 + 10;
								UIControlTarget[L"Image/BrushColor2/y"].v = UIControl[L"RoundRect/RoundRect2/y"].v + 25 + 10;
								UIControlTarget[L"Image/BrushColor2/transparency"].v = 0;
							}
						}
						{
							UIControl[L"RoundRect/BrushColor3/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColor3/x"].v = 11 + 44;
								UIControlTarget[L"RoundRect/BrushColor3/y"].v = (floating_windows.height - 257 + 6);
								UIControlTarget[L"RoundRect/BrushColor3/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor3/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor3/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor3/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor3/transparency"].v = 255;
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColor3/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColor3/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColor3/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor3/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor3/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor3/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor3/transparency"].v = 0;
							}

							UIControl[L"RoundRect/BrushColorFrame3/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColorFrame3/x"].v = UIControlTarget[L"RoundRect/BrushColor3/x"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame3/y"].v = UIControlTarget[L"RoundRect/BrushColor3/y"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame3/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame3/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame3/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame3/ellipsewidth"].v = 10;
								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor3/fill"].v, 255))
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame3/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame3/thickness"].v = 3;
								}
								else
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame3/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame3/thickness"].v = 1;
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorFrame3/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorFrame3/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorFrame3/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame3/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame3/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame3/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame3/thickness"].v = 1;
								UIControlColorTarget[L"RoundRect/BrushColorFrame3/frame"].v = RGBA(130, 130, 130, 0);
							}

							UIControl[L"Image/BrushColor3/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"Image/BrushColor3/x"].v = UIControlTarget[L"RoundRect/BrushColor3/x"].v + 10;
								UIControlTarget[L"Image/BrushColor3/y"].v = UIControlTarget[L"RoundRect/BrushColor3/y"].v + 10;

								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor3/fill"].v, 255)) UIControlTarget[L"Image/BrushColor3/transparency"].v = 255;
								else UIControlTarget[L"Image/BrushColor3/transparency"].v = 0;
							}
							else
							{
								UIControlTarget[L"Image/BrushColor3/x"].v = 96 + 8 + 20 + 10;
								UIControlTarget[L"Image/BrushColor3/y"].v = UIControl[L"RoundRect/RoundRect2/y"].v + 25 + 10;
								UIControlTarget[L"Image/BrushColor3/transparency"].v = 0;
							}
						}
						{
							UIControl[L"RoundRect/BrushColor4/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColor4/x"].v = 11 + 44;
								UIControlTarget[L"RoundRect/BrushColor4/y"].v = (floating_windows.height - 257 + 6) + 44;
								UIControlTarget[L"RoundRect/BrushColor4/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor4/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor4/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor4/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor4/transparency"].v = 255;
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColor4/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColor4/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColor4/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor4/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor4/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor4/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor4/transparency"].v = 0;
							}

							UIControl[L"RoundRect/BrushColorFrame4/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColorFrame4/x"].v = UIControlTarget[L"RoundRect/BrushColor4/x"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame4/y"].v = UIControlTarget[L"RoundRect/BrushColor4/y"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame4/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame4/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame4/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame4/ellipsewidth"].v = 10;
								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor4/fill"].v, 255))
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame4/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame4/thickness"].v = 3;
								}
								else
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame4/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame4/thickness"].v = 1;
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorFrame4/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorFrame4/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorFrame4/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame4/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame4/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame4/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame4/thickness"].v = 1;
								UIControlColorTarget[L"RoundRect/BrushColorFrame4/frame"].v = RGBA(130, 130, 130, 0);
							}

							UIControl[L"Image/BrushColor4/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"Image/BrushColor4/x"].v = UIControlTarget[L"RoundRect/BrushColor4/x"].v + 10;
								UIControlTarget[L"Image/BrushColor4/y"].v = UIControlTarget[L"RoundRect/BrushColor4/y"].v + 10;

								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor4/fill"].v, 255)) UIControlTarget[L"Image/BrushColor4/transparency"].v = 255;
								else UIControlTarget[L"Image/BrushColor4/transparency"].v = 0;
							}
							else
							{
								UIControlTarget[L"Image/BrushColor4/x"].v = 96 + 8 + 20 + 10;
								UIControlTarget[L"Image/BrushColor4/y"].v = UIControl[L"RoundRect/RoundRect2/y"].v + 25 + 10;
								UIControlTarget[L"Image/BrushColor4/transparency"].v = 0;
							}
						}
						{
							UIControl[L"RoundRect/BrushColor5/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColor5/x"].v = 11 + 44 * 2;
								UIControlTarget[L"RoundRect/BrushColor5/y"].v = (floating_windows.height - 257 + 6);
								UIControlTarget[L"RoundRect/BrushColor5/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor5/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor5/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor5/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor5/transparency"].v = 255;
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColor5/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColor5/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColor5/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor5/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor5/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor5/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor5/transparency"].v = 0;
							}

							UIControl[L"RoundRect/BrushColorFrame5/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColorFrame5/x"].v = UIControlTarget[L"RoundRect/BrushColor5/x"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame5/y"].v = UIControlTarget[L"RoundRect/BrushColor5/y"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame5/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame5/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame5/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame5/ellipsewidth"].v = 10;
								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor5/fill"].v, 255))
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame5/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame5/thickness"].v = 3;
								}
								else
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame5/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame5/thickness"].v = 1;
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorFrame5/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorFrame5/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorFrame5/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame5/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame5/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame5/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame5/thickness"].v = 1;
								UIControlColorTarget[L"RoundRect/BrushColorFrame5/frame"].v = RGBA(130, 130, 130, 0);
							}

							UIControl[L"Image/BrushColor5/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"Image/BrushColor5/x"].v = UIControlTarget[L"RoundRect/BrushColor5/x"].v + 10;
								UIControlTarget[L"Image/BrushColor5/y"].v = UIControlTarget[L"RoundRect/BrushColor5/y"].v + 10;

								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor5/fill"].v, 255)) UIControlTarget[L"Image/BrushColor5/transparency"].v = 255;
								else UIControlTarget[L"Image/BrushColor5/transparency"].v = 0;
							}
							else
							{
								UIControlTarget[L"Image/BrushColor5/x"].v = 96 + 8 + 20 + 10;
								UIControlTarget[L"Image/BrushColor5/y"].v = UIControl[L"RoundRect/RoundRect2/y"].v + 25 + 10;
								UIControlTarget[L"Image/BrushColor5/transparency"].v = 0;
							}
						}
						{
							UIControl[L"RoundRect/BrushColor6/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColor6/x"].v = 11 + 44 * 2;
								UIControlTarget[L"RoundRect/BrushColor6/y"].v = (floating_windows.height - 257 + 6) + 44;
								UIControlTarget[L"RoundRect/BrushColor6/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor6/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor6/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor6/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor6/transparency"].v = 255;
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColor6/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColor6/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColor6/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor6/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor6/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor6/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor6/transparency"].v = 0;
							}

							UIControl[L"RoundRect/BrushColorFrame6/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColorFrame6/x"].v = UIControlTarget[L"RoundRect/BrushColor6/x"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame6/y"].v = UIControlTarget[L"RoundRect/BrushColor6/y"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame6/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame6/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame6/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame6/ellipsewidth"].v = 10;
								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor6/fill"].v, 255))
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame6/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame6/thickness"].v = 3;
								}
								else
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame6/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame6/thickness"].v = 1;
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorFrame6/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorFrame6/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorFrame6/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame6/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame6/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame6/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame6/thickness"].v = 1;
								UIControlColorTarget[L"RoundRect/BrushColorFrame6/frame"].v = RGBA(130, 130, 130, 0);
							}

							UIControl[L"Image/BrushColor6/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"Image/BrushColor6/x"].v = UIControlTarget[L"RoundRect/BrushColor6/x"].v + 10;
								UIControlTarget[L"Image/BrushColor6/y"].v = UIControlTarget[L"RoundRect/BrushColor6/y"].v + 10;

								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor6/fill"].v, 255)) UIControlTarget[L"Image/BrushColor6/transparency"].v = 255;
								else UIControlTarget[L"Image/BrushColor6/transparency"].v = 0;
							}
							else
							{
								UIControlTarget[L"Image/BrushColor6/x"].v = 96 + 8 + 20 + 10;
								UIControlTarget[L"Image/BrushColor6/y"].v = UIControl[L"RoundRect/RoundRect2/y"].v + 25 + 10;
								UIControlTarget[L"Image/BrushColor6/transparency"].v = 0;
							}
						}
						{
							UIControl[L"RoundRect/BrushColor7/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColor7/x"].v = 11 + 44 * 3;
								UIControlTarget[L"RoundRect/BrushColor7/y"].v = (floating_windows.height - 257 + 6);
								UIControlTarget[L"RoundRect/BrushColor7/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor7/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor7/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor7/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor7/transparency"].v = 255;
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColor7/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColor7/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColor7/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor7/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor7/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor7/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor7/transparency"].v = 0;
							}

							UIControl[L"RoundRect/BrushColorFrame7/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColorFrame7/x"].v = UIControlTarget[L"RoundRect/BrushColor7/x"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame7/y"].v = UIControlTarget[L"RoundRect/BrushColor7/y"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame7/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame7/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame7/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame7/ellipsewidth"].v = 10;
								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor7/fill"].v, 255))
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame7/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame7/thickness"].v = 3;
								}
								else
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame7/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame7/thickness"].v = 1;
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorFrame7/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorFrame7/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorFrame7/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame7/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame7/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame7/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame7/thickness"].v = 1;
								UIControlColorTarget[L"RoundRect/BrushColorFrame7/frame"].v = RGBA(130, 130, 130, 0);
							}

							UIControl[L"Image/BrushColor7/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"Image/BrushColor7/x"].v = UIControlTarget[L"RoundRect/BrushColor7/x"].v + 10;
								UIControlTarget[L"Image/BrushColor7/y"].v = UIControlTarget[L"RoundRect/BrushColor7/y"].v + 10;

								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor7/fill"].v, 255)) UIControlTarget[L"Image/BrushColor7/transparency"].v = 255;
								else UIControlTarget[L"Image/BrushColor7/transparency"].v = 0;
							}
							else
							{
								UIControlTarget[L"Image/BrushColor7/x"].v = 96 + 8 + 20 + 10;
								UIControlTarget[L"Image/BrushColor7/y"].v = UIControl[L"RoundRect/RoundRect2/y"].v + 25 + 10;
								UIControlTarget[L"Image/BrushColor7/transparency"].v = 0;
							}
						}
						{
							UIControl[L"RoundRect/BrushColor8/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColor8/x"].v = 11 + 44 * 3;
								UIControlTarget[L"RoundRect/BrushColor8/y"].v = (floating_windows.height - 257 + 6) + 44;
								UIControlTarget[L"RoundRect/BrushColor8/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor8/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor8/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor8/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor8/transparency"].v = 255;
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColor8/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColor8/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColor8/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor8/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor8/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor8/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor8/transparency"].v = 0;
							}

							UIControl[L"RoundRect/BrushColorFrame8/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColorFrame8/x"].v = UIControlTarget[L"RoundRect/BrushColor8/x"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame8/y"].v = UIControlTarget[L"RoundRect/BrushColor8/y"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame8/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame8/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame8/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame8/ellipsewidth"].v = 10;
								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor8/fill"].v, 255))
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame8/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame8/thickness"].v = 3;
								}
								else
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame8/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame8/thickness"].v = 1;
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorFrame8/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorFrame8/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorFrame8/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame8/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame8/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame8/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame8/thickness"].v = 1;
								UIControlColorTarget[L"RoundRect/BrushColorFrame8/frame"].v = RGBA(130, 130, 130, 0);
							}

							UIControl[L"Image/BrushColor8/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"Image/BrushColor8/x"].v = UIControlTarget[L"RoundRect/BrushColor8/x"].v + 10;
								UIControlTarget[L"Image/BrushColor8/y"].v = UIControlTarget[L"RoundRect/BrushColor8/y"].v + 10;

								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor8/fill"].v, 255)) UIControlTarget[L"Image/BrushColor8/transparency"].v = 255;
								else UIControlTarget[L"Image/BrushColor8/transparency"].v = 0;
							}
							else
							{
								UIControlTarget[L"Image/BrushColor8/x"].v = 96 + 8 + 20 + 10;
								UIControlTarget[L"Image/BrushColor8/y"].v = UIControl[L"RoundRect/RoundRect2/y"].v + 25 + 10;
								UIControlTarget[L"Image/BrushColor8/transparency"].v = 0;
							}
						}
						{
							UIControl[L"RoundRect/BrushColor9/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColor9/x"].v = 11 + 44 * 4;
								UIControlTarget[L"RoundRect/BrushColor9/y"].v = (floating_windows.height - 257 + 6);
								UIControlTarget[L"RoundRect/BrushColor9/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor9/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor9/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor9/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor9/transparency"].v = 255;
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColor9/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColor9/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColor9/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor9/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor9/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor9/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor9/transparency"].v = 0;
							}

							UIControl[L"RoundRect/BrushColorFrame9/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColorFrame9/x"].v = UIControlTarget[L"RoundRect/BrushColor9/x"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame9/y"].v = UIControlTarget[L"RoundRect/BrushColor9/y"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame9/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame9/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame9/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame9/ellipsewidth"].v = 10;
								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor9/fill"].v, 255))
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame9/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame9/thickness"].v = 3;
								}
								else
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame9/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame9/thickness"].v = 1;
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorFrame9/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorFrame9/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorFrame9/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame9/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame9/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame9/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame9/thickness"].v = 1;
								UIControlColorTarget[L"RoundRect/BrushColorFrame9/frame"].v = RGBA(130, 130, 130, 0);
							}

							UIControl[L"Image/BrushColor9/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"Image/BrushColor9/x"].v = UIControlTarget[L"RoundRect/BrushColor9/x"].v + 10;
								UIControlTarget[L"Image/BrushColor9/y"].v = UIControlTarget[L"RoundRect/BrushColor9/y"].v + 10;

								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor9/fill"].v, 255)) UIControlTarget[L"Image/BrushColor9/transparency"].v = 255;
								else UIControlTarget[L"Image/BrushColor9/transparency"].v = 0;
							}
							else
							{
								UIControlTarget[L"Image/BrushColor9/x"].v = 96 + 8 + 20 + 10;
								UIControlTarget[L"Image/BrushColor9/y"].v = UIControl[L"RoundRect/RoundRect2/y"].v + 25 + 10;
								UIControlTarget[L"Image/BrushColor9/transparency"].v = 0;
							}
						}
						{
							UIControl[L"RoundRect/BrushColor10/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColor10/x"].v = 11 + 44 * 4;
								UIControlTarget[L"RoundRect/BrushColor10/y"].v = (floating_windows.height - 257 + 6) + 44;
								UIControlTarget[L"RoundRect/BrushColor10/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor10/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor10/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor10/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor10/transparency"].v = 255;
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColor10/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColor10/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColor10/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor10/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor10/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor10/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor10/transparency"].v = 0;
							}

							UIControl[L"RoundRect/BrushColorFrame10/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColorFrame10/x"].v = UIControlTarget[L"RoundRect/BrushColor10/x"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame10/y"].v = UIControlTarget[L"RoundRect/BrushColor10/y"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame10/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame10/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame10/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame10/ellipsewidth"].v = 10;
								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor10/fill"].v, 255))
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame10/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame10/thickness"].v = 3;
								}
								else
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame10/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame10/thickness"].v = 1;
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorFrame10/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorFrame10/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorFrame10/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame10/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame10/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame10/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame10/thickness"].v = 1;
								UIControlColorTarget[L"RoundRect/BrushColorFrame10/frame"].v = RGBA(130, 130, 130, 0);
							}

							UIControl[L"Image/BrushColor10/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"Image/BrushColor10/x"].v = UIControlTarget[L"RoundRect/BrushColor10/x"].v + 10;
								UIControlTarget[L"Image/BrushColor10/y"].v = UIControlTarget[L"RoundRect/BrushColor10/y"].v + 10;

								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor10/fill"].v, 255)) UIControlTarget[L"Image/BrushColor10/transparency"].v = 255;
								else UIControlTarget[L"Image/BrushColor10/transparency"].v = 0;
							}
							else
							{
								UIControlTarget[L"Image/BrushColor10/x"].v = 96 + 8 + 20 + 10;
								UIControlTarget[L"Image/BrushColor10/y"].v = UIControl[L"RoundRect/RoundRect2/y"].v + 25 + 10;
								UIControlTarget[L"Image/BrushColor10/transparency"].v = 0;
							}
						}
						{
							UIControl[L"RoundRect/BrushColor11/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColor11/x"].v = 11 + 44 * 5;
								UIControlTarget[L"RoundRect/BrushColor11/y"].v = (floating_windows.height - 257 + 6);
								UIControlTarget[L"RoundRect/BrushColor11/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor11/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor11/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor11/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor11/transparency"].v = 255;
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColor11/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColor11/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColor11/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor11/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor11/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor11/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColor11/transparency"].v = 0;
							}

							UIControl[L"RoundRect/BrushColorFrame11/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColorFrame11/x"].v = UIControlTarget[L"RoundRect/BrushColor11/x"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame11/y"].v = UIControlTarget[L"RoundRect/BrushColor11/y"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame11/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame11/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame11/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame11/ellipsewidth"].v = 10;
								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor11/fill"].v, 255))
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame11/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame11/thickness"].v = 3;
								}
								else
								{
									UIControlColorTarget[L"RoundRect/BrushColorFrame11/frame"].v = RGBA(130, 130, 130, 255);
									UIControlTarget[L"RoundRect/BrushColorFrame11/thickness"].v = 1;
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorFrame11/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorFrame11/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorFrame11/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame11/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame11/ellipseheight"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame11/ellipsewidth"].v = 10;
								UIControlTarget[L"RoundRect/BrushColorFrame11/thickness"].v = 1;
								UIControlColorTarget[L"RoundRect/BrushColorFrame11/frame"].v = RGBA(130, 130, 130, 0);
							}

							UIControl[L"Image/BrushColor11/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"Image/BrushColor11/x"].v = UIControlTarget[L"RoundRect/BrushColor11/x"].v + 10;
								UIControlTarget[L"Image/BrushColor11/y"].v = UIControlTarget[L"RoundRect/BrushColor11/y"].v + 10;

								if (SET_ALPHA(brush.color, 255) == SET_ALPHA((int)UIControlColor[L"RoundRect/BrushColor11/fill"].v, 255)) UIControlTarget[L"Image/BrushColor11/transparency"].v = 255;
								else UIControlTarget[L"Image/BrushColor11/transparency"].v = 0;
							}
							else
							{
								UIControlTarget[L"Image/BrushColor11/x"].v = 96 + 8 + 20 + 10;
								UIControlTarget[L"Image/BrushColor11/y"].v = UIControl[L"RoundRect/RoundRect2/y"].v + 25 + 10;
								UIControlTarget[L"Image/BrushColor11/transparency"].v = 0;
							}
						}
						{
							UIControl[L"RoundRect/BrushColor12/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColor12/x"].v = 11 + 44 * 5;
								UIControlTarget[L"RoundRect/BrushColor12/y"].v = (floating_windows.height - 257 + 6) + 44;
								UIControlTarget[L"RoundRect/BrushColor12/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor12/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor12/transparency"].v = 255;
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColor12/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColor12/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColor12/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor12/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColor12/transparency"].v = 0;
							}
							UIControlTarget[L"RoundRect/BrushColor12/angle"].v = int(UIControlTarget[L"RoundRect/BrushColor12/angle"].v + 1) % 360;

							UIControl[L"RoundRect/BrushColorFrame12/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								UIControlTarget[L"RoundRect/BrushColorFrame12/x"].v = UIControlTarget[L"RoundRect/BrushColor12/x"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame12/y"].v = UIControlTarget[L"RoundRect/BrushColor12/y"].v;
								UIControlTarget[L"RoundRect/BrushColorFrame12/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame12/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame12/ellipseheight"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame12/ellipsewidth"].v = 40;

								if (BrushColorChoose.x && BrushColorChoose.y) UIControlTarget[L"RoundRect/BrushColorFrame12/thickness"].v = 3;
								else UIControlTarget[L"RoundRect/BrushColorFrame12/thickness"].v = 1;

								UIControlColorTarget[L"RoundRect/BrushColorFrame12/frame"].v = RGBA(130, 130, 130, 255);
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorFrame12/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorFrame12/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorFrame12/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame12/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame12/ellipseheight"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame12/ellipsewidth"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorFrame12/thickness"].v = 1;
								UIControlColorTarget[L"RoundRect/BrushColorFrame12/frame"].v = RGBA(130, 130, 130, 0);
							}
						}
					}
					{
						UIControl[L"RoundRect/PaintThickness/x"].s = 3;

						if (state == 1.1 || state == 1.11 || state == 1.12)
						{
							UIControlTarget[L"RoundRect/PaintThickness/x"].v = UIControlTarget[L"RoundRect/BrushTop/x"].v + 360;
							UIControlTarget[L"RoundRect/PaintThickness/y"].v = floating_windows.height - 257 + 6;
							UIControlTarget[L"RoundRect/PaintThickness/width"].v = 103;
							UIControlTarget[L"RoundRect/PaintThickness/height"].v = 84;
							UIControlTarget[L"RoundRect/PaintThickness/ellipseheight"].v = 25;
							UIControlTarget[L"RoundRect/PaintThickness/ellipsewidth"].v = 25;

							if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/PaintThickness/fill"].v = RGBA(230, 230, 230, 255);
							else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/PaintThickness/fill"].v = RGBA(70, 70, 70, 255);
						}
						else
						{
							UIControlTarget[L"RoundRect/PaintThickness/x"].v = 96 + 8 + 10;
							UIControlTarget[L"RoundRect/PaintThickness/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 10;
							UIControlTarget[L"RoundRect/PaintThickness/width"].v = UIControl[L"RoundRect/RoundRect2/width"].v - 20;
							UIControlTarget[L"RoundRect/PaintThickness/height"].v = 70;
							UIControlTarget[L"RoundRect/PaintThickness/ellipseheight"].v = 25;
							UIControlTarget[L"RoundRect/PaintThickness/ellipsewidth"].v = 25;

							if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/PaintThickness/fill"].v = RGBA(230, 230, 230, 0);
							else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/PaintThickness/fill"].v = RGBA(70, 70, 70, 0);
						}

						UIControl[L"RoundRect/PaintThicknessPrompt/x"].s = 3;
						if (state == 1.1 || state == 1.11 || state == 1.12)
						{
							if (brush.width < 40)
							{
								UIControlTarget[L"RoundRect/PaintThicknessPrompt/x"].v = UIControlTarget[L"RoundRect/PaintThickness/x"].v + 32 + 35 - (brush.width + 1) / 2 - brush.width / 8 - 5;
								UIControlTarget[L"RoundRect/PaintThicknessPrompt/y"].v = UIControlTarget[L"RoundRect/PaintThickness/y"].v + 42 - (brush.width + 1) / 2 - brush.width / 5 - 5;
							}
							else
							{
								if (brush.width > 60)
								{
									UIControlTarget[L"RoundRect/PaintThicknessPrompt/x"].v = UIControlTarget[L"RoundRect/PaintThickness/x"].v + 32 + 35 - 30;
									UIControlTarget[L"RoundRect/PaintThicknessPrompt/y"].v = UIControlTarget[L"RoundRect/PaintThickness/y"].v + 42 - 30;
								}
								else
								{
									UIControlTarget[L"RoundRect/PaintThicknessPrompt/x"].v = UIControlTarget[L"RoundRect/PaintThickness/x"].v + 32 + 35 - (brush.width + 1) / 2;
									UIControlTarget[L"RoundRect/PaintThicknessPrompt/y"].v = UIControlTarget[L"RoundRect/PaintThickness/y"].v + 42 - (brush.width + 1) / 2;
								}
							}
							UIControlTarget[L"RoundRect/PaintThicknessPrompt/width"].v = (brush.width + 1);
							UIControlTarget[L"RoundRect/PaintThicknessPrompt/height"].v = (brush.width + 1);
							UIControlTarget[L"RoundRect/PaintThicknessPrompt/ellipseheight"].v = (brush.width + 1);
							UIControlTarget[L"RoundRect/PaintThicknessPrompt/ellipsewidth"].v = (brush.width + 1);

							UIControlColorTarget[L"RoundRect/PaintThicknessPrompt/fill"].v = SET_ALPHA(brush.color, 255);
						}
						else
						{
							UIControlTarget[L"RoundRect/PaintThicknessPrompt/x"].v = 96 + 8 + (48 - min(45, brush.width / 2)) + 6;
							UIControlTarget[L"RoundRect/PaintThicknessPrompt/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 10 + 35 - min(45, brush.width / 2) + 6;
							UIControlTarget[L"RoundRect/PaintThicknessPrompt/width"].v = min(90, brush.width);
							UIControlTarget[L"RoundRect/PaintThicknessPrompt/height"].v = min(90, brush.width);
							UIControlTarget[L"RoundRect/PaintThicknessPrompt/ellipseheight"].v = min(90, brush.width);
							UIControlTarget[L"RoundRect/PaintThicknessPrompt/ellipsewidth"].v = min(90, brush.width);

							UIControlColorTarget[L"RoundRect/PaintThicknessPrompt/fill"].v = SET_ALPHA(brush.color, 0);
						}

						{
							UIControl[L"RoundRect/PaintThicknessAdjust/x"].s = 3;

							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								if (state == 1.11)
								{
									UIControlTarget[L"RoundRect/PaintThicknessAdjust/x"].v = 1;
									UIControlTarget[L"RoundRect/PaintThicknessAdjust/y"].v = floating_windows.height - 312;
									UIControlTarget[L"RoundRect/PaintThicknessAdjust/width"].v = floating_windows.width - 106;
									UIControlTarget[L"RoundRect/PaintThicknessAdjust/height"].v = 50;
									UIControlTarget[L"RoundRect/PaintThicknessAdjust/ellipseheight"].v = 25;
									UIControlTarget[L"RoundRect/PaintThicknessAdjust/ellipsewidth"].v = 25;

									if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/PaintThicknessAdjust/fill"].v = RGBA(255, 255, 255, 255);
									else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/PaintThicknessAdjust/fill"].v = RGBA(30, 33, 41, 255);
									UIControlColorTarget[L"RoundRect/PaintThicknessAdjust/frame"].v = RGBA(150, 150, 150, 255);
								}
								else
								{
									UIControlTarget[L"RoundRect/PaintThicknessAdjust/x"].v = UIControlTarget[L"RoundRect/BrushTop/x"].v + 360;
									UIControlTarget[L"RoundRect/PaintThicknessAdjust/y"].v = floating_windows.height - 257 + 6;
									UIControlTarget[L"RoundRect/PaintThicknessAdjust/width"].v = 103;
									UIControlTarget[L"RoundRect/PaintThicknessAdjust/height"].v = 84;
									UIControlTarget[L"RoundRect/PaintThicknessAdjust/ellipseheight"].v = 25;
									UIControlTarget[L"RoundRect/PaintThicknessAdjust/ellipsewidth"].v = 25;

									if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/PaintThicknessAdjust/fill"].v = RGBA(255, 255, 255, 255);
									else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/PaintThicknessAdjust/fill"].v = RGBA(30, 33, 41, 255);
									UIControlColorTarget[L"RoundRect/PaintThicknessAdjust/frame"].v = RGBA(150, 150, 150, 255);
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/PaintThicknessAdjust/x"].v = 96 + 8 + 10;
								UIControlTarget[L"RoundRect/PaintThicknessAdjust/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 10;
								UIControlTarget[L"RoundRect/PaintThicknessAdjust/width"].v = UIControl[L"RoundRect/RoundRect2/width"].v - 20;
								UIControlTarget[L"RoundRect/PaintThicknessAdjust/height"].v = 70;
								UIControlTarget[L"RoundRect/PaintThicknessAdjust/ellipseheight"].v = 25;
								UIControlTarget[L"RoundRect/PaintThicknessAdjust/ellipsewidth"].v = 25;

								if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/PaintThicknessAdjust/fill"].v = RGBA(255, 255, 255, 0);
								else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/PaintThicknessAdjust/fill"].v = RGBA(30, 33, 41, 0);
								UIControlColorTarget[L"RoundRect/PaintThicknessAdjust/frame"].v = RGBA(150, 150, 150, 0);
							}

							{
								UIControl[L"RoundRect/PaintThicknessSchedule1/x"].s = 3;
								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									if (state == 1.11)
									{
										UIControlTarget[L"RoundRect/PaintThicknessSchedule1/x"].v = 20;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule1/y"].v = floating_windows.height - 312 + 22;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule1/width"].v = 330;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule1/height"].v = 6;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule1/ellipseheight"].v = 6;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule1/ellipsewidth"].v = 6;

										if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/PaintThicknessSchedule1/fill"].v = RGBA(230, 230, 230, 255);
										else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/PaintThicknessSchedule1/fill"].v = RGBA(70, 70, 70, 255);
									}
									else
									{
										UIControlTarget[L"RoundRect/PaintThicknessSchedule1/x"].v = UIControlTarget[L"RoundRect/BrushTop/x"].v + 360;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule1/y"].v = floating_windows.height - 257 + 6 + 39;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule1/width"].v = 103;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule1/height"].v = 6;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule1/ellipseheight"].v = 6;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule1/ellipsewidth"].v = 6;

										if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/PaintThicknessSchedule1/fill"].v = RGBA(230, 230, 230, 255);
										else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/PaintThicknessSchedule1/fill"].v = RGBA(70, 70, 70, 255);
									}
								}
								else
								{
									UIControlTarget[L"RoundRect/PaintThicknessSchedule1/x"].v = 96 + 8 + 10;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule1/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 42;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule1/width"].v = UIControl[L"RoundRect/RoundRect2/width"].v - 20;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule1/height"].v = 6;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule1/ellipseheight"].v = 6;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule1/ellipsewidth"].v = 6;

									if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/PaintThicknessSchedule1/fill"].v = RGBA(230, 230, 230, 0);
									else if (BackgroundColorMode == 1) UIControlColorTarget[L"RoundRect/PaintThicknessSchedule1/fill"].v = RGBA(70, 70, 70, 0);
								}

								UIControl[L"RoundRect/PaintThicknessSchedule2/x"].s = 3;
								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									if (state == 1.11)
									{
										UIControlTarget[L"RoundRect/PaintThicknessSchedule2/x"].v = 20;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule2/y"].v = floating_windows.height - 312 + 22;

										if (brush.width <= 50) UIControlTarget[L"RoundRect/PaintThicknessSchedule2/width"].v = 10 + 190.0 * double(brush.width - 1) / 49.0;
										else if (brush.width <= 100) UIControlTarget[L"RoundRect/PaintThicknessSchedule2/width"].v = 200.0 + 60.0 * double(brush.width - 51) / 49.0;
										else UIControlTarget[L"RoundRect/PaintThicknessSchedule2/width"].v = 260.0 + 60.0 * double(brush.width - 101) / 399.0;

										UIControlTarget[L"RoundRect/PaintThicknessSchedule2/height"].v = 6;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule2/ellipseheight"].v = 6;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule2/ellipsewidth"].v = 6;

										UIControlColorTarget[L"RoundRect/PaintThicknessSchedule2/fill"].v = SET_ALPHA(brush.color, 255);
									}
									else
									{
										UIControlTarget[L"RoundRect/PaintThicknessSchedule2/x"].v = UIControlTarget[L"RoundRect/BrushTop/x"].v + 360;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule2/y"].v = floating_windows.height - 257 + 6 + 39;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule2/width"].v = 103;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule2/height"].v = 6;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule2/ellipseheight"].v = 6;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule2/ellipsewidth"].v = 6;

										UIControlColorTarget[L"RoundRect/PaintThicknessSchedule2/fill"].v = SET_ALPHA(brush.color, 255);
									}
								}
								else
								{
									UIControlTarget[L"RoundRect/PaintThicknessSchedule2/x"].v = 96 + 8 + 10;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule2/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 42;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule2/width"].v = UIControl[L"RoundRect/RoundRect2/width"].v - 20;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule2/height"].v = 6;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule2/ellipseheight"].v = 6;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule2/ellipsewidth"].v = 6;

									UIControlColorTarget[L"RoundRect/PaintThicknessSchedule2/fill"].v = SET_ALPHA(brush.color, 0);
								}

								UIControl[L"RoundRect/PaintThicknessSchedule3/x"].s = 3;
								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									if (state == 1.11)
									{
										if (brush.width <= 50) UIControlTarget[L"RoundRect/PaintThicknessSchedule3/x"].v = 30 - UIControlTarget[L"RoundRect/PaintThicknessSchedule3/width"].v / 2 + 190.0 * double(brush.width - 1) / 49.0;
										else if (brush.width <= 100) UIControlTarget[L"RoundRect/PaintThicknessSchedule3/x"].v = 20 - UIControlTarget[L"RoundRect/PaintThicknessSchedule3/width"].v / 2 + 200.0 + 60.0 * double(brush.width - 51) / 49.0;
										else UIControlTarget[L"RoundRect/PaintThicknessSchedule3/x"].v = 20 - UIControlTarget[L"RoundRect/PaintThicknessSchedule3/width"].v / 2 + 260.0 + 60.0 * double(brush.width - 101) / 399.0;

										UIControlTarget[L"RoundRect/PaintThicknessSchedule3/y"].v = floating_windows.height - 312 + 25 - UIControlTarget[L"RoundRect/PaintThicknessSchedule3/width"].v / 2;

										UIControlColorTarget[L"RoundRect/PaintThicknessSchedule3/fill"].v = SET_ALPHA(brush.color, 255);
									}
									else
									{
										UIControlTarget[L"RoundRect/PaintThicknessSchedule3/x"].v = UIControlTarget[L"RoundRect/BrushTop/x"].v + 360 + 50 - UIControlTarget[L"RoundRect/PaintThicknessSchedule3/width"].v / 2;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule3/y"].v = floating_windows.height - 257 + 6 + 32 - UIControlTarget[L"RoundRect/PaintThicknessSchedule3/width"].v / 2;

										UIControlColorTarget[L"RoundRect/PaintThicknessSchedule3/fill"].v = SET_ALPHA(brush.color, 255);
									}
								}
								else
								{
									UIControlTarget[L"RoundRect/PaintThicknessSchedule3/x"].v = 96 + 8 + 48 - UIControlTarget[L"RoundRect/PaintThicknessSchedule3/width"].v / 2;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule3/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 35 - UIControlTarget[L"RoundRect/PaintThicknessSchedule3/width"].v / 2;

									UIControlColorTarget[L"RoundRect/PaintThicknessSchedule3/fill"].v = SET_ALPHA(brush.color, 0);
								}

								UIControl[L"RoundRect/PaintThicknessSchedule4a/x"].s = 3;
								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									if (state == 1.11)
									{
										if (brush.mode == 2)
										{
											UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/x"].v = 385 - UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v / 2;
											UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/y"].v = floating_windows.height - 312 + 25 - UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/height"].v / 2;

											UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v = 40;
											UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/height"].v = 40;
											if (UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v == UIControl[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v) UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v = 40;
										}
										else
										{
											UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/x"].v = 380 - UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v / 2;
											UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/y"].v = floating_windows.height - 312 + 25 - UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/height"].v / 2;

											UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v = 4;
											UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/height"].v = 4;
											if (UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v == UIControl[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v) UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v = 4;
										}

										UIControlColorTarget[L"RoundRect/PaintThicknessSchedule4a/fill"].v = SET_ALPHA(brush.color, 255);
									}
									else
									{
										UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/x"].v = UIControlTarget[L"RoundRect/BrushTop/x"].v + 360 + 50 - UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v / 2;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/y"].v = floating_windows.height - 257 + 6 + 32 - UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v / 2;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v = 20;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/height"].v = 20;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v = 20;

										UIControlColorTarget[L"RoundRect/PaintThicknessSchedule4a/fill"].v = SET_ALPHA(brush.color, 255);
									}
								}
								else
								{
									UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/x"].v = 96 + 8 + 48 - UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v / 2;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 35 - UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v / 2;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/width"].v = 20;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/height"].v = 20;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v = 20;

									UIControlColorTarget[L"RoundRect/PaintThicknessSchedule4a/fill"].v = SET_ALPHA(brush.color, 0);
								}

								UIControl[L"RoundRect/PaintThicknessSchedule5a/x"].s = 3;
								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									if (state == 1.11)
									{
										UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/x"].v = 410 - UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/width"].v / 2;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/y"].v = floating_windows.height - 312 + 25 - UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/width"].v / 2;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/width"].v = 10;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/height"].v = 10;
										if (UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/ellipse"].v == UIControl[L"RoundRect/PaintThicknessSchedule5a/ellipse"].v) UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/ellipse"].v = 10;

										if (brush.mode == 2) UIControlColorTarget[L"RoundRect/PaintThicknessSchedule5a/fill"].v = SET_ALPHA(brush.color, 0);
										else UIControlColorTarget[L"RoundRect/PaintThicknessSchedule5a/fill"].v = SET_ALPHA(brush.color, 255);
									}
									else
									{
										UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/x"].v = UIControlTarget[L"RoundRect/BrushTop/x"].v + 360 + 50 - UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/width"].v / 2;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/y"].v = floating_windows.height - 257 + 6 + 32 - UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/width"].v / 2;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/width"].v = 20;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/height"].v = 20;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/ellipse"].v = 20;

										UIControlColorTarget[L"RoundRect/PaintThicknessSchedule5a/fill"].v = SET_ALPHA(brush.color, 255);
									}
								}
								else
								{
									UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/x"].v = 96 + 8 + 48 - UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/width"].v / 2;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 35 - UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/width"].v / 2;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/width"].v = 20;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/height"].v = 20;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/ellipse"].v = 20;

									UIControlColorTarget[L"RoundRect/PaintThicknessSchedule5a/fill"].v = SET_ALPHA(brush.color, 0);
								}

								UIControl[L"RoundRect/PaintThicknessSchedule6a/x"].s = 3;
								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									if (state == 1.11)
									{
										UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/x"].v = 440 - UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/width"].v / 2;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/y"].v = floating_windows.height - 312 + 25 - UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/height"].v / 2;

										if (brush.mode == 2)
										{
											UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/width"].v = 50;
											UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/height"].v = 40;
											if (UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v == UIControl[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v) UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v = 40;

											UIControlColorTarget[L"RoundRect/PaintThicknessSchedule6a/fill"].v = SET_ALPHA(brush.color, 255);
										}
										else
										{
											UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/width"].v = 20;
											UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/height"].v = 20;
											if (UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v == UIControl[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v) UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v = 20;

											UIControlColorTarget[L"RoundRect/PaintThicknessSchedule6a/fill"].v = SET_ALPHA(brush.color, 255);
										}
									}
									else
									{
										UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/x"].v = UIControlTarget[L"RoundRect/BrushTop/x"].v + 360 + 50 - UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/width"].v / 2;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/y"].v = floating_windows.height - 257 + 6 + 32 - UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/width"].v / 2;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/width"].v = 20;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/height"].v = 20;
										UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v = 20;

										UIControlColorTarget[L"RoundRect/PaintThicknessSchedule6a/fill"].v = SET_ALPHA(brush.color, 255);
									}
								}
								else
								{
									UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/x"].v = 96 + 8 + 48 - UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/width"].v / 2;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + 35 - UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/width"].v / 2;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/width"].v = 20;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/height"].v = 20;
									UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v = 20;

									UIControlColorTarget[L"RoundRect/PaintThicknessSchedule6a/fill"].v = SET_ALPHA(brush.color, 0);
								}
							}
						}
					}
					{
						{
							UIControl[L"RoundRect/BrushColorChoose/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								if (state == 1.12)
								{
									UIControlTarget[L"RoundRect/BrushColorChoose/x"].v = 11 + 44 * 5 - 120;
									UIControlTarget[L"RoundRect/BrushColorChoose/y"].v = floating_windows.height - 382;
									UIControlTarget[L"RoundRect/BrushColorChoose/width"].v = 280;
									UIControlTarget[L"RoundRect/BrushColorChoose/height"].v = 120;
									UIControlTarget[L"RoundRect/BrushColorChoose/ellipseheight"].v = 25;
									UIControlTarget[L"RoundRect/BrushColorChoose/ellipsewidth"].v = 25;

									if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v = RGBA(255, 255, 255, 255);
									else UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v = RGBA(30, 33, 41, 255);
									UIControlColorTarget[L"RoundRect/BrushColorChoose/frame"].v = RGBA(130, 130, 130, 255);
								}
								else
								{
									UIControlTarget[L"RoundRect/BrushColorChoose/x"].v = 11 + 44 * 5;
									UIControlTarget[L"RoundRect/BrushColorChoose/y"].v = (floating_windows.height - 257 + 6) + 44;
									UIControlTarget[L"RoundRect/BrushColorChoose/width"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChoose/height"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChoose/ellipseheight"].v = 5;
									UIControlTarget[L"RoundRect/BrushColorChoose/ellipsewidth"].v = 5;

									if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v = RGBA(255, 255, 255, 0);
									else UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v = RGBA(30, 33, 41, 0);
									UIControlColorTarget[L"RoundRect/BrushColorChoose/frame"].v = RGBA(130, 130, 130, 0);
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorChoose/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorChoose/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorChoose/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorChoose/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorChoose/ellipseheight"].v = 5;
								UIControlTarget[L"RoundRect/BrushColorChoose/ellipsewidth"].v = 5;

								if (BackgroundColorMode == 0) UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v = RGBA(255, 255, 255, 0);
								else UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v = RGBA(30, 33, 41, 0);
								UIControlColorTarget[L"RoundRect/BrushColorChoose/frame"].v = RGBA(130, 130, 130, 0);
							}
						}
						{
							UIControl[L"RoundRect/BrushColorChooseWheel/x"].s = 3;
							if (state == 1.1 || state == 1.11 || state == 1.12)
							{
								if (state == 1.12)
								{
									UIControlTarget[L"RoundRect/BrushColorChooseWheel/x"].v = 11 + 44 * 5 - 110;
									UIControlTarget[L"RoundRect/BrushColorChooseWheel/y"].v = floating_windows.height - 372;
									UIControlTarget[L"RoundRect/BrushColorChooseWheel/width"].v = 100;
									UIControlTarget[L"RoundRect/BrushColorChooseWheel/height"].v = 100;
									UIControlTarget[L"RoundRect/BrushColorChooseWheel/transparency"].v = 255;
								}
								else
								{
									UIControlTarget[L"RoundRect/BrushColorChooseWheel/x"].v = 11 + 44 * 5;
									UIControlTarget[L"RoundRect/BrushColorChooseWheel/y"].v = (floating_windows.height - 257 + 6) + 44;
									UIControlTarget[L"RoundRect/BrushColorChooseWheel/width"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChooseWheel/height"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChooseWheel/transparency"].v = 0;
								}
							}
							else
							{
								UIControlTarget[L"RoundRect/BrushColorChooseWheel/x"].v = 96 + 8 + 20;
								UIControlTarget[L"RoundRect/BrushColorChooseWheel/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
								UIControlTarget[L"RoundRect/BrushColorChooseWheel/width"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorChooseWheel/height"].v = 40;
								UIControlTarget[L"RoundRect/BrushColorChooseWheel/transparency"].v = 0;
							}

							{
								UIControl[L"RoundRect/BrushColorChooseMark/x"].s = 3;
								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									if (state == 1.12 && BrushColorChoose.x && BrushColorChoose.y)
									{
										if (BrushColorChoose.x == BrushColorChoose.last_x && BrushColorChoose.y == BrushColorChoose.last_y)
										{
											UIControlTarget[L"RoundRect/BrushColorChooseMark/x"].v = BrushColorChoose.x + UIControlTarget[L"RoundRect/BrushColorChooseWheel/x"].v - 7;
											UIControlTarget[L"RoundRect/BrushColorChooseMark/y"].v = BrushColorChoose.y + UIControlTarget[L"RoundRect/BrushColorChooseWheel/y"].v - 7;
										}
										UIControlTarget[L"RoundRect/BrushColorChooseMark/width"].v = 15;
										UIControlTarget[L"RoundRect/BrushColorChooseMark/height"].v = 15;
										UIControlTarget[L"RoundRect/BrushColorChooseMark/ellipseheight"].v = 15;
										UIControlTarget[L"RoundRect/BrushColorChooseMark/ellipsewidth"].v = 15;

										UIControlColorTarget[L"RoundRect/BrushColorChooseMark/fill"].v = SET_ALPHA(brush.color, 255);
										UIControlColorTarget[L"RoundRect/BrushColorChooseMark/frame"].v = RGBA(130, 130, 130, 255);
									}
									else
									{
										UIControlTarget[L"RoundRect/BrushColorChooseMark/x"].v = 11 + 44 * 5;
										UIControlTarget[L"RoundRect/BrushColorChooseMark/y"].v = (floating_windows.height - 257 + 6) + 44;
										UIControlTarget[L"RoundRect/BrushColorChooseMark/width"].v = 15;
										UIControlTarget[L"RoundRect/BrushColorChooseMark/height"].v = 15;
										UIControlTarget[L"RoundRect/BrushColorChooseMark/ellipseheight"].v = 15;
										UIControlTarget[L"RoundRect/BrushColorChooseMark/ellipsewidth"].v = 15;

										UIControlColorTarget[L"RoundRect/BrushColorChooseMark/fill"].v = SET_ALPHA(brush.color, 0);
										UIControlColorTarget[L"RoundRect/BrushColorChooseMark/frame"].v = RGBA(130, 130, 130, 0);
									}
								}
								else
								{
									UIControlTarget[L"RoundRect/BrushColorChooseMark/x"].v = 96 + 8 + 20;
									UIControlTarget[L"RoundRect/BrushColorChooseMark/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
									UIControlTarget[L"RoundRect/BrushColorChooseMark/width"].v = 15;
									UIControlTarget[L"RoundRect/BrushColorChooseMark/height"].v = 15;
									UIControlTarget[L"RoundRect/BrushColorChooseMark/ellipseheight"].v = 15;
									UIControlTarget[L"RoundRect/BrushColorChooseMark/ellipsewidth"].v = 15;

									UIControlColorTarget[L"RoundRect/BrushColorChooseMark/fill"].v = SET_ALPHA(brush.color, 0);
									UIControlColorTarget[L"RoundRect/BrushColorChooseMark/frame"].v = RGBA(130, 130, 130, 0);
								}

								UIControl[L"RoundRect/BrushColorChooseMarkR/x"].s = 3;
								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									if (state == 1.12)
									{
										UIControlTarget[L"RoundRect/BrushColorChooseMarkR/x"].v = 11 + 44 * 5;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkR/y"].v = floating_windows.height - 372;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkR/width"].v = 45;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkR/height"].v = 35;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkR/ellipseheight"].v = 20;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkR/ellipsewidth"].v = 20;

										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkR/fill"].v = RGBA(255, 0, 0, GetRValue(brush.color));
										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkR/frame"].v = RGBA(255, 0, 0, 255);
										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkR/text"].v = GetRValue(brush.color) <= 127 ? RGBA(255, 0, 0, 255) : SET_ALPHA(UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v, 255);
									}
									else
									{
										UIControlTarget[L"RoundRect/BrushColorChooseMarkR/x"].v = 11 + 44 * 5;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkR/y"].v = (floating_windows.height - 257 + 6) + 44;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkR/width"].v = 40;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkR/height"].v = 40;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkR/ellipseheight"].v = 40;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkR/ellipsewidth"].v = 40;

										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkR/fill"].v = RGBA(255, 0, 0, 0);
										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkR/frame"].v = RGBA(255, 0, 0, 0);
										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkR/text"].v = GetRValue(brush.color) <= 127 ? RGBA(255, 0, 0, 0) : SET_ALPHA(UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v, 0);
									}
								}
								else
								{
									UIControlTarget[L"RoundRect/BrushColorChooseMarkR/x"].v = 96 + 8 + 20;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkR/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkR/width"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkR/height"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkR/ellipseheight"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkR/ellipsewidth"].v = 40;

									UIControlColorTarget[L"RoundRect/BrushColorChooseMarkR/fill"].v = RGBA(255, 0, 0, 0);
									UIControlColorTarget[L"RoundRect/BrushColorChooseMarkR/frame"].v = RGBA(255, 0, 0, 0);
									UIControlColorTarget[L"RoundRect/BrushColorChooseMarkR/text"].v = GetRValue(brush.color) <= 127 ? RGBA(255, 0, 0, 0) : SET_ALPHA(UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v, 0);
								}

								UIControl[L"RoundRect/BrushColorChooseMarkG/x"].s = 3;
								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									if (state == 1.12)
									{
										UIControlTarget[L"RoundRect/BrushColorChooseMarkG/x"].v = 11 + 44 * 5 + 52;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkG/y"].v = floating_windows.height - 372;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkG/width"].v = 45;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkG/height"].v = 35;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkG/ellipseheight"].v = 20;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkG/ellipsewidth"].v = 20;

										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkG/fill"].v = RGBA(0, 255, 0, GetGValue(brush.color));
										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkG/frame"].v = RGBA(0, 255, 0, 255);
										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkG/text"].v = GetGValue(brush.color) <= 127 ? RGBA(0, 255, 0, 255) : SET_ALPHA(UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v, 255);
									}
									else
									{
										UIControlTarget[L"RoundRect/BrushColorChooseMarkG/x"].v = 11 + 44 * 5;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkG/y"].v = (floating_windows.height - 257 + 6) + 44;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkG/width"].v = 40;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkG/height"].v = 40;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkG/ellipseheight"].v = 40;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkG/ellipsewidth"].v = 40;

										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkG/fill"].v = RGBA(0, 255, 0, 0);
										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkG/frame"].v = RGBA(0, 255, 0, 0);
										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkG/text"].v = GetGValue(brush.color) <= 127 ? RGBA(0, 255, 0, 0) : SET_ALPHA(UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v, 0);
									}
								}
								else
								{
									UIControlTarget[L"RoundRect/BrushColorChooseMarkG/x"].v = 96 + 8 + 20;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkG/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkG/width"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkG/height"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkG/ellipseheight"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkG/ellipsewidth"].v = 40;

									UIControlColorTarget[L"RoundRect/BrushColorChooseMarkG/fill"].v = RGBA(0, 255, 0, 0);
									UIControlColorTarget[L"RoundRect/BrushColorChooseMarkG/frame"].v = RGBA(0, 255, 0, 0);
									UIControlColorTarget[L"RoundRect/BrushColorChooseMarkG/text"].v = GetGValue(brush.color) <= 127 ? RGBA(0, 255, 0, 0) : SET_ALPHA(UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v, 0);
								}

								UIControl[L"RoundRect/BrushColorChooseMarkB/x"].s = 3;
								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									if (state == 1.12)
									{
										UIControlTarget[L"RoundRect/BrushColorChooseMarkB/x"].v = 11 + 44 * 5 + 52 * 2;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkB/y"].v = floating_windows.height - 372;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkB/width"].v = 45;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkB/height"].v = 35;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkB/ellipseheight"].v = 20;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkB/ellipsewidth"].v = 20;

										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkB/fill"].v = RGBA(0, 0, 255, GetBValue(brush.color));
										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkB/frame"].v = RGBA(0, 0, 255, 255);
										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkB/text"].v = GetBValue(brush.color) <= 127 ? RGBA(0, 0, 255, 255) : SET_ALPHA(UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v, 255);
									}
									else
									{
										UIControlTarget[L"RoundRect/BrushColorChooseMarkB/x"].v = 11 + 44 * 5;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkB/y"].v = (floating_windows.height - 257 + 6) + 44;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkB/width"].v = 40;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkB/height"].v = 40;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkB/ellipseheight"].v = 40;
										UIControlTarget[L"RoundRect/BrushColorChooseMarkB/ellipsewidth"].v = 40;

										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkB/fill"].v = RGBA(0, 0, 255, 0);
										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkB/frame"].v = RGBA(0, 0, 255, 0);
										UIControlColorTarget[L"RoundRect/BrushColorChooseMarkB/text"].v = GetBValue(brush.color) <= 127 ? RGBA(0, 0, 255, 0) : SET_ALPHA(UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v, 0);
									}
								}
								else
								{
									UIControlTarget[L"RoundRect/BrushColorChooseMarkB/x"].v = 96 + 8 + 20;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkB/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + 25;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkB/width"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkB/height"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkB/ellipseheight"].v = 40;
									UIControlTarget[L"RoundRect/BrushColorChooseMarkB/ellipsewidth"].v = 40;

									UIControlColorTarget[L"RoundRect/BrushColorChooseMarkB/fill"].v = RGBA(0, 0, 255, 0);
									UIControlColorTarget[L"RoundRect/BrushColorChooseMarkB/frame"].v = RGBA(0, 0, 255, 0);
									UIControlColorTarget[L"RoundRect/BrushColorChooseMarkB/text"].v = GetBValue(brush.color) <= 127 ? RGBA(0, 0, 255, 0) : SET_ALPHA(UIControlColorTarget[L"RoundRect/BrushColorChoose/fill"].v, 0);
								}
							}
						}
					}

					{
						UIControl[L"RoundRect/BrushBottom/x"].s = 3;

						if (state == 1.1 || state == 1.11 || state == 1.12)
						{
							UIControlTarget[L"RoundRect/BrushBottom/x"].v = 1;
							UIControlTarget[L"RoundRect/BrushBottom/y"].v = floating_windows.height - 55;
							UIControlTarget[L"RoundRect/BrushBottom/width"].v = floating_windows.width - 106;
							UIControlTarget[L"RoundRect/BrushBottom/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushBottom/ellipseheight"].v = 25;
							UIControlTarget[L"RoundRect/BrushBottom/ellipsewidth"].v = 25;

							if (BackgroundColorMode == 0)
							{
								UIControlColorTarget[L"RoundRect/BrushBottom/fill"].v = RGBA(255, 255, 255, 255);
								UIControlColorTarget[L"RoundRect/BrushBottom/frame"].v = RGBA(150, 150, 150, 255);
							}
							else if (BackgroundColorMode == 1)
							{
								UIControlColorTarget[L"RoundRect/BrushBottom/fill"].v = RGBA(30, 33, 41, 255);
								UIControlColorTarget[L"RoundRect/BrushBottom/frame"].v = RGBA(150, 150, 150, 255);
							}
						}
						else
						{
							UIControlTarget[L"RoundRect/BrushBottom/x"].v = 96 + 8;
							UIControlTarget[L"RoundRect/BrushBottom/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + UIControlTarget[L"RoundRect/RoundRect1/height"].v - 40;
							UIControlTarget[L"RoundRect/BrushBottom/width"].v = UIControl[L"RoundRect/RoundRect2/width"].v;
							UIControlTarget[L"RoundRect/BrushBottom/height"].v = 40;
							UIControlTarget[L"RoundRect/BrushBottom/ellipseheight"].v = 25;
							UIControlTarget[L"RoundRect/BrushBottom/ellipsewidth"].v = 25;

							if (BackgroundColorMode == 0)
							{
								UIControlColorTarget[L"RoundRect/BrushBottom/fill"].v = RGBA(255, 255, 255, 0);
								UIControlColorTarget[L"RoundRect/BrushBottom/frame"].v = RGBA(150, 150, 150, 0);
							}
							else if (BackgroundColorMode == 1)
							{
								UIControlColorTarget[L"RoundRect/BrushBottom/fill"].v = RGBA(30, 33, 41, 0);
								UIControlColorTarget[L"RoundRect/BrushBottom/frame"].v = RGBA(150, 150, 150, 0);
							}
						}
					}
					{
						UIControl[L"RoundRect/BrushChoose/x"].s = 3;

						if (state == 1.1 || state == 1.11 || state == 1.12)
						{
							if (brush.mode == 2) UIControlTarget[L"RoundRect/BrushChoose/x"].v = 95;
							else UIControlTarget[L"RoundRect/BrushChoose/x"].v = 5;

							UIControlTarget[L"RoundRect/BrushChoose/y"].v = floating_windows.height - 55 + 5;
							UIControlTarget[L"RoundRect/BrushChoose/width"].v = 90;
							UIControlTarget[L"RoundRect/BrushChoose/height"].v = 30;
							UIControlTarget[L"RoundRect/BrushChoose/ellipseheight"].v = 15;
							UIControlTarget[L"RoundRect/BrushChoose/ellipsewidth"].v = 15;

							UIControlColorTarget[L"RoundRect/BrushChoose/frame"].v = SET_ALPHA(brush.color, 255);
						}
						else
						{
							UIControlTarget[L"RoundRect/BrushChoose/x"].v = 96 + 8 + 5;
							UIControlTarget[L"RoundRect/BrushChoose/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + UIControlTarget[L"RoundRect/RoundRect1/height"].v - 35;
							UIControlTarget[L"RoundRect/BrushChoose/width"].v = UIControl[L"RoundRect/RoundRect2/width"].v - 10;
							UIControlTarget[L"RoundRect/BrushChoose/height"].v = 30;
							UIControlTarget[L"RoundRect/BrushChoose/ellipseheight"].v = 15;
							UIControlTarget[L"RoundRect/BrushChoose/ellipsewidth"].v = 15;

							UIControlColorTarget[L"RoundRect/BrushChoose/frame"].v = SET_ALPHA(brush.color, 0);
						}
					}
					{
						UIControl[L"RoundRect/BrushMode/x"].s = 3;

						if (state == 1.1 || state == 1.11 || state == 1.12)
						{
							if (brush.mode == 3) UIControlTarget[L"RoundRect/BrushMode/x"].v = 285;
							else if (brush.mode == 4) UIControlTarget[L"RoundRect/BrushMode/x"].v = 375;
							else UIControlTarget[L"RoundRect/BrushMode/x"].v = 195;

							UIControlTarget[L"RoundRect/BrushMode/y"].v = floating_windows.height - 55 + 5;
							UIControlTarget[L"RoundRect/BrushMode/width"].v = 90;
							UIControlTarget[L"RoundRect/BrushMode/height"].v = 30;
							UIControlTarget[L"RoundRect/BrushMode/ellipseheight"].v = 15;
							UIControlTarget[L"RoundRect/BrushMode/ellipsewidth"].v = 15;

							UIControlColorTarget[L"RoundRect/BrushMode/frame"].v = SET_ALPHA(brush.color, 255);
						}
						else
						{
							UIControlTarget[L"RoundRect/BrushMode/x"].v = 96 + 8 + 5;
							UIControlTarget[L"RoundRect/BrushMode/y"].v = UIControlTarget[L"RoundRect/RoundRect1/y"].v + UIControlTarget[L"RoundRect/RoundRect1/height"].v - 35;
							UIControlTarget[L"RoundRect/BrushMode/width"].v = UIControl[L"RoundRect/RoundRect2/width"].v - 10;
							UIControlTarget[L"RoundRect/BrushMode/height"].v = 30;
							UIControlTarget[L"RoundRect/BrushMode/ellipseheight"].v = 15;
							UIControlTarget[L"RoundRect/BrushMode/ellipsewidth"].v = 15;

							UIControlColorTarget[L"RoundRect/BrushMode/frame"].v = SET_ALPHA(brush.color, 0);
						}
					}
					{
						UIControl[L"RoundRect/BrushInterval/x"].s = 3;

						if (state == 1.1 || state == 1.11 || state == 1.12)
						{
							UIControlTarget[L"RoundRect/BrushInterval/x"].v = 189;
							UIControlTarget[L"RoundRect/BrushInterval/y"].v = floating_windows.height - 55 + 10;
							UIControlTarget[L"RoundRect/BrushInterval/width"].v = 3;
							UIControlTarget[L"RoundRect/BrushInterval/height"].v = 20;
							UIControlColorTarget[L"RoundRect/BrushInterval/fill"].v = RGBA(150, 150, 150, 255);
						}
						else
						{
							UIControlTarget[L"RoundRect/BrushInterval/x"].v = UIControlTarget[L"RoundRect/BrushChoose/x"].v + 34;
							UIControlTarget[L"RoundRect/BrushInterval/y"].v = UIControl[L"RoundRect/RoundRect1/y"].v + UIControl[L"RoundRect/RoundRect1/height"].v - 40 + 10;
							UIControlTarget[L"RoundRect/BrushInterval/width"].v = 3;
							UIControlTarget[L"RoundRect/BrushInterval/height"].v = 20;
							UIControlColorTarget[L"RoundRect/BrushInterval/fill"].v = RGBA(150, 150, 150, 0);
						}
					}
				}
				//图像
				{
					{
						UIControlTarget[L"Image/Sign1/transparency"].v = 255;
					}
					//选择
					{
						UIControlTarget[L"Image/choose/x"].v = 0 + 28;
						UIControlTarget[L"Image/choose/y"].v = floating_windows.height - 140;
						if (choose.select) UIControlColorTarget[L"Image/choose/fill"].v = RGBA(98, 175, 82, 255);
						else UIControlColorTarget[L"Image/choose/fill"].v = RGBA(130, 130, 130, 255);
					}
					//画笔
					{
						if (brush.width >= 100) UIControlTarget[L"Image/brush/x"].v = 96 + 23;
						else UIControlTarget[L"Image/brush/x"].v = 96 + 28;
						UIControlTarget[L"Image/brush/y"].v = floating_windows.height - 140;

						if (brush.select == true) UIControlColorTarget[L"Image/brush/fill"].v = SET_ALPHA(brush.color, 255);
						else UIControlColorTarget[L"Image/brush/fill"].v = RGBA(130, 130, 130, 255);

						//画笔底部栏
						{
							{
								UIControl[L"Image/PaintBrush/x"].s = 3;

								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									UIControlTarget[L"Image/PaintBrush/x"].v = 5 + 10;
									UIControlTarget[L"Image/PaintBrush/y"].v = floating_windows.height - 55 + 10;

									if (brush.mode == 1 || brush.mode == 3 || brush.mode == 4) UIControlColorTarget[L"Image/PaintBrush/words_color"].v = SET_ALPHA(brush.color, 255);
									else UIControlColorTarget[L"Image/PaintBrush/words_color"].v = RGBA(130, 130, 130, 255);
								}
								else
								{
									UIControlTarget[L"Image/PaintBrush/x"].v = UIControlTarget[L"RoundRect/BrushBottom/x"].v + 10;
									UIControlTarget[L"Image/PaintBrush/y"].v = UIControlTarget[L"RoundRect/BrushBottom/y"].v + 10;

									if (brush.mode == 1 || brush.mode == 3 || brush.mode == 4) UIControlColorTarget[L"Image/PaintBrush/words_color"].v = SET_ALPHA(brush.color, 0);
									else UIControlColorTarget[L"Image/PaintBrush/words_color"].v = RGBA(130, 130, 130, 0);
								}
							}
							{
								UIControl[L"Image/FluorescentBrush/x"].s = 3;

								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									UIControlTarget[L"Image/FluorescentBrush/x"].v = 95 + 5;
									UIControlTarget[L"Image/FluorescentBrush/y"].v = floating_windows.height - 55 + 10;

									if (brush.mode == 2) UIControlColorTarget[L"Image/FluorescentBrush/words_color"].v = SET_ALPHA(brush.color, 255);
									else UIControlColorTarget[L"Image/FluorescentBrush/words_color"].v = RGBA(130, 130, 130, 255);
								}
								else
								{
									UIControlTarget[L"Image/FluorescentBrush/x"].v = UIControlTarget[L"RoundRect/BrushBottom/x"].v + 10;
									UIControlTarget[L"Image/FluorescentBrush/y"].v = UIControlTarget[L"RoundRect/BrushBottom/y"].v + 10;

									if (brush.mode == 2) UIControlColorTarget[L"Image/FluorescentBrush/words_color"].v = SET_ALPHA(brush.color, 0);
									else UIControlColorTarget[L"Image/FluorescentBrush/words_color"].v = RGBA(130, 130, 130, 0);
								}
							}

							{
								UIControl[L"Image/WriteBrush/x"].s = 3;

								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									UIControlTarget[L"Image/WriteBrush/x"].v = 195 + 10;
									UIControlTarget[L"Image/WriteBrush/y"].v = floating_windows.height - 55 + 10;

									if (brush.mode == 1 || brush.mode == 2) UIControlColorTarget[L"Image/WriteBrush/words_color"].v = SET_ALPHA(brush.color, 255);
									else UIControlColorTarget[L"Image/WriteBrush/words_color"].v = RGBA(130, 130, 130, 255);
								}
								else
								{
									UIControlTarget[L"Image/WriteBrush/x"].v = UIControlTarget[L"RoundRect/BrushBottom/x"].v + 10;
									UIControlTarget[L"Image/WriteBrush/y"].v = UIControlTarget[L"RoundRect/BrushBottom/y"].v + 10;

									if (brush.mode == 1 || brush.mode == 2) UIControlColorTarget[L"Image/WriteBrush/words_color"].v = SET_ALPHA(brush.color, 0);
									else UIControlColorTarget[L"Image/WriteBrush/words_color"].v = RGBA(130, 130, 130, 0);
								}
							}
							{
								UIControl[L"Image/LineBrush/x"].s = 3;

								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									UIControlTarget[L"Image/LineBrush/x"].v = 285 + 10;
									UIControlTarget[L"Image/LineBrush/y"].v = floating_windows.height - 55 + 10;

									if (brush.mode == 3) UIControlColorTarget[L"Image/LineBrush/words_color"].v = SET_ALPHA(brush.color, 255);
									else UIControlColorTarget[L"Image/LineBrush/words_color"].v = RGBA(130, 130, 130, 255);
								}
								else
								{
									UIControlTarget[L"Image/LineBrush/x"].v = UIControlTarget[L"RoundRect/BrushBottom/x"].v + 10;
									UIControlTarget[L"Image/LineBrush/y"].v = UIControlTarget[L"RoundRect/BrushBottom/y"].v + 10;

									if (brush.mode == 3) UIControlColorTarget[L"Image/LineBrush/words_color"].v = SET_ALPHA(brush.color, 0);
									else UIControlColorTarget[L"Image/LineBrush/words_color"].v = RGBA(130, 130, 130, 0);
								}
							}
							{
								UIControl[L"Image/RectangleBrush/x"].s = 3;

								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									UIControlTarget[L"Image/RectangleBrush/x"].v = 375 + 10;
									UIControlTarget[L"Image/RectangleBrush/y"].v = floating_windows.height - 55 + 10;

									if (brush.mode == 4) UIControlColorTarget[L"Image/RectangleBrush/words_color"].v = SET_ALPHA(brush.color, 255);
									else UIControlColorTarget[L"Image/RectangleBrush/words_color"].v = RGBA(130, 130, 130, 255);
								}
								else
								{
									UIControlTarget[L"Image/RectangleBrush/x"].v = UIControlTarget[L"RoundRect/BrushBottom/x"].v + 10;
									UIControlTarget[L"Image/RectangleBrush/y"].v = UIControlTarget[L"RoundRect/BrushBottom/y"].v + 10;

									if (brush.mode == 4) UIControlColorTarget[L"Image/RectangleBrush/words_color"].v = SET_ALPHA(brush.color, 0);
									else UIControlColorTarget[L"Image/RectangleBrush/words_color"].v = RGBA(130, 130, 130, 0);
								}
							}
						}
					}
					//橡皮
					{
						UIControlTarget[L"Image/rubber/x"].v = 192 + 28;
						UIControlTarget[L"Image/rubber/y"].v = floating_windows.height - 140;
						if (rubber.select) UIControlColorTarget[L"Image/rubber/fill"].v = RGBA(98, 175, 82, 255);
						else UIControlColorTarget[L"Image/rubber/fill"].v = RGBA(130, 130, 130, 255);
					}
					//程序调测
					{
						UIControlTarget[L"Image/test/x"].v = 288 + 28;
						UIControlTarget[L"Image/test/y"].v = floating_windows.height - 140;
						if (test.select) UIControlColorTarget[L"Image/test/fill"].v = RGBA(98, 175, 82, 255);
						else UIControlColorTarget[L"Image/test/fill"].v = RGBA(130, 130, 130, 255);
					}
				}
				//文字
				{
					//选择
					{
						UIControlTarget[L"Words/choose/height"].v = 18;
						UIControlTarget[L"Words/choose/left"].v = 0 + 7;
						UIControlTarget[L"Words/choose/top"].v = floating_windows.height - 155 + 48;
						UIControlTarget[L"Words/choose/right"].v = 0 + 7 + 83;
						UIControlTarget[L"Words/choose/bottom"].v = floating_windows.height - 155 + 48 + 48;
						if (choose.select) UIControlColorTarget[L"Words/choose/words_color"].v = RGBA(98, 175, 82, 255);
						else UIControlColorTarget[L"Words/choose/words_color"].v = RGBA(130, 130, 130, 255);
					}
					//画笔
					{
						UIControlTarget[L"Words/brush/height"].v = 18;
						UIControlTarget[L"Words/brush/left"].v = 96 + 7;
						UIControlTarget[L"Words/brush/top"].v = floating_windows.height - 155 + 48;
						UIControlTarget[L"Words/brush/right"].v = 96 + 7 + 83;
						UIControlTarget[L"Words/brush/bottom"].v = floating_windows.height - 155 + 48 + 48;
						if (brush.select) UIControlColorTarget[L"Words/brush/words_color"].v = SET_ALPHA(brush.color, 255), RestoreSketchpad = true;
						else UIControlColorTarget[L"Words/brush/words_color"].v = RGBA(130, 130, 130, 255);

						{
							if (brush.width >= 100) UIControlTarget[L"Words/brushSize/left"].v = 96 + 7 + 40;
							else UIControlTarget[L"Words/brushSize/left"].v = 96 + 7 + 45;
							UIControlTarget[L"Words/brushSize/top"].v = floating_windows.height - 156 + 35;

							if (brush.select) UIControlColorTarget[L"Words/brushSize/words_color"].v = SET_ALPHA(brush.color, 255);
							else UIControlColorTarget[L"Words/brushSize/words_color"].v = SET_ALPHA(brush.color, 0);
						}

						//画笔顶部栏
						{
							//画笔粗细
							{
								UIControlTarget[L"Words/PaintThickness/left"].s = 3;

								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									UIControlTarget[L"Words/PaintThickness/size"].v = 20;
									UIControlTarget[L"Words/PaintThickness/left"].v = UIControlTarget[L"RoundRect/BrushTop/x"].v + 365;
									UIControlTarget[L"Words/PaintThickness/top"].v = UIControlTarget[L"RoundRect/BrushTop/y"].v;
									UIControlTarget[L"Words/PaintThickness/width"].v = 35;
									UIControlTarget[L"Words/PaintThickness/height"].v = 100;
									UIControlColorTarget[L"Words/PaintThickness/words_color"].v = SET_ALPHA(brush.color, 255);
								}
								else
								{
									UIControlTarget[L"Words/PaintThickness/size"].v = 20;
									UIControlTarget[L"Words/PaintThickness/left"].v = 96 + 8 + 15;
									UIControlTarget[L"Words/PaintThickness/top"].v = UIControlTarget[L"RoundRect/BrushTop/y"].v;
									UIControlTarget[L"Words/PaintThickness/width"].v = 50;
									UIControlTarget[L"Words/PaintThickness/height"].v = 100;
									UIControlColorTarget[L"Words/PaintThickness/words_color"].v = SET_ALPHA(brush.color, 255);
								}

								UIControlTarget[L"Words/PaintThicknessValue/left"].s = 3;

								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									UIControlTarget[L"Words/PaintThicknessValue/size"].v = 20;
									if (brush.width < 40)
									{
										UIControlTarget[L"Words/PaintThicknessValue/left"].v = UIControlTarget[L"RoundRect/BrushTop/x"].v + 390 + brush.width / 6 + 5;
										UIControlTarget[L"Words/PaintThicknessValue/top"].v = UIControlTarget[L"RoundRect/BrushTop/y"].v + brush.width / 3 + 10;

										UIControlColorTarget[L"Words/PaintThicknessValue/words_color"].v = SET_ALPHA(brush.color, 255);
									}
									else
									{
										UIControlTarget[L"Words/PaintThicknessValue/left"].v = UIControlTarget[L"RoundRect/BrushTop/x"].v + 390;
										UIControlTarget[L"Words/PaintThicknessValue/top"].v = UIControlTarget[L"RoundRect/BrushTop/y"].v;

										if (BackgroundColorMode == 0) UIControlColorTarget[L"Words/PaintThicknessValue/words_color"].v = RGBA(255, 255, 255, 255);
										if (BackgroundColorMode == 1) UIControlColorTarget[L"Words/PaintThicknessValue/words_color"].v = RGBA(30, 33, 41, 255);
									}
									UIControlTarget[L"Words/PaintThicknessValue/width"].v = 75;
									UIControlTarget[L"Words/PaintThicknessValue/height"].v = 100;
								}
								else
								{
									UIControlTarget[L"Words/PaintThicknessValue/size"].v = 20;
									UIControlTarget[L"Words/PaintThicknessValue/left"].v = 96 + 8 + 15;
									UIControlTarget[L"Words/PaintThicknessValue/top"].v = UIControlTarget[L"RoundRect/BrushTop/y"].v;
									UIControlTarget[L"Words/PaintThicknessValue/width"].v = 50;
									UIControlTarget[L"Words/PaintThicknessValue/height"].v = 100;
									UIControlColorTarget[L"Words/PaintThicknessValue/words_color"].v = SET_ALPHA(brush.color, 255);
								}
							}
						}

						//画笔底部栏
						{
							{
								UIControl[L"Words/PaintBrush/left"].s = 3;

								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									UIControlTarget[L"Words/PaintBrush/size"].v = 18;
									UIControlTarget[L"Words/PaintBrush/left"].v = 5 + 25;
									UIControlTarget[L"Words/PaintBrush/top"].v = floating_windows.height - 55 + 5;
									UIControlTarget[L"Words/PaintBrush/width"].v = 65;
									UIControlTarget[L"Words/PaintBrush/height"].v = 33;
									UIControlColorTarget[L"Words/PaintBrush/words_color"].v = SET_ALPHA(brush.color, 255);

									if (brush.mode == 1 || brush.mode == 3 || brush.mode == 4) UIControlColorTarget[L"Words/PaintBrush/words_color"].v = SET_ALPHA(brush.color, 255);
									else UIControlColorTarget[L"Words/PaintBrush/words_color"].v = RGBA(130, 130, 130, 255);
								}
								else
								{
									UIControlTarget[L"Words/PaintBrush/size"].v = 18;
									UIControlTarget[L"Words/PaintBrush/left"].v = UIControlTarget[L"RoundRect/BrushChoose/x"].v + 20;
									UIControlTarget[L"Words/PaintBrush/top"].v = UIControlTarget[L"RoundRect/BrushChoose/y"].v;
									UIControlTarget[L"Words/PaintBrush/width"].v = 60;
									UIControlTarget[L"Words/PaintBrush/height"].v = 33;
									UIControlColorTarget[L"Words/PaintBrush/words_color"].v = SET_ALPHA(brush.color, 255);

									if (brush.mode == 1 || brush.mode == 3 || brush.mode == 4) UIControlColorTarget[L"Words/PaintBrush/words_color"].v = SET_ALPHA(brush.color, 0);
									else UIControlColorTarget[L"Words/PaintBrush/words_color"].v = RGBA(130, 130, 130, 0);
								}
							}
							{
								UIControl[L"Words/FluorescentBrush/left"].s = 3;

								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									UIControlTarget[L"Words/FluorescentBrush/size"].v = 18;
									UIControlTarget[L"Words/FluorescentBrush/left"].v = 95 + 25;
									UIControlTarget[L"Words/FluorescentBrush/top"].v = floating_windows.height - 55 + 5;
									UIControlTarget[L"Words/FluorescentBrush/width"].v = 65;
									UIControlTarget[L"Words/FluorescentBrush/height"].v = 33;
									UIControlColorTarget[L"Words/FluorescentBrush/words_color"].v = SET_ALPHA(brush.color, 255);

									if (brush.mode == 2) UIControlColorTarget[L"Words/FluorescentBrush/words_color"].v = SET_ALPHA(brush.color, 255);
									else UIControlColorTarget[L"Words/FluorescentBrush/words_color"].v = RGBA(130, 130, 130, 255);
								}
								else
								{
									UIControlTarget[L"Words/FluorescentBrush/size"].v = 18;
									UIControlTarget[L"Words/FluorescentBrush/left"].v = UIControlTarget[L"RoundRect/BrushChoose/x"].v + 20;
									UIControlTarget[L"Words/FluorescentBrush/top"].v = UIControlTarget[L"RoundRect/BrushChoose/y"].v;
									UIControlTarget[L"Words/FluorescentBrush/width"].v = 65;
									UIControlTarget[L"Words/FluorescentBrush/height"].v = 33;
									UIControlColorTarget[L"Words/FluorescentBrush/words_color"].v = SET_ALPHA(brush.color, 255);

									if (brush.mode == 2) UIControlColorTarget[L"Words/FluorescentBrush/words_color"].v = SET_ALPHA(brush.color, 0);
									else UIControlColorTarget[L"Words/FluorescentBrush/words_color"].v = RGBA(130, 130, 130, 0);
								}
							}

							{
								UIControl[L"Words/WriteBrush/left"].s = 3;

								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									UIControlTarget[L"Words/WriteBrush/size"].v = 18;
									UIControlTarget[L"Words/WriteBrush/left"].v = 195 + 25;
									UIControlTarget[L"Words/WriteBrush/top"].v = floating_windows.height - 55 + 5;
									UIControlTarget[L"Words/WriteBrush/width"].v = 65;
									UIControlTarget[L"Words/WriteBrush/height"].v = 33;
									UIControlColorTarget[L"Words/WriteBrush/words_color"].v = SET_ALPHA(brush.color, 255);

									if (brush.mode == 1 || brush.mode == 2) UIControlColorTarget[L"Words/WriteBrush/words_color"].v = SET_ALPHA(brush.color, 255);
									else UIControlColorTarget[L"Words/WriteBrush/words_color"].v = RGBA(130, 130, 130, 255);
								}
								else
								{
									UIControlTarget[L"Words/WriteBrush/size"].v = 18;
									UIControlTarget[L"Words/WriteBrush/left"].v = UIControlTarget[L"RoundRect/BrushMode/x"].v + 20;
									UIControlTarget[L"Words/WriteBrush/top"].v = UIControlTarget[L"RoundRect/BrushMode/y"].v;
									UIControlTarget[L"Words/WriteBrush/width"].v = 60;
									UIControlTarget[L"Words/WriteBrush/height"].v = 33;
									UIControlColorTarget[L"Words/WriteBrush/words_color"].v = SET_ALPHA(brush.color, 255);

									if (brush.mode == 1 || brush.mode == 2) UIControlColorTarget[L"Words/WriteBrush/words_color"].v = SET_ALPHA(brush.color, 0);
									else UIControlColorTarget[L"Words/WriteBrush/words_color"].v = RGBA(130, 130, 130, 0);
								}
							}
							{
								UIControl[L"Words/LineBrush/left"].s = 3;

								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									UIControlTarget[L"Words/LineBrush/size"].v = 18;
									UIControlTarget[L"Words/LineBrush/left"].v = 285 + 25;
									UIControlTarget[L"Words/LineBrush/top"].v = floating_windows.height - 55 + 5;
									UIControlTarget[L"Words/LineBrush/width"].v = 65;
									UIControlTarget[L"Words/LineBrush/height"].v = 33;
									UIControlColorTarget[L"Words/LineBrush/words_color"].v = SET_ALPHA(brush.color, 255);

									if (brush.mode == 3) UIControlColorTarget[L"Words/LineBrush/words_color"].v = SET_ALPHA(brush.color, 255);
									else UIControlColorTarget[L"Words/LineBrush/words_color"].v = RGBA(130, 130, 130, 255);
								}
								else
								{
									UIControlTarget[L"Words/LineBrush/size"].v = 18;
									UIControlTarget[L"Words/LineBrush/left"].v = UIControlTarget[L"RoundRect/BrushMode/x"].v + 20;
									UIControlTarget[L"Words/LineBrush/top"].v = UIControlTarget[L"RoundRect/BrushMode/y"].v;
									UIControlTarget[L"Words/LineBrush/width"].v = 60;
									UIControlTarget[L"Words/LineBrush/height"].v = 33;
									UIControlColorTarget[L"Words/LineBrush/words_color"].v = SET_ALPHA(brush.color, 255);

									if (brush.mode == 3) UIControlColorTarget[L"Words/LineBrush/words_color"].v = SET_ALPHA(brush.color, 0);
									else UIControlColorTarget[L"Words/LineBrush/words_color"].v = RGBA(130, 130, 130, 0);
								}
							}
							{
								UIControl[L"Words/RectangleBrush/left"].s = 3;

								if (state == 1.1 || state == 1.11 || state == 1.12)
								{
									UIControlTarget[L"Words/RectangleBrush/size"].v = 18;
									UIControlTarget[L"Words/RectangleBrush/left"].v = 375 + 25;
									UIControlTarget[L"Words/RectangleBrush/top"].v = floating_windows.height - 55 + 5;
									UIControlTarget[L"Words/RectangleBrush/width"].v = 65;
									UIControlTarget[L"Words/RectangleBrush/height"].v = 33;
									UIControlColorTarget[L"Words/RectangleBrush/words_color"].v = SET_ALPHA(brush.color, 255);

									if (brush.mode == 4) UIControlColorTarget[L"Words/RectangleBrush/words_color"].v = SET_ALPHA(brush.color, 255);
									else UIControlColorTarget[L"Words/RectangleBrush/words_color"].v = RGBA(130, 130, 130, 255);
								}
								else
								{
									UIControlTarget[L"Words/RectangleBrush/size"].v = 18;
									UIControlTarget[L"Words/RectangleBrush/left"].v = UIControlTarget[L"RoundRect/BrushMode/x"].v + 20;
									UIControlTarget[L"Words/RectangleBrush/top"].v = UIControlTarget[L"RoundRect/BrushMode/y"].v;
									UIControlTarget[L"Words/RectangleBrush/width"].v = 60;
									UIControlTarget[L"Words/RectangleBrush/height"].v = 33;
									UIControlColorTarget[L"Words/RectangleBrush/words_color"].v = SET_ALPHA(brush.color, 255);

									if (brush.mode == 4) UIControlColorTarget[L"Words/RectangleBrush/words_color"].v = SET_ALPHA(brush.color, 0);
									else UIControlColorTarget[L"Words/RectangleBrush/words_color"].v = RGBA(130, 130, 130, 0);
								}
							}
						}
					}
					//橡皮
					{
						UIControlTarget[L"Words/rubber/height"].v = 18;
						UIControlTarget[L"Words/rubber/left"].v = 192 + 7;
						UIControlTarget[L"Words/rubber/top"].v = floating_windows.height - 155 + 48;
						UIControlTarget[L"Words/rubber/right"].v = 192 + 7 + 83;
						UIControlTarget[L"Words/rubber/bottom"].v = floating_windows.height - 155 + 48 + 48;
						if (rubber.select) UIControlColorTarget[L"Words/rubber/words_color"].v = RGBA(98, 175, 82, 255);
						else UIControlColorTarget[L"Words/rubber/words_color"].v = RGBA(130, 130, 130, 255);
					}
					//程序调测
					{
						UIControlTarget[L"Words/test/height"].v = 18;
						UIControlTarget[L"Words/test/left"].v = 288 + 7;
						UIControlTarget[L"Words/test/top"].v = floating_windows.height - 155 + 48;
						UIControlTarget[L"Words/test/right"].v = 288 + 7 + 83;
						UIControlTarget[L"Words/test/bottom"].v = floating_windows.height - 155 + 48 + 48;
						if (test.select) UIControlColorTarget[L"Words/test/words_color"].v = RGBA(98, 175, 82, 255);
						else UIControlColorTarget[L"Words/test/words_color"].v = RGBA(130, 130, 130, 255);
					}
				}
			}

			for (const auto& [key, value] : UIControl)
			{
				if (UIControl[key].s && UIControl[key].v != UIControlTarget[key].v)
				{
					if (abs(UIControl[key].v - UIControlTarget[key].v) <= UIControl[key].e) UIControl[key].v = UIControlTarget[key].v;
					else if (UIControl[key].v < UIControlTarget[key].v) UIControl[key].v = UIControl[key].v + max(UIControl[key].e, (UIControlTarget[key].v - UIControl[key].v) / UIControl[key].s);
					else UIControl[key].v = UIControl[key].v + min(-UIControl[key].e, (UIControlTarget[key].v - UIControl[key].v) / UIControl[key].s);
				}
			}
			for (const auto& [key, value] : UIControlColor)
			{
				if (UIControlColor[key].s && UIControlColor[key].v != UIControlColorTarget[key].v)
				{
					//TODO
					float r1 = GetRValue(UIControlColor[key].v);
					float g1 = GetGValue(UIControlColor[key].v);
					float b1 = GetBValue(UIControlColor[key].v);
					float a1 = (UIControlColor[key].v >> 24) & 0xff;

					float r2 = GetRValue(UIControlColorTarget[key].v);
					float g2 = GetGValue(UIControlColorTarget[key].v);
					float b2 = GetBValue(UIControlColorTarget[key].v);
					float a2 = (UIControlColorTarget[key].v >> 24) & 0xff;

					if (abs(r1 - r2) <= UIControlColor[key].e) r1 = r2;
					else if (r1 < r2) r1 = r1 + max(UIControlColor[key].e, (r2 - r1) / UIControlColor[key].s);
					else if (r1 > r2) r1 = r1 + min(-UIControlColor[key].e, (r2 - r1) / UIControlColor[key].s);

					if (abs(g1 - g2) <= UIControlColor[key].e) g1 = g2;
					else if (g1 < g2) g1 = g1 + max(UIControlColor[key].e, (g2 - g1) / UIControlColor[key].s);
					else if (g1 > g2) g1 = g1 + min(-UIControlColor[key].e, (g2 - g1) / UIControlColor[key].s);

					if (abs(b1 - b2) <= UIControlColor[key].e) b1 = b2;
					else if (b1 < b2) b1 = b1 + max(UIControlColor[key].e, (b2 - b1) / UIControlColor[key].s);
					else if (b1 > b2) b1 = b1 + min(-UIControlColor[key].e, (b2 - b1) / UIControlColor[key].s);

					if (abs(a1 - a2) <= UIControlColor[key].e) a1 = a2;
					else if (a1 < a2) a1 = a1 + max(UIControlColor[key].e, (a2 - a1) / UIControlColor[key].s);
					else if (a1 > a2) a1 = a1 + min(-UIControlColor[key].e, (a2 - a1) / UIControlColor[key].s);

					UIControlColor[key].v = RGBA(max(0, min(255, (int)r1)), max(0, min(255, (int)g1)), max(0, min(255, (int)b1)), max(0, min(255, (int)a1)));
				}
			}
		}

		if ((int)state == target_status)
		{
			{
				{
					//画笔粗细调整
					hiex::EasyX_Gdiplus_FillRoundRect(UIControl[L"RoundRect/PaintThicknessAdjust/x"].v, UIControl[L"RoundRect/PaintThicknessAdjust/y"].v, UIControl[L"RoundRect/PaintThicknessAdjust/width"].v, UIControl[L"RoundRect/PaintThicknessAdjust/height"].v, UIControl[L"RoundRect/PaintThicknessAdjust/ellipsewidth"].v, UIControl[L"RoundRect/PaintThicknessAdjust/ellipseheight"].v, UIControlColor[L"RoundRect/PaintThicknessAdjust/frame"].v, UIControlColor[L"RoundRect/PaintThicknessAdjust/fill"].v, 2, true, SmoothingModeHighQuality, &background);

					hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/PaintThicknessSchedule1/x"].v, UIControl[L"RoundRect/PaintThicknessSchedule1/y"].v, UIControl[L"RoundRect/PaintThicknessSchedule1/width"].v, UIControl[L"RoundRect/PaintThicknessSchedule1/height"].v, UIControl[L"RoundRect/PaintThicknessSchedule1/ellipsewidth"].v, UIControl[L"RoundRect/PaintThicknessSchedule1/ellipseheight"].v, UIControlColor[L"RoundRect/PaintThicknessSchedule1/fill"].v, true, SmoothingModeHighQuality, &background);
					hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/PaintThicknessSchedule2/x"].v, UIControl[L"RoundRect/PaintThicknessSchedule2/y"].v, UIControl[L"RoundRect/PaintThicknessSchedule2/width"].v, UIControl[L"RoundRect/PaintThicknessSchedule2/height"].v, UIControl[L"RoundRect/PaintThicknessSchedule2/ellipsewidth"].v, UIControl[L"RoundRect/PaintThicknessSchedule2/ellipseheight"].v, UIControlColor[L"RoundRect/PaintThicknessSchedule2/fill"].v, true, SmoothingModeHighQuality, &background);
					hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/PaintThicknessSchedule3/x"].v, UIControl[L"RoundRect/PaintThicknessSchedule3/y"].v, UIControl[L"RoundRect/PaintThicknessSchedule3/width"].v, UIControl[L"RoundRect/PaintThicknessSchedule3/height"].v, UIControl[L"RoundRect/PaintThicknessSchedule3/ellipsewidth"].v, UIControl[L"RoundRect/PaintThicknessSchedule3/ellipseheight"].v, UIControlColor[L"RoundRect/PaintThicknessSchedule3/fill"].v, true, SmoothingModeHighQuality, &background);

					hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/PaintThicknessSchedule4a/x"].v, UIControl[L"RoundRect/PaintThicknessSchedule4a/y"].v, UIControl[L"RoundRect/PaintThicknessSchedule4a/width"].v, UIControl[L"RoundRect/PaintThicknessSchedule4a/height"].v, UIControl[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v, UIControl[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v, UIControlColor[L"RoundRect/PaintThicknessSchedule4a/fill"].v, true, SmoothingModeHighQuality, &background);
					hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/PaintThicknessSchedule5a/x"].v, UIControl[L"RoundRect/PaintThicknessSchedule5a/y"].v, UIControl[L"RoundRect/PaintThicknessSchedule5a/width"].v, UIControl[L"RoundRect/PaintThicknessSchedule5a/height"].v, UIControl[L"RoundRect/PaintThicknessSchedule5a/ellipse"].v, UIControl[L"RoundRect/PaintThicknessSchedule5a/ellipse"].v, UIControlColor[L"RoundRect/PaintThicknessSchedule5a/fill"].v, true, SmoothingModeHighQuality, &background);
					hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/PaintThicknessSchedule6a/x"].v, UIControl[L"RoundRect/PaintThicknessSchedule6a/y"].v, UIControl[L"RoundRect/PaintThicknessSchedule6a/width"].v, UIControl[L"RoundRect/PaintThicknessSchedule6a/height"].v, UIControl[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v, UIControl[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v, UIControlColor[L"RoundRect/PaintThicknessSchedule6a/fill"].v, true, SmoothingModeHighQuality, &background);

					//画笔颜色选择
					hiex::EasyX_Gdiplus_FillRoundRect(UIControl[L"RoundRect/BrushColorChoose/x"].v, UIControl[L"RoundRect/BrushColorChoose/y"].v, UIControl[L"RoundRect/BrushColorChoose/width"].v, UIControl[L"RoundRect/BrushColorChoose/height"].v, UIControl[L"RoundRect/BrushColorChoose/ellipsewidth"].v, UIControl[L"RoundRect/BrushColorChoose/ellipseheight"].v, UIControlColor[L"RoundRect/BrushColorChoose/frame"].v, UIControlColor[L"RoundRect/BrushColorChoose/fill"].v, 1, true, SmoothingModeHighQuality, &background);
					if (UIControl[L"RoundRect/BrushColorChooseWheel/transparency"].v != 0)
					{
						std::unique_lock<std::shared_mutex> lock(ColorPaletteSm);
						ColorPaletteImg = DrawHSVWheel(UIControl[L"RoundRect/BrushColorChooseWheel/width"].v, 40);
						lock.unlock();

						hiex::TransparentImage(&background, UIControl[L"RoundRect/BrushColorChooseWheel/x"].v, UIControl[L"RoundRect/BrushColorChooseWheel/y"].v, &ColorPaletteImg, UIControl[L"RoundRect/BrushColorChooseWheel/transparency"].v);

						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorChooseWheel/x"].v, UIControl[L"RoundRect/BrushColorChooseWheel/y"].v, UIControl[L"RoundRect/BrushColorChooseWheel/width"].v, UIControl[L"RoundRect/BrushColorChooseWheel/height"].v, UIControl[L"RoundRect/BrushColorChooseWheel/width"].v, UIControl[L"RoundRect/BrushColorChooseWheel/height"].v, RGBA(130, 130, 130, (int)UIControl[L"RoundRect/BrushColorChooseWheel/transparency"].v), 2, true, SmoothingModeHighQuality, &background);
						hiex::EasyX_Gdiplus_FillRoundRect(UIControl[L"RoundRect/BrushColorChooseMark/x"].v, UIControl[L"RoundRect/BrushColorChooseMark/y"].v, UIControl[L"RoundRect/BrushColorChooseMark/width"].v, UIControl[L"RoundRect/BrushColorChooseMark/height"].v, UIControl[L"RoundRect/BrushColorChooseMark/ellipsewidth"].v, UIControl[L"RoundRect/BrushColorChooseMark/ellipseheight"].v, UIControlColor[L"RoundRect/BrushColorChooseMark/frame"].v, UIControlColor[L"RoundRect/BrushColorChooseMark/fill"].v, 2, true, SmoothingModeHighQuality, &background);

						{
							hiex::EasyX_Gdiplus_FillRoundRect(UIControl[L"RoundRect/BrushColorChooseMarkR/x"].v, UIControl[L"RoundRect/BrushColorChooseMarkR/y"].v, UIControl[L"RoundRect/BrushColorChooseMarkR/width"].v, UIControl[L"RoundRect/BrushColorChooseMarkR/height"].v, UIControl[L"RoundRect/BrushColorChooseMarkR/ellipsewidth"].v, UIControl[L"RoundRect/BrushColorChooseMarkR/ellipseheight"].v, UIControlColor[L"RoundRect/BrushColorChooseMarkR/frame"].v, UIControlColor[L"RoundRect/BrushColorChooseMarkR/fill"].v, 2, true, SmoothingModeHighQuality, &background);
							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"RoundRect/BrushColorChooseMarkR/text"].v, true));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = UIControl[L"RoundRect/BrushColorChooseMarkR/x"].v;
								words_rect.top = UIControl[L"RoundRect/BrushColorChooseMarkR/y"].v;
								words_rect.right = UIControl[L"RoundRect/BrushColorChooseMarkR/x"].v + UIControl[L"RoundRect/BrushColorChooseMarkR/width"].v;
								words_rect.bottom = UIControl[L"RoundRect/BrushColorChooseMarkR/y"].v + UIControl[L"RoundRect/BrushColorChooseMarkR/height"].v + 3;
							}
							graphics.DrawString(to_wstring(GetRValue(brush.color)).c_str(), -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}
						{
							hiex::EasyX_Gdiplus_FillRoundRect(UIControl[L"RoundRect/BrushColorChooseMarkG/x"].v, UIControl[L"RoundRect/BrushColorChooseMarkG/y"].v, UIControl[L"RoundRect/BrushColorChooseMarkG/width"].v, UIControl[L"RoundRect/BrushColorChooseMarkG/height"].v, UIControl[L"RoundRect/BrushColorChooseMarkG/ellipsewidth"].v, UIControl[L"RoundRect/BrushColorChooseMarkG/ellipseheight"].v, UIControlColor[L"RoundRect/BrushColorChooseMarkG/frame"].v, UIControlColor[L"RoundRect/BrushColorChooseMarkG/fill"].v, 2, true, SmoothingModeHighQuality, &background);
							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"RoundRect/BrushColorChooseMarkG/text"].v, true));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = UIControl[L"RoundRect/BrushColorChooseMarkG/x"].v;
								words_rect.top = UIControl[L"RoundRect/BrushColorChooseMarkG/y"].v;
								words_rect.right = UIControl[L"RoundRect/BrushColorChooseMarkG/x"].v + UIControl[L"RoundRect/BrushColorChooseMarkG/width"].v;
								words_rect.bottom = UIControl[L"RoundRect/BrushColorChooseMarkG/y"].v + UIControl[L"RoundRect/BrushColorChooseMarkG/height"].v + 3;
							}
							graphics.DrawString(to_wstring(GetGValue(brush.color)).c_str(), -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}
						{
							hiex::EasyX_Gdiplus_FillRoundRect(UIControl[L"RoundRect/BrushColorChooseMarkB/x"].v, UIControl[L"RoundRect/BrushColorChooseMarkB/y"].v, UIControl[L"RoundRect/BrushColorChooseMarkB/width"].v, UIControl[L"RoundRect/BrushColorChooseMarkB/height"].v, UIControl[L"RoundRect/BrushColorChooseMarkB/ellipsewidth"].v, UIControl[L"RoundRect/BrushColorChooseMarkB/ellipseheight"].v, UIControlColor[L"RoundRect/BrushColorChooseMarkB/frame"].v, UIControlColor[L"RoundRect/BrushColorChooseMarkB/fill"].v, 2, true, SmoothingModeHighQuality, &background);
							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"RoundRect/BrushColorChooseMarkB/text"].v, true));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = UIControl[L"RoundRect/BrushColorChooseMarkB/x"].v;
								words_rect.top = UIControl[L"RoundRect/BrushColorChooseMarkB/y"].v;
								words_rect.right = UIControl[L"RoundRect/BrushColorChooseMarkB/x"].v + UIControl[L"RoundRect/BrushColorChooseMarkB/width"].v;
								words_rect.bottom = UIControl[L"RoundRect/BrushColorChooseMarkB/y"].v + UIControl[L"RoundRect/BrushColorChooseMarkB/height"].v + 3;
							}
							graphics.DrawString(to_wstring(GetBValue(brush.color)).c_str(), -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}
					}
				}

				hiex::EasyX_Gdiplus_FillRoundRect(UIControl[L"RoundRect/BrushTop/x"].v, UIControl[L"RoundRect/BrushTop/y"].v, UIControl[L"RoundRect/BrushTop/width"].v, UIControl[L"RoundRect/BrushTop/height"].v, UIControl[L"RoundRect/BrushTop/ellipseheight"].v, UIControl[L"RoundRect/BrushTop/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushTop/frame"].v, UIControlColor[L"RoundRect/BrushTop/fill"].v, 2, true, SmoothingModeHighQuality, &background);
				{
					{
						hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/BrushColor1/x"].v, UIControl[L"RoundRect/BrushColor1/y"].v, UIControl[L"RoundRect/BrushColor1/width"].v, UIControl[L"RoundRect/BrushColor1/height"].v, UIControl[L"RoundRect/BrushColor1/ellipseheight"].v, UIControl[L"RoundRect/BrushColor1/ellipsewidth"].v, SET_ALPHA(UIControlColor[L"RoundRect/BrushColor1/fill"].v, (int)UIControl[L"RoundRect/BrushColor1/transparency"].v), true, SmoothingModeHighQuality, &background);
						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorFrame1/x"].v, UIControl[L"RoundRect/BrushColorFrame1/y"].v, UIControl[L"RoundRect/BrushColorFrame1/width"].v, UIControl[L"RoundRect/BrushColorFrame1/height"].v, UIControl[L"RoundRect/BrushColorFrame1/ellipseheight"].v, UIControl[L"RoundRect/BrushColorFrame1/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushColorFrame1/frame"].v, UIControl[L"RoundRect/BrushColorFrame1/thickness"].v, true, SmoothingModeHighQuality, &background);

						hiex::TransparentImage(&background, UIControl[L"Image/BrushColor1/x"].v, UIControl[L"Image/BrushColor1/y"].v, &floating_icon[17], UIControl[L"Image/BrushColor1/transparency"].v);
					}
					{
						hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/BrushColor2/x"].v, UIControl[L"RoundRect/BrushColor2/y"].v, UIControl[L"RoundRect/BrushColor2/width"].v, UIControl[L"RoundRect/BrushColor2/height"].v, UIControl[L"RoundRect/BrushColor2/ellipseheight"].v, UIControl[L"RoundRect/BrushColor2/ellipsewidth"].v, SET_ALPHA(UIControlColor[L"RoundRect/BrushColor2/fill"].v, (int)UIControl[L"RoundRect/BrushColor2/transparency"].v), true, SmoothingModeHighQuality, &background);
						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorFrame2/x"].v, UIControl[L"RoundRect/BrushColorFrame2/y"].v, UIControl[L"RoundRect/BrushColorFrame2/width"].v, UIControl[L"RoundRect/BrushColorFrame2/height"].v, UIControl[L"RoundRect/BrushColorFrame2/ellipseheight"].v, UIControl[L"RoundRect/BrushColorFrame2/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushColorFrame2/frame"].v, UIControl[L"RoundRect/BrushColorFrame2/thickness"].v, true, SmoothingModeHighQuality, &background);

						hiex::TransparentImage(&background, UIControl[L"Image/BrushColor2/x"].v, UIControl[L"Image/BrushColor2/y"].v, &floating_icon[17], UIControl[L"Image/BrushColor2/transparency"].v);
					}
					{
						hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/BrushColor3/x"].v, UIControl[L"RoundRect/BrushColor3/y"].v, UIControl[L"RoundRect/BrushColor3/width"].v, UIControl[L"RoundRect/BrushColor3/height"].v, UIControl[L"RoundRect/BrushColor3/ellipseheight"].v, UIControl[L"RoundRect/BrushColor3/ellipsewidth"].v, SET_ALPHA(UIControlColor[L"RoundRect/BrushColor3/fill"].v, (int)UIControl[L"RoundRect/BrushColor3/transparency"].v), true, SmoothingModeHighQuality, &background);
						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorFrame3/x"].v, UIControl[L"RoundRect/BrushColorFrame3/y"].v, UIControl[L"RoundRect/BrushColorFrame3/width"].v, UIControl[L"RoundRect/BrushColorFrame3/height"].v, UIControl[L"RoundRect/BrushColorFrame3/ellipseheight"].v, UIControl[L"RoundRect/BrushColorFrame3/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushColorFrame3/frame"].v, UIControl[L"RoundRect/BrushColorFrame3/thickness"].v, true, SmoothingModeHighQuality, &background);

						hiex::TransparentImage(&background, UIControl[L"Image/BrushColor3/x"].v, UIControl[L"Image/BrushColor3/y"].v, &floating_icon[17], UIControl[L"Image/BrushColor3/transparency"].v);
					}
					{
						hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/BrushColor4/x"].v, UIControl[L"RoundRect/BrushColor4/y"].v, UIControl[L"RoundRect/BrushColor4/width"].v, UIControl[L"RoundRect/BrushColor4/height"].v, UIControl[L"RoundRect/BrushColor4/ellipseheight"].v, UIControl[L"RoundRect/BrushColor4/ellipsewidth"].v, SET_ALPHA(UIControlColor[L"RoundRect/BrushColor4/fill"].v, (int)UIControl[L"RoundRect/BrushColor4/transparency"].v), true, SmoothingModeHighQuality, &background);
						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorFrame4/x"].v, UIControl[L"RoundRect/BrushColorFrame4/y"].v, UIControl[L"RoundRect/BrushColorFrame4/width"].v, UIControl[L"RoundRect/BrushColorFrame4/height"].v, UIControl[L"RoundRect/BrushColorFrame4/ellipseheight"].v, UIControl[L"RoundRect/BrushColorFrame4/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushColorFrame4/frame"].v, UIControl[L"RoundRect/BrushColorFrame4/thickness"].v, true, SmoothingModeHighQuality, &background);

						hiex::TransparentImage(&background, UIControl[L"Image/BrushColor4/x"].v, UIControl[L"Image/BrushColor4/y"].v, &floating_icon[17], UIControl[L"Image/BrushColor4/transparency"].v);
					}
					{
						hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/BrushColor5/x"].v, UIControl[L"RoundRect/BrushColor5/y"].v, UIControl[L"RoundRect/BrushColor5/width"].v, UIControl[L"RoundRect/BrushColor5/height"].v, UIControl[L"RoundRect/BrushColor5/ellipseheight"].v, UIControl[L"RoundRect/BrushColor5/ellipsewidth"].v, SET_ALPHA(UIControlColor[L"RoundRect/BrushColor5/fill"].v, (int)UIControl[L"RoundRect/BrushColor5/transparency"].v), true, SmoothingModeHighQuality, &background);
						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorFrame5/x"].v, UIControl[L"RoundRect/BrushColorFrame5/y"].v, UIControl[L"RoundRect/BrushColorFrame5/width"].v, UIControl[L"RoundRect/BrushColorFrame5/height"].v, UIControl[L"RoundRect/BrushColorFrame5/ellipseheight"].v, UIControl[L"RoundRect/BrushColorFrame5/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushColorFrame5/frame"].v, UIControl[L"RoundRect/BrushColorFrame5/thickness"].v, true, SmoothingModeHighQuality, &background);

						hiex::TransparentImage(&background, UIControl[L"Image/BrushColor5/x"].v, UIControl[L"Image/BrushColor5/y"].v, &floating_icon[17], UIControl[L"Image/BrushColor5/transparency"].v);
					}
					{
						hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/BrushColor6/x"].v, UIControl[L"RoundRect/BrushColor6/y"].v, UIControl[L"RoundRect/BrushColor6/width"].v, UIControl[L"RoundRect/BrushColor6/height"].v, UIControl[L"RoundRect/BrushColor6/ellipseheight"].v, UIControl[L"RoundRect/BrushColor6/ellipsewidth"].v, SET_ALPHA(UIControlColor[L"RoundRect/BrushColor6/fill"].v, (int)UIControl[L"RoundRect/BrushColor6/transparency"].v), true, SmoothingModeHighQuality, &background);
						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorFrame6/x"].v, UIControl[L"RoundRect/BrushColorFrame6/y"].v, UIControl[L"RoundRect/BrushColorFrame6/width"].v, UIControl[L"RoundRect/BrushColorFrame6/height"].v, UIControl[L"RoundRect/BrushColorFrame6/ellipseheight"].v, UIControl[L"RoundRect/BrushColorFrame6/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushColorFrame6/frame"].v, UIControl[L"RoundRect/BrushColorFrame6/thickness"].v, true, SmoothingModeHighQuality, &background);

						hiex::TransparentImage(&background, UIControl[L"Image/BrushColor6/x"].v, UIControl[L"Image/BrushColor6/y"].v, &floating_icon[17], UIControl[L"Image/BrushColor6/transparency"].v);
					}
					{
						hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/BrushColor7/x"].v, UIControl[L"RoundRect/BrushColor7/y"].v, UIControl[L"RoundRect/BrushColor7/width"].v, UIControl[L"RoundRect/BrushColor7/height"].v, UIControl[L"RoundRect/BrushColor7/ellipseheight"].v, UIControl[L"RoundRect/BrushColor7/ellipsewidth"].v, SET_ALPHA(UIControlColor[L"RoundRect/BrushColor7/fill"].v, (int)UIControl[L"RoundRect/BrushColor7/transparency"].v), true, SmoothingModeHighQuality, &background);
						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorFrame7/x"].v, UIControl[L"RoundRect/BrushColorFrame7/y"].v, UIControl[L"RoundRect/BrushColorFrame7/width"].v, UIControl[L"RoundRect/BrushColorFrame7/height"].v, UIControl[L"RoundRect/BrushColorFrame7/ellipseheight"].v, UIControl[L"RoundRect/BrushColorFrame7/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushColorFrame7/frame"].v, UIControl[L"RoundRect/BrushColorFrame7/thickness"].v, true, SmoothingModeHighQuality, &background);

						hiex::TransparentImage(&background, UIControl[L"Image/BrushColor7/x"].v, UIControl[L"Image/BrushColor7/y"].v, &floating_icon[17], UIControl[L"Image/BrushColor7/transparency"].v);
					}
					{
						hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/BrushColor8/x"].v, UIControl[L"RoundRect/BrushColor8/y"].v, UIControl[L"RoundRect/BrushColor8/width"].v, UIControl[L"RoundRect/BrushColor8/height"].v, UIControl[L"RoundRect/BrushColor8/ellipseheight"].v, UIControl[L"RoundRect/BrushColor8/ellipsewidth"].v, SET_ALPHA(UIControlColor[L"RoundRect/BrushColor8/fill"].v, (int)UIControl[L"RoundRect/BrushColor8/transparency"].v), true, SmoothingModeHighQuality, &background);
						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorFrame8/x"].v, UIControl[L"RoundRect/BrushColorFrame8/y"].v, UIControl[L"RoundRect/BrushColorFrame8/width"].v, UIControl[L"RoundRect/BrushColorFrame8/height"].v, UIControl[L"RoundRect/BrushColorFrame8/ellipseheight"].v, UIControl[L"RoundRect/BrushColorFrame8/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushColorFrame8/frame"].v, UIControl[L"RoundRect/BrushColorFrame8/thickness"].v, true, SmoothingModeHighQuality, &background);

						hiex::TransparentImage(&background, UIControl[L"Image/BrushColor8/x"].v, UIControl[L"Image/BrushColor8/y"].v, &floating_icon[17], UIControl[L"Image/BrushColor8/transparency"].v);
					}
					{
						hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/BrushColor9/x"].v, UIControl[L"RoundRect/BrushColor9/y"].v, UIControl[L"RoundRect/BrushColor9/width"].v, UIControl[L"RoundRect/BrushColor9/height"].v, UIControl[L"RoundRect/BrushColor9/ellipseheight"].v, UIControl[L"RoundRect/BrushColor9/ellipsewidth"].v, SET_ALPHA(UIControlColor[L"RoundRect/BrushColor9/fill"].v, (int)UIControl[L"RoundRect/BrushColor9/transparency"].v), true, SmoothingModeHighQuality, &background);
						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorFrame9/x"].v, UIControl[L"RoundRect/BrushColorFrame9/y"].v, UIControl[L"RoundRect/BrushColorFrame9/width"].v, UIControl[L"RoundRect/BrushColorFrame9/height"].v, UIControl[L"RoundRect/BrushColorFrame9/ellipseheight"].v, UIControl[L"RoundRect/BrushColorFrame9/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushColorFrame9/frame"].v, UIControl[L"RoundRect/BrushColorFrame9/thickness"].v, true, SmoothingModeHighQuality, &background);

						hiex::TransparentImage(&background, UIControl[L"Image/BrushColor9/x"].v, UIControl[L"Image/BrushColor9/y"].v, &floating_icon[17], UIControl[L"Image/BrushColor9/transparency"].v);
					}
					{
						hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/BrushColor10/x"].v, UIControl[L"RoundRect/BrushColor10/y"].v, UIControl[L"RoundRect/BrushColor10/width"].v, UIControl[L"RoundRect/BrushColor10/height"].v, UIControl[L"RoundRect/BrushColor10/ellipseheight"].v, UIControl[L"RoundRect/BrushColor10/ellipsewidth"].v, SET_ALPHA(UIControlColor[L"RoundRect/BrushColor10/fill"].v, (int)UIControl[L"RoundRect/BrushColor10/transparency"].v), true, SmoothingModeHighQuality, &background);
						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorFrame10/x"].v, UIControl[L"RoundRect/BrushColorFrame10/y"].v, UIControl[L"RoundRect/BrushColorFrame10/width"].v, UIControl[L"RoundRect/BrushColorFrame10/height"].v, UIControl[L"RoundRect/BrushColorFrame10/ellipseheight"].v, UIControl[L"RoundRect/BrushColorFrame10/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushColorFrame10/frame"].v, UIControl[L"RoundRect/BrushColorFrame10/thickness"].v, true, SmoothingModeHighQuality, &background);

						hiex::TransparentImage(&background, UIControl[L"Image/BrushColor10/x"].v, UIControl[L"Image/BrushColor10/y"].v, &floating_icon[17], UIControl[L"Image/BrushColor10/transparency"].v);
					}
					{
						hiex::EasyX_Gdiplus_SolidRoundRect(UIControl[L"RoundRect/BrushColor11/x"].v, UIControl[L"RoundRect/BrushColor11/y"].v, UIControl[L"RoundRect/BrushColor11/width"].v, UIControl[L"RoundRect/BrushColor11/height"].v, UIControl[L"RoundRect/BrushColor11/ellipseheight"].v, UIControl[L"RoundRect/BrushColor11/ellipsewidth"].v, SET_ALPHA(UIControlColor[L"RoundRect/BrushColor11/fill"].v, (int)UIControl[L"RoundRect/BrushColor11/transparency"].v), true, SmoothingModeHighQuality, &background);
						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorFrame11/x"].v, UIControl[L"RoundRect/BrushColorFrame11/y"].v, UIControl[L"RoundRect/BrushColorFrame11/width"].v, UIControl[L"RoundRect/BrushColorFrame11/height"].v, UIControl[L"RoundRect/BrushColorFrame11/ellipseheight"].v, UIControl[L"RoundRect/BrushColorFrame11/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushColorFrame11/frame"].v, UIControl[L"RoundRect/BrushColorFrame11/thickness"].v, true, SmoothingModeHighQuality, &background);

						hiex::TransparentImage(&background, UIControl[L"Image/BrushColor11/x"].v, UIControl[L"Image/BrushColor11/y"].v, &floating_icon[17], UIControl[L"Image/BrushColor11/transparency"].v);
					}
					{
						IMAGE img = DrawHSVWheel(UIControl[L"RoundRect/BrushColor12/width"].v, UIControl[L"RoundRect/BrushColor12/width"].v / 2 - 10, UIControlTarget[L"RoundRect/BrushColor12/angle"].v);
						hiex::TransparentImage(&background, UIControl[L"RoundRect/BrushColor12/x"].v, UIControl[L"RoundRect/BrushColor12/y"].v, &img, UIControl[L"RoundRect/BrushColor12/transparency"].v);
						hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/BrushColorFrame12/x"].v, UIControl[L"RoundRect/BrushColorFrame12/y"].v, UIControl[L"RoundRect/BrushColorFrame12/width"].v, UIControl[L"RoundRect/BrushColorFrame12/height"].v, UIControl[L"RoundRect/BrushColorFrame12/ellipseheight"].v, UIControl[L"RoundRect/BrushColorFrame12/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushColorFrame12/frame"].v, UIControl[L"RoundRect/BrushColorFrame12/thickness"].v, true, SmoothingModeHighQuality, &background);
					}

					//画笔粗细
					{
						hiex::EasyX_Gdiplus_FillRoundRect(UIControl[L"RoundRect/PaintThickness/x"].v, UIControl[L"RoundRect/PaintThickness/y"].v, UIControl[L"RoundRect/PaintThickness/width"].v, UIControl[L"RoundRect/PaintThickness/height"].v, UIControl[L"RoundRect/PaintThickness/ellipseheight"].v, UIControl[L"RoundRect/PaintThickness/ellipsewidth"].v, UIControlColor[L"RoundRect/PaintThickness/frame"].v, UIControlColor[L"RoundRect/PaintThickness/fill"].v, 2, true, SmoothingModeHighQuality, &background);
						{
							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, UIControl[L"Words/PaintThickness/size"].v, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"Words/PaintThickness/words_color"].v, true));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = UIControl[L"Words/PaintThickness/left"].v;
								words_rect.top = UIControl[L"Words/PaintThickness/top"].v;
								words_rect.right = words_rect.left + UIControl[L"Words/PaintThickness/width"].v;
								words_rect.bottom = words_rect.top + UIControl[L"Words/PaintThickness/height"].v;
							}
							graphics.DrawString(L"粗\n细", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}

						hiex::EasyX_Gdiplus_FillRoundRect(UIControl[L"RoundRect/PaintThicknessPrompt/x"].v, UIControl[L"RoundRect/PaintThicknessPrompt/y"].v, min(UIControl[L"RoundRect/PaintThickness/x"].v + 97 - UIControl[L"RoundRect/PaintThicknessPrompt/x"].v, UIControl[L"RoundRect/PaintThicknessPrompt/width"].v), min(UIControl[L"RoundRect/PaintThickness/y"].v + 72 - UIControl[L"RoundRect/PaintThicknessPrompt/y"].v, UIControl[L"RoundRect/PaintThicknessPrompt/height"].v), max(10, UIControl[L"RoundRect/PaintThicknessPrompt/ellipseheight"].v - max(0, UIControl[L"RoundRect/PaintThicknessPrompt/width"].v - 60) * 2), max(10, UIControl[L"RoundRect/PaintThicknessPrompt/ellipseheight"].v - max(0, UIControl[L"RoundRect/PaintThicknessPrompt/height"].v - 60) * 2), UIControlColor[L"RoundRect/PaintThicknessPrompt/frame"].v, UIControlColor[L"RoundRect/PaintThicknessPrompt/fill"].v, 2, true, SmoothingModeHighQuality, &background);
						{
							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, UIControl[L"Words/PaintThicknessValue/size"].v, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"Words/PaintThicknessValue/words_color"].v, true));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = UIControl[L"Words/PaintThicknessValue/left"].v;
								words_rect.top = UIControl[L"Words/PaintThicknessValue/top"].v;
								words_rect.right = words_rect.left + UIControl[L"Words/PaintThicknessValue/width"].v;
								words_rect.bottom = words_rect.top + UIControl[L"Words/PaintThicknessValue/height"].v;
							}
							graphics.DrawString(to_wstring(brush.width).c_str(), -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}
					}
				}

				hiex::EasyX_Gdiplus_FillRoundRect(UIControl[L"RoundRect/BrushBottom/x"].v, UIControl[L"RoundRect/BrushBottom/y"].v, UIControl[L"RoundRect/BrushBottom/width"].v, UIControl[L"RoundRect/BrushBottom/height"].v, UIControl[L"RoundRect/BrushBottom/ellipseheight"].v, UIControl[L"RoundRect/BrushBottom/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushBottom/frame"].v, UIControlColor[L"RoundRect/BrushBottom/fill"].v, 2, true, SmoothingModeHighQuality, &background);
				{
					hiex::EasyX_Gdiplus_SolidRectangle(UIControl[L"RoundRect/BrushInterval/x"].v, UIControl[L"RoundRect/BrushInterval/y"].v, UIControl[L"RoundRect/BrushInterval/width"].v, UIControl[L"RoundRect/BrushInterval/height"].v, UIControlColor[L"RoundRect/BrushInterval/fill"].v, true, SmoothingModeHighQuality, &background);

					{
						ChangeColor(floating_icon[12], UIControlColor[L"Image/PaintBrush/words_color"].v);
						hiex::TransparentImage(&background, UIControl[L"Image/PaintBrush/x"].v, UIControl[L"Image/PaintBrush/y"].v, &floating_icon[12], (UIControlColor[L"Image/PaintBrush/words_color"].v >> 24) & 0xff);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, UIControl[L"Words/PaintBrush/size"].v, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"Words/PaintBrush/words_color"].v, true));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							if ((int)state == 1) words_rect.left = UIControl[L"Words/PaintBrush/left"].v;
							else words_rect.left = min(UIControl[L"Words/PaintBrush/left"].v, UIControl[L"RoundRect/BrushBottom/x"].v + UIControl[L"RoundRect/BrushBottom/width"].v - 55);

							words_rect.top = UIControl[L"Words/PaintBrush/top"].v;
							words_rect.right = words_rect.left + UIControl[L"Words/PaintBrush/width"].v;
							words_rect.bottom = words_rect.top + UIControl[L"Words/PaintBrush/height"].v;
						}
						graphics.DrawString(L"画笔", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
					}
					{
						ChangeColor(floating_icon[11], UIControlColor[L"Image/FluorescentBrush/words_color"].v);
						hiex::TransparentImage(&background, UIControl[L"Image/FluorescentBrush/x"].v, UIControl[L"Image/FluorescentBrush/y"].v, &floating_icon[11], (UIControlColor[L"Image/FluorescentBrush/words_color"].v >> 24) & 0xff);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, UIControl[L"Words/FluorescentBrush/size"].v, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"Words/FluorescentBrush/words_color"].v, true));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							if ((int)state == 1) words_rect.left = UIControl[L"Words/FluorescentBrush/left"].v;
							else words_rect.left = min(UIControl[L"Words/FluorescentBrush/left"].v, UIControl[L"RoundRect/BrushBottom/x"].v + UIControl[L"RoundRect/BrushBottom/width"].v - 55);

							words_rect.top = UIControl[L"Words/FluorescentBrush/top"].v;
							words_rect.right = words_rect.left + UIControl[L"Words/FluorescentBrush/width"].v;
							words_rect.bottom = words_rect.top + UIControl[L"Words/FluorescentBrush/height"].v;
						}
						graphics.DrawString(L"荧光笔", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
					}

					{
						ChangeColor(floating_icon[13], UIControlColor[L"Image/WriteBrush/words_color"].v);
						hiex::TransparentImage(&background, UIControl[L"Image/WriteBrush/x"].v, UIControl[L"Image/WriteBrush/y"].v, &floating_icon[13], (UIControlColor[L"Image/WriteBrush/words_color"].v >> 24) & 0xff);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, UIControl[L"Words/WriteBrush/size"].v, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"Words/WriteBrush/words_color"].v, true));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							if ((int)state == 1) words_rect.left = UIControl[L"Words/WriteBrush/left"].v;
							else words_rect.left = min(UIControl[L"Words/LineBrush/left"].v, UIControl[L"RoundRect/BrushBottom/x"].v + UIControl[L"RoundRect/BrushBottom/width"].v - 55);

							words_rect.top = UIControl[L"Words/WriteBrush/top"].v;
							words_rect.right = words_rect.left + UIControl[L"Words/WriteBrush/width"].v;
							words_rect.bottom = words_rect.top + UIControl[L"Words/WriteBrush/height"].v;
						}
						graphics.DrawString(L"书写", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
					}
					{
						ChangeColor(floating_icon[14], UIControlColor[L"Image/LineBrush/words_color"].v);
						hiex::TransparentImage(&background, UIControl[L"Image/LineBrush/x"].v, UIControl[L"Image/LineBrush/y"].v, &floating_icon[14], (UIControlColor[L"Image/LineBrush/words_color"].v >> 24) & 0xff);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, UIControl[L"Words/LineBrush/size"].v, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"Words/LineBrush/words_color"].v, true));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							if ((int)state == 1) words_rect.left = UIControl[L"Words/LineBrush/left"].v;
							else words_rect.left = min(UIControl[L"Words/LineBrush/left"].v, UIControl[L"RoundRect/BrushBottom/x"].v + UIControl[L"RoundRect/BrushBottom/width"].v - 55);

							words_rect.top = UIControl[L"Words/LineBrush/top"].v;
							words_rect.right = words_rect.left + UIControl[L"Words/LineBrush/width"].v;
							words_rect.bottom = words_rect.top + UIControl[L"Words/LineBrush/height"].v;
						}
						graphics.DrawString(L"直线", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
					}
					{
						ChangeColor(floating_icon[15], UIControlColor[L"Image/RectangleBrush/words_color"].v);
						hiex::TransparentImage(&background, UIControl[L"Image/RectangleBrush/x"].v, UIControl[L"Image/RectangleBrush/y"].v, &floating_icon[15], (UIControlColor[L"Image/RectangleBrush/words_color"].v >> 24) & 0xff);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, UIControl[L"Words/RectangleBrush/size"].v, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"Words/RectangleBrush/words_color"].v, true));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							if ((int)state == 1) words_rect.left = UIControl[L"Words/RectangleBrush/left"].v;
							else words_rect.left = min(UIControl[L"Words/RectangleBrush/left"].v, UIControl[L"RoundRect/BrushBottom/x"].v + UIControl[L"RoundRect/BrushBottom/width"].v - 55);

							words_rect.top = UIControl[L"Words/RectangleBrush/top"].v;
							words_rect.right = words_rect.left + UIControl[L"Words/RectangleBrush/width"].v;
							words_rect.bottom = words_rect.top + UIControl[L"Words/RectangleBrush/height"].v;
						}
						graphics.DrawString(L"矩形", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
					}

					hiex::EasyX_Gdiplus_RoundRect(max(UIControl[L"RoundRect/BrushChoose/x"].v, UIControl[L"RoundRect/BrushBottom/x"].v + 5), UIControl[L"RoundRect/BrushChoose/y"].v, UIControl[L"RoundRect/BrushChoose/width"].v, UIControl[L"RoundRect/BrushChoose/height"].v, UIControl[L"RoundRect/BrushChoose/ellipseheight"].v, UIControl[L"RoundRect/BrushChoose/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushChoose/frame"].v, 2, true, SmoothingModeHighQuality, &background);
					hiex::EasyX_Gdiplus_RoundRect(min(UIControl[L"RoundRect/BrushMode/x"].v, UIControl[L"RoundRect/BrushBottom/x"].v + UIControl[L"RoundRect/BrushBottom/width"].v - UIControl[L"RoundRect/BrushMode/width"].v - 5), UIControl[L"RoundRect/BrushMode/y"].v, UIControl[L"RoundRect/BrushMode/width"].v, UIControl[L"RoundRect/BrushMode/height"].v, UIControl[L"RoundRect/BrushMode/ellipseheight"].v, UIControl[L"RoundRect/BrushMode/ellipsewidth"].v, UIControlColor[L"RoundRect/BrushMode/frame"].v, 2, true, SmoothingModeHighQuality, &background);
				}
			}

			{
				Graphics eraser(GetImageHDC(&background));
				GraphicsPath path;

				path.AddArc(1, floating_windows.height - 155 - 2, 25, 25, 180, 90);
				path.AddArc(1 + floating_windows.width - 48 + 8 - 25 - 1, floating_windows.height - 155 - 2, 25, 25, 270, 90);
				path.AddArc(1 + floating_windows.width - 48 + 8 - 25 - 1, floating_windows.height - 155 - 2 + 94 + 4 - 25 - 1, 25, 25, 0, 90);
				path.AddArc(1, floating_windows.height - 155 - 2 + 94 + 4 - 25 - 1, 25, 25, 90, 90);
				path.CloseFigure();

				Region region(&path);
				eraser.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
				eraser.SetClip(&region, CombineModeReplace);
				eraser.Clear(Color(0, 0, 0, 0));
				eraser.ResetClip();
			}
			hiex::EasyX_Gdiplus_FillRoundRect(UIControl[L"RoundRect/RoundRect1/x"].v, UIControl[L"RoundRect/RoundRect1/y"].v, UIControl[L"RoundRect/RoundRect1/width"].v, UIControl[L"RoundRect/RoundRect1/height"].v, UIControl[L"RoundRect/RoundRect1/ellipsewidth"].v, UIControl[L"RoundRect/RoundRect1/ellipseheight"].v, UIControlColor[L"RoundRect/RoundRect1/frame"].v, UIControlColor[L"RoundRect/RoundRect1/fill"].v, 2, true, SmoothingModeHighQuality, &background);

			//选择
			{
				ChangeColor(floating_icon[0], UIControlColor[L"Image/choose/fill"].v);
				hiex::TransparentImage(&background, UIControl[L"Image/choose/x"].v, UIControl[L"Image/choose/y"].v, &floating_icon[0], (UIControlColor[L"Image/choose/fill"].v >> 24) & 0xff);

				Gdiplus::Font gp_font(&HarmonyOS_fontFamily, UIControl[L"Words/choose/height"].v, FontStyleRegular, UnitPixel);
				SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"Words/choose/words_color"].v, true));
				graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
				{
					words_rect.left = UIControl[L"Words/choose/left"].v;
					words_rect.top = UIControl[L"Words/choose/top"].v;
					words_rect.right = UIControl[L"Words/choose/right"].v;
					words_rect.bottom = UIControl[L"Words/choose/bottom"].v;
				}
				graphics.DrawString(L"选择", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
			}
			//画笔
			{
				ChangeColor(floating_icon[1], UIControlColor[L"Image/brush/fill"].v);
				hiex::TransparentImage(&background, UIControl[L"Image/brush/x"].v, UIControl[L"Image/brush/y"].v, &floating_icon[1], (UIControlColor[L"Image/brush/fill"].v >> 24) & 0xff);

				Gdiplus::Font gp_font(&HarmonyOS_fontFamily, UIControl[L"Words/brush/height"].v, FontStyleRegular, UnitPixel);
				SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"Words/brush/words_color"].v, true));
				graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
				{
					words_rect.left = UIControl[L"Words/brush/left"].v;
					words_rect.top = UIControl[L"Words/brush/top"].v;
					words_rect.right = UIControl[L"Words/brush/right"].v;
					words_rect.bottom = UIControl[L"Words/brush/bottom"].v;
				}
				graphics.DrawString(L"画笔", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);

				WordBrush.SetColor(hiex::ConvertToGdiplusColor(UIControlColor[L"Words/brushSize/words_color"].v, true));
				Gdiplus::Font gp_font_02(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
				graphics.DrawString(to_wstring(brush.width).c_str(), -1, &gp_font_02, { UIControl[L"Words/brushSize/left"].v ,UIControl[L"Words/brushSize/top"].v }, &WordBrush);

				/*
				if (state == 1.1 || state == 1.11 || state == 1.12)
				{
					hiex::EasyX_Gdiplus_FillRoundRect(0, floating_windows.height - 256, floating_windows.width - 106, 90, 25, 25, RGB(150, 150, 150), RGB(255, 255, 255), 1, false, SmoothingModeHighQuality, &background);

					//白
					hiex::EasyX_Gdiplus_FillEllipse(20, floating_windows.height - 246, 40, 40, RGB(150, 150, 150), color_preinstall[WHITE], 2, false, SmoothingModeHighQuality, &background);
					if (brush.color == color_preinstall[WHITE]) hiex::EasyX_Gdiplus_Ellipse(15, floating_windows.height - 251, 50, 50, RGB(98, 175, 82), 2, false, SmoothingModeHighQuality, &background);
					//黑
					hiex::EasyX_Gdiplus_FillEllipse(60, floating_windows.height - 216, 40, 40, RGB(150, 150, 150), color_preinstall[BLACK], 2, false, SmoothingModeHighQuality, &background);
					if (brush.color == color_preinstall[BLACK]) hiex::EasyX_Gdiplus_Ellipse(55, floating_windows.height - 221, 50, 50, RGB(98, 175, 82), 2, false, SmoothingModeHighQuality, &background);
					//黄
					hiex::EasyX_Gdiplus_FillEllipse(100, floating_windows.height - 246, 40, 40, RGB(150, 150, 150), color_preinstall[YELLOW], 2, false, SmoothingModeHighQuality, &background);
					if (brush.color == color_preinstall[YELLOW]) hiex::EasyX_Gdiplus_Ellipse(95, floating_windows.height - 251, 50, 50, RGB(98, 175, 82), 2, false, SmoothingModeHighQuality, &background);
					//蓝
					hiex::EasyX_Gdiplus_FillEllipse(140, floating_windows.height - 216, 40, 40, RGB(150, 150, 150), color_preinstall[BLUE], 2, false, SmoothingModeHighQuality, &background);
					if (brush.color == color_preinstall[BLUE]) hiex::EasyX_Gdiplus_Ellipse(135, floating_windows.height - 221, 50, 50, RGB(98, 175, 82), 2, false, SmoothingModeHighQuality, &background);
					//绿
					hiex::EasyX_Gdiplus_FillEllipse(180, floating_windows.height - 246, 40, 40, RGB(150, 150, 150), color_preinstall[GREEN], 2, false, SmoothingModeHighQuality, &background);
					if (brush.color == color_preinstall[GREEN]) hiex::EasyX_Gdiplus_Ellipse(175, floating_windows.height - 251, 50, 50, RGB(98, 175, 82), 2, false, SmoothingModeHighQuality, &background);
					//红
					hiex::EasyX_Gdiplus_FillEllipse(220, floating_windows.height - 216, 40, 40, RGB(150, 150, 150), color_preinstall[RED], 2, false, SmoothingModeHighQuality, &background);
					if (brush.color == color_preinstall[RED]) hiex::EasyX_Gdiplus_Ellipse(215, floating_windows.height - 221, 50, 50, RGB(98, 175, 82), 2, false, SmoothingModeHighQuality, &background);

					//画板模式
					{
						hiex::EasyX_Gdiplus_FillRoundRect(275, floating_windows.height - 246, 90, 70, 25, 25, RGB(150, 150, 150), color_distance(WHITE, brush.color) >= 120 ? WHITE : RGB(130, 130, 130), 2, false, SmoothingModeHighQuality, &background);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(brush.color, false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							words_rect.left = 275;
							words_rect.top = floating_windows.height - 246 + 45;
							words_rect.right = 275 + 90;
							words_rect.bottom = floating_windows.height - 246 + 70;
						}
						graphics.DrawString(L"标准笔迹", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);

						ChangeColor(floating_icon[10], brush.color);
						hiex::TransparentImage(&background, 275 + 25, floating_windows.height - 242, &floating_icon[10]);
					}
					//画笔粗细
					{
						if (brush.mode == 1)
						{
							hiex::EasyX_Gdiplus_FillRoundRect(370, floating_windows.height - 246, 90, 70, 25, 25, RGB(150, 150, 150), color_distance(WHITE, brush.color) >= 120 ? WHITE : RGB(130, 130, 130), 2, false, SmoothingModeHighQuality, &background);
							{
								Gdiplus::Graphics graphics(GetImageHDC(&background));
								Gdiplus::Pen pen(hiex::ConvertToGdiplusColor(brush.color, false), min(10, brush.width));
								pen.SetStartCap(LineCapRound);
								pen.SetEndCap(LineCapRound);

								graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
								graphics.SetSmoothingMode(SmoothingModeHighQuality);
								graphics.DrawLine(&pen, 380, floating_windows.height - 236, 380 + 70, floating_windows.height - 236 + 30);
							}

							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(brush.color, false));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = 370;
								words_rect.top = floating_windows.height - 246 + 45;
								words_rect.right = 370 + 90;
								words_rect.bottom = floating_windows.height - 246 + 70;
							}
							graphics.DrawString(L"画笔粗细", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);

							Gdiplus::Font gp_font_2(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
							{
								words_rect.left = 360 + 62;
								words_rect.top = floating_windows.height - 246;
								words_rect.right = 360 + 100;
								words_rect.bottom = floating_windows.height - 246 + 30;
							}
							graphics.DrawString(to_wstring(brush.width).c_str(), -1, &gp_font_2, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}
						else if (brush.mode == 2)
						{
							hiex::EasyX_Gdiplus_FillRoundRect(370, floating_windows.height - 246, 90, 70, 25, 25, RGB(150, 150, 150), color_distance(WHITE, brush.color) >= 120 ? WHITE : RGB(130, 130, 130), 2, false, SmoothingModeHighQuality, &background);
							{
								Gdiplus::Graphics graphics(GetImageHDC(&background));
								Gdiplus::Pen pen(hiex::ConvertToGdiplusColor(brush.color, false), min(10, brush.width));
								pen.SetStartCap(LineCapRound);
								pen.SetEndCap(LineCapRound);

								graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
								graphics.SetSmoothingMode(SmoothingModeHighQuality);
								graphics.DrawLine(&pen, 380, floating_windows.height - 236, 380 + 70, floating_windows.height - 236 + 30);
							}

							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(brush.color, false));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = 370;
								words_rect.top = floating_windows.height - 246 + 45;
								words_rect.right = 370 + 90;
								words_rect.bottom = floating_windows.height - 246 + 70;
							}
							graphics.DrawString(L"画笔粗细", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);

							Gdiplus::Font gp_font_2(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
							{
								words_rect.left = 360 + 62;
								words_rect.top = floating_windows.height - 246;
								words_rect.right = 360 + 100;
								words_rect.bottom = floating_windows.height - 246 + 30;
							}
							graphics.DrawString(to_wstring(brush.width * 10).c_str(), -1, &gp_font_2, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}
						else if (brush.mode == 3)
						{
							hiex::EasyX_Gdiplus_FillRoundRect(370, floating_windows.height - 246, 90, 70, 25, 25, RGB(150, 150, 150), color_distance(WHITE, brush.color) >= 120 ? WHITE : RGB(130, 130, 130), 2, false, SmoothingModeHighQuality, &background);
							{
								Gdiplus::Graphics graphics(GetImageHDC(&background));
								Gdiplus::Pen pen(hiex::ConvertToGdiplusColor(brush.color, false), min(10, brush.width));
								pen.SetStartCap(LineCapRound);
								pen.SetEndCap(LineCapRound);

								graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
								graphics.SetSmoothingMode(SmoothingModeHighQuality);
								graphics.DrawLine(&pen, 380, floating_windows.height - 236, 380 + 70, floating_windows.height - 236 + 30);
							}

							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(brush.color, false));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = 370;
								words_rect.top = floating_windows.height - 246 + 45;
								words_rect.right = 370 + 90;
								words_rect.bottom = floating_windows.height - 246 + 70;
							}
							graphics.DrawString(L"直线粗细", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);

							Gdiplus::Font gp_font_2(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
							{
								words_rect.left = 360 + 62;
								words_rect.top = floating_windows.height - 246;
								words_rect.right = 360 + 100;
								words_rect.bottom = floating_windows.height - 246 + 30;
							}
							graphics.DrawString(to_wstring(brush.width).c_str(), -1, &gp_font_2, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}
						else if (brush.mode == 4)
						{
							hiex::EasyX_Gdiplus_FillRoundRect(370, floating_windows.height - 246, 90, 70, 25, 25, RGB(150, 150, 150), color_distance(WHITE, brush.color) >= 120 ? WHITE : RGB(130, 130, 130), 2, false, SmoothingModeHighQuality, &background);
							{
								Gdiplus::Graphics graphics(GetImageHDC(&background));
								Gdiplus::Pen pen(hiex::ConvertToGdiplusColor(brush.color, false), min(10, brush.width));
								pen.SetStartCap(LineCapRound);
								pen.SetEndCap(LineCapRound);

								graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
								graphics.SetSmoothingMode(SmoothingModeHighQuality);
								graphics.DrawLine(&pen, 380, floating_windows.height - 236, 380 + 70, floating_windows.height - 236 + 30);
							}

							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(brush.color, false));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = 370;
								words_rect.top = floating_windows.height - 246 + 45;
								words_rect.right = 370 + 90;
								words_rect.bottom = floating_windows.height - 246 + 70;
							}
							graphics.DrawString(L"边框粗细", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);

							Gdiplus::Font gp_font_2(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
							{
								words_rect.left = 360 + 62;
								words_rect.top = floating_windows.height - 246;
								words_rect.right = 360 + 100;
								words_rect.bottom = floating_windows.height - 246 + 30;
							}
							graphics.DrawString(to_wstring(brush.width).c_str(), -1, &gp_font_2, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}
					}

	*/
			}
			//橡皮
			{
				ChangeColor(floating_icon[2], UIControlColor[L"Image/rubber/fill"].v);
				hiex::TransparentImage(&background, UIControl[L"Image/rubber/x"].v, UIControl[L"Image/rubber/y"].v, &floating_icon[2], (UIControlColor[L"Image/rubber/fill"].v >> 24) & 0xff);

				Gdiplus::Font gp_font(&HarmonyOS_fontFamily, UIControl[L"Words/rubber/height"].v, FontStyleRegular, UnitPixel);
				SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"Words/rubber/words_color"].v, true));
				graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
				{
					words_rect.left = UIControl[L"Words/rubber/left"].v;
					words_rect.top = UIControl[L"Words/rubber/top"].v;
					words_rect.right = UIControl[L"Words/rubber/right"].v;
					words_rect.bottom = UIControl[L"Words/rubber/bottom"].v;
				}
				graphics.DrawString(L"橡皮", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
			}
			//选项
			{
				ChangeColor(floating_icon[7], UIControlColor[L"Image/test/fill"].v);
				hiex::TransparentImage(&background, UIControl[L"Image/test/x"].v, UIControl[L"Image/test/y"].v, &floating_icon[7], (UIControlColor[L"Image/test/fill"].v >> 24) & 0xff);

				Gdiplus::Font gp_font(&HarmonyOS_fontFamily, UIControl[L"Words/test/height"].v, FontStyleRegular, UnitPixel);
				SolidBrush WordBrush(hiex::ConvertToGdiplusColor(UIControlColor[L"Words/test/words_color"].v, true));
				graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
				{
					words_rect.left = UIControl[L"Words/test/left"].v;
					words_rect.top = UIControl[L"Words/test/top"].v;
					words_rect.right = UIControl[L"Words/test/right"].v;
					words_rect.bottom = UIControl[L"Words/test/bottom"].v;
				}
				graphics.DrawString(L"选项", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
			}

			//插件空位1：随机点名
			{
				if (UIControl[L"RoundRect/RoundRect2/x"].v == 8 && plug_in_RandomRollCall.select)
				{
					hiex::EasyX_Gdiplus_FillRoundRect(2, floating_windows.height - 55, 100, 40, 20, 20, RGB(130, 130, 130), BackgroundColorMode == 0 ? RGB(255, 255, 255) : RGB(30, 33, 41), 1, false, SmoothingModeHighQuality, &background);
					hiex::EasyX_Gdiplus_RoundRect(2 + 3, floating_windows.height - 55 + 3, 100 - 6, 40 - 6, 20, 20, RGB(200, 200, 200), 2, false, SmoothingModeHighQuality, &background);

					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 19, FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(110, 110, 110), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						words_rect.left = 2;
						words_rect.top = (floating_windows.height - 55);
						words_rect.right = 2 + 100;
						words_rect.bottom = (floating_windows.height - 55) + 43;
					}
					graphics.DrawString(L"随机点名", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);

					plug_in_RandomRollCall.select = 2;
				}
				else if (plug_in_RandomRollCall.select) plug_in_RandomRollCall.select = 1;
			}

			hiex::EasyX_Gdiplus_RoundRect(UIControl[L"RoundRect/RoundRect2/x"].v, UIControl[L"RoundRect/RoundRect2/y"].v, UIControl[L"RoundRect/RoundRect2/width"].v, UIControl[L"RoundRect/RoundRect2/height"].v, UIControl[L"RoundRect/RoundRect2/ellipsewidth"].v, UIControl[L"RoundRect/RoundRect2/ellipseheight"].v, UIControlColor[L"RoundRect/RoundRect2/frame"].v, 2.5, true, SmoothingModeHighQuality, &background);

			/*

			//放大器
			{
				{
					ChangeColor(floating_icon[4], RGB(130, 130, 130));
					hiex::TransparentImage(&background, 288 + 28, floating_windows.height - 140, &floating_icon[4]);

					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(130, 130, 130), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						words_rect.left = 288 + 7;
						words_rect.top = floating_windows.height - 155 + 48;
						words_rect.right = 288 + 7 + 83;
						words_rect.bottom = floating_windows.height - 155 + 48 + 48;
					}
					graphics.DrawString(L"缩放器", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
				}
			}
			//更多
			{
				if (test.select == true)
				{
					ChangeColor(floating_icon[5], RGB(98, 175, 82));
					hiex::TransparentImage(&background, 384 + 28, floating_windows.height - 140, &floating_icon[5]);
					hiex::EasyX_Gdiplus_RoundRect(384 + 8, floating_windows.height - 156 + 8, 80, 80, 25, 25, RGB(98, 175, 82), 2.5, false, SmoothingModeHighQuality, &background);

					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(98, 175, 82), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						words_rect.left = 384 + 7;
						words_rect.top = floating_windows.height - 155 + 48;
						words_rect.right = 384 + 7 + 83;
						words_rect.bottom = floating_windows.height - 155 + 48 + 48;
					}
					graphics.DrawString(L"触控调测", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
				}
				else
				{
					ChangeColor(floating_icon[5], RGB(130, 130, 130));
					hiex::TransparentImage(&background, 384 + 28, floating_windows.height - 140, &floating_icon[5]);

					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(130, 130, 130), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						words_rect.left = 384 + 7;
						words_rect.top = floating_windows.height - 155 + 48;
						words_rect.right = 384 + 7 + 83;
						words_rect.bottom = floating_windows.height - 155 + 48 + 48;
					}
					graphics.DrawString(L"触控调测", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
				}
			}
	*/

	//主按钮
			if (1)
			{
				{
					Graphics eraser(GetImageHDC(&background));
					GraphicsPath path;

					path.AddArc(UIControl[L"Ellipse/Ellipse1/x"].v, UIControl[L"Ellipse/Ellipse1/y"].v, (UIControl[L"Ellipse/Ellipse1/width"].v + 2), UIControl[L"Ellipse/Ellipse1/height"].v, 270, -180);
					path.AddLine((UIControl[L"Ellipse/Ellipse1/x"].v + 2) + UIControl[L"Ellipse/Ellipse1/width"].v / 2, (UIControl[L"Ellipse/Ellipse1/y"].v + 1) + UIControl[L"Ellipse/Ellipse1/height"].v, (float)floating_windows.width, (UIControl[L"Ellipse/Ellipse1/y"].v + 1) + UIControl[L"Ellipse/Ellipse1/height"].v);
					path.AddLine((float)floating_windows.width, (UIControl[L"Ellipse/Ellipse1/y"].v + 1) + UIControl[L"Ellipse/Ellipse1/height"].v, (float)floating_windows.width, (UIControl[L"Ellipse/Ellipse1/y"].v - 2));
					path.AddLine((float)floating_windows.width, (UIControl[L"Ellipse/Ellipse1/y"].v - 2), (UIControl[L"Ellipse/Ellipse1/x"].v + 2) + UIControl[L"Ellipse/Ellipse1/width"].v / 2, (UIControl[L"Ellipse/Ellipse1/y"].v - 2));
					path.CloseFigure();

					Region region(&path);
					eraser.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
					eraser.SetClip(&region, CombineModeReplace);
					eraser.Clear(Color(0, 0, 0, 255));
					eraser.ResetClip();
				}
				hiex::EasyX_Gdiplus_FillEllipse(UIControl[L"Ellipse/Ellipse1/x"].v, UIControl[L"Ellipse/Ellipse1/y"].v, UIControl[L"Ellipse/Ellipse1/width"].v, UIControl[L"Ellipse/Ellipse1/height"].v, SET_ALPHA(UIControlColor[L"Ellipse/Ellipse1/frame"].v, (int)UIControl[L"Image/Sign1/frame_transparency"].v), UIControlColor[L"Ellipse/Ellipse1/fill"].v, 2, true, SmoothingModeHighQuality, &background);
				hiex::TransparentImage(&background, UIControl[L"Image/Sign1/x"].v, UIControl[L"Image/Sign1/y"].v, &sign, UIControl[L"Image/Sign1/transparency"].v);

				if (ppt_info_stay.TotalPage != -1) hiex::TransparentImage(&background, floating_windows.width - 45, floating_windows.height - 95, &floating_icon[16]);
				else if (SeewoCamera == true) hiex::TransparentImage(&background, floating_windows.width - 45, floating_windows.height - 95, &floating_icon[18]);

				if (choose.select != true && (int)state == 1)
				{
					hiex::EasyX_Gdiplus_FillRoundRect(floating_windows.width - 96, floating_windows.height - 257, 96, 96, 25, 25, RGB(150, 150, 150), BackgroundColorMode == 0 ? RGB(255, 255, 255) : RGB(30, 33, 41), 2, false, SmoothingModeHighQuality, &background);

					if (penetrate.select == true)
					{
						hiex::EasyX_Gdiplus_RoundRect(floating_windows.width - 96 + 4, floating_windows.height - 256 + 8, 88, 40, 20, 20, RGB(98, 175, 82), 2.5, false, SmoothingModeHighQuality, &background);
						ChangeColor(floating_icon[6], RGB(98, 175, 82));
						hiex::TransparentImage(&background, floating_windows.width - 96 + 10, floating_windows.height - 256 + 12, &floating_icon[6]);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(98, 175, 82), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							words_rect.left = floating_windows.width - 96 + 40;
							words_rect.top = floating_windows.height - 256 + 8;
							words_rect.right = floating_windows.width - 96 + 40 + 56;
							words_rect.bottom = floating_windows.height - 256 + 8 + 40;
						}
						graphics.DrawString(L"穿透", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
					}
					else
					{
						ChangeColor(floating_icon[6], RGB(130, 130, 130));
						hiex::TransparentImage(&background, floating_windows.width - 96 + 10, floating_windows.height - 256 + 12, &floating_icon[6]);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(130, 130, 130), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							words_rect.left = floating_windows.width - 96 + 40;
							words_rect.top = floating_windows.height - 256 + 8;
							words_rect.right = floating_windows.width - 96 + 40 + 56;
							words_rect.bottom = floating_windows.height - 256 + 8 + 40;
						}
						graphics.DrawString(L"穿透", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
					}
				}

				if ((int)state == 1)
				{
					if (ppt_show == NULL && choose.select == true)
					{
						hiex::EasyX_Gdiplus_FillRoundRect(floating_windows.width - 96, floating_windows.height - 256 + 44, 96, 51, 25, 25, RGB(150, 150, 150), BackgroundColorMode == 0 ? RGB(255, 255, 255) : RGB(30, 33, 41), 2, false, SmoothingModeHighQuality, &background);

						if (FreezeFrame.mode == 1)
						{
							hiex::EasyX_Gdiplus_RoundRect(floating_windows.width - 96 + 4, floating_windows.height - 256 + 50, 88, 40, 20, 20, RGB(98, 175, 82), 2.5, false, SmoothingModeHighQuality, &background);
							ChangeColor(floating_icon[8], RGB(98, 175, 82));
							hiex::TransparentImage(&background, floating_windows.width - 96 + 10, floating_windows.height - 256 + 54, &floating_icon[8]);

							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(98, 175, 82), false));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = floating_windows.width - 96 + 40;
								words_rect.top = floating_windows.height - 256 + 50;
								words_rect.right = floating_windows.width - 96 + 40 + 56;
								words_rect.bottom = floating_windows.height - 256 + 50 + 40;
							}
							graphics.DrawString(L"定格", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}
						else
						{
							ChangeColor(floating_icon[8], RGB(130, 130, 130));
							hiex::TransparentImage(&background, floating_windows.width - 96 + 10, floating_windows.height - 256 + 54, &floating_icon[8]);

							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(130, 130, 130), false));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = floating_windows.width - 96 + 40;
								words_rect.top = floating_windows.height - 256 + 50;
								words_rect.right = floating_windows.width - 96 + 40 + 56;
								words_rect.bottom = floating_windows.height - 256 + 50 + 40;
							}
							graphics.DrawString(L"定格", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}
					}
					else if (choose.select == false)
					{
						if (FreezeFrame.mode == 1)
						{
							hiex::EasyX_Gdiplus_RoundRect(floating_windows.width - 96 + 4, floating_windows.height - 256 + 50, 88, 40, 20, 20, RGB(98, 175, 82), 2.5, false, SmoothingModeHighQuality, &background);
							ChangeColor(floating_icon[8], RGB(98, 175, 82));
							hiex::TransparentImage(&background, floating_windows.width - 96 + 10, floating_windows.height - 256 + 54, &floating_icon[8]);

							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(98, 175, 82), false));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = floating_windows.width - 96 + 40;
								words_rect.top = floating_windows.height - 256 + 50;
								words_rect.right = floating_windows.width - 96 + 40 + 56;
								words_rect.bottom = floating_windows.height - 256 + 50 + 40;
							}
							graphics.DrawString(L"定格", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}
						else
						{
							ChangeColor(floating_icon[8], RGB(130, 130, 130));
							hiex::TransparentImage(&background, floating_windows.width - 96 + 10, floating_windows.height - 256 + 54, &floating_icon[8]);

							Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
							SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(130, 130, 130), false));
							graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
							{
								words_rect.left = floating_windows.width - 96 + 40;
								words_rect.top = floating_windows.height - 256 + 50;
								words_rect.right = floating_windows.width - 96 + 40 + 56;
								words_rect.bottom = floating_windows.height - 256 + 50 + 40;
							}
							graphics.DrawString(L"定格", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
						}
					}
				}

				if (choose.select == false && RecallImage.size() > 1)
				{
					hiex::EasyX_Gdiplus_FillRoundRect(floating_windows.width - 96, floating_windows.height - 55, 96, 40, 25, 25, (!choose.select && !rubber.select) ? brush.color : RGBA(255, 255, 255, 255), RGBA(0, 0, 0, 150), 2, true, SmoothingModeHighQuality, &background);
					ChangeColor(floating_icon[3], RGB(255, 255, 255));
					hiex::TransparentImage(&background, floating_windows.width - 86, floating_windows.height - 50, &floating_icon[3]);

					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(255, 255, 255), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						words_rect.left = (floating_windows.width - 96 + 30);
						words_rect.top = (floating_windows.height - 55);
						words_rect.right = (floating_windows.width - 96 + 30) + 66;
						words_rect.bottom = (floating_windows.height - 55) + 42;
					}
					graphics.DrawString(L"撤回", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
				}
				else if (choose.select == true && (int)state == 1 && RestoreSketchpad)
				{
					hiex::EasyX_Gdiplus_FillRoundRect(floating_windows.width - 96, floating_windows.height - 55, 96, 40, 25, 25, (!choose.select && !rubber.select) ? brush.color : RGBA(255, 255, 255, 255), RGBA(0, 0, 0, 150), 2, true, SmoothingModeHighQuality, &background);
					ChangeColor(floating_icon[3], RGB(255, 255, 255));
					hiex::TransparentImage(&background, floating_windows.width - 86, floating_windows.height - 50, &floating_icon[3]);

					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(255, 255, 255), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						words_rect.left = (floating_windows.width - 96 + 30);
						words_rect.top = (floating_windows.height - 55);
						words_rect.right = (floating_windows.width - 96 + 30) + 66;
						words_rect.bottom = (floating_windows.height - 55) + 42;
					}
					graphics.DrawString(L"恢复", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
				}
			}

			//更新窗口
			{
				// 设置窗口位置
				POINT ptDst = { floating_windows.x, floating_windows.y };

				ulwi.pptDst = &ptDst;
				ulwi.hdcSrc = GetImageHDC(&background);
				UpdateLayeredWindowIndirect(floating_window, &ulwi);
			}
		}
		else state = target_status;

		if (for_num == 1) ShowWindow(floating_window, SW_SHOW);
		hiex::DelayFPS(40);
	}

	ShowWindow(floating_window, SW_HIDE);
	thread_status[L"DrawScreen"] = false;
}
void SeekBar(ExMessage m)
{
	if (!KEY_DOWN(VK_LBUTTON)) return;

	if (m.lbutton)
	{
		POINT p;
		GetCursorPos(&p);

		int pop_x = p.x;
		int pop_y = p.y;

		while (1)
		{
			if (!KEY_DOWN(VK_LBUTTON)) break;

			POINT p;
			GetCursorPos(&p);

			pop_x = p.x;
			pop_y = p.y;

			SetWindowPos(floating_window, NULL,
				floating_windows.x = min(GetSystemMetrics(SM_CXSCREEN) - floating_windows.width, max(1, p.x - m.x)),
				floating_windows.y = min(GetSystemMetrics(SM_CYSCREEN) - floating_windows.height, max(1, p.y - m.y)),
				0,
				0,
				SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
		}
	}

	return;
}

void MouseInteraction()
{
	int brush_connect = -1;

	ExMessage m;
	while (!off_signal)
	{
		hiex::getmessage_win32(&m, EM_MOUSE, floating_window);

		if ((int)state == 0)
		{
			if (IsInRect(m.x, m.y, { floating_windows.width - 96, floating_windows.height - 156, floating_windows.width - 96 + 96, floating_windows.height - 156 + 96 }))
			{
				if (m.lbutton)
				{
					int lx = m.x, ly = m.y;
					while (1)
					{
						ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
						if (abs(m.x - lx) <= 5 && abs(m.y - ly) <= 5)
						{
							if (!m.lbutton)
							{
								target_status = 1;

								break;
							}
						}
						else
						{
							SeekBar(m);

							hiex::flushmessage_win32(EM_MOUSE, floating_window);

							break;
						}
					}
				}

				if (m.rbutton)
				{
					off_signal = true;
				}
			}
			if (choose.select == false && state == int(state) && RecallImage.size() > 1 && IsInRect(m.x, m.y, { floating_windows.width - 96, floating_windows.height - 55, floating_windows.width - 96 + 96, floating_windows.height - 50 + 40 }))
			{
				if (m.lbutton)
				{
					int lx = m.x, ly = m.y;
					while (1)
					{
						ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
						if (IsInRect(m.x, m.y, { floating_windows.width - 96, floating_windows.height - 55, floating_windows.width - 96 + 96, floating_windows.height - 50 + 40 }))
						{
							if (!m.lbutton)
							{
								RecallImage.pop_back();
								deque<RecallStruct>(RecallImage).swap(RecallImage); // 使用swap技巧来释放未使用的内存

								if (!RecallImage.empty()) drawpad = RecallImage.back().img;
								else SetImageColor(drawpad, RGBA(0, 0, 0, 1), true);

								extreme_point = RecallImage.back().extreme_point;

								{
									// 设置BLENDFUNCTION结构体
									BLENDFUNCTION blend;
									blend.BlendOp = AC_SRC_OVER;
									blend.BlendFlags = 0;
									blend.SourceConstantAlpha = 255; // 设置透明度，0为全透明，255为不透明
									blend.AlphaFormat = AC_SRC_ALPHA; // 使用源图像的alpha通道
									HDC hdcScreen = GetDC(NULL);
									// 调用UpdateLayeredWindow函数更新窗口
									POINT ptSrc = { 0,0 };
									SIZE sizeWnd = { drawpad.getwidth(),drawpad.getheight() };
									POINT ptDst = { 0,0 }; // 设置窗口位置
									UPDATELAYEREDWINDOWINFO ulwi = { 0 };
									ulwi.cbSize = sizeof(ulwi);
									ulwi.hdcDst = hdcScreen;
									ulwi.pptDst = &ptDst;
									ulwi.psize = &sizeWnd;
									ulwi.pptSrc = &ptSrc;
									ulwi.crKey = RGB(255, 255, 255);
									ulwi.pblend = &blend;
									ulwi.dwFlags = ULW_ALPHA;

									// 定义要更新的矩形区域
									RECT rcDirty = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

									ulwi.hdcSrc = GetImageHDC(&drawpad);
									ulwi.prcDirty = &rcDirty;
									UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
								}

								break;
							}
						}
						else
						{
							hiex::flushmessage_win32(EM_MOUSE, floating_window);

							break;
						}
					}
					hiex::flushmessage_win32(EM_MOUSE, floating_window);
				}
			}
		}
		if ((int)state == 1)
		{
			if (IsInRect(m.x, m.y, { floating_windows.width - 96, floating_windows.height - 156, floating_windows.width - 96 + 96, floating_windows.height - 156 + 96 }))
			{
				if (m.lbutton)
				{
					int lx = m.x, ly = m.y;
					while (1)
					{
						ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
						if (m.x == lx && m.y == ly)
						{
							if (!m.lbutton)
							{
								state = 1;

								target_status = 0;

								break;
							}
						}
						else
						{
							SeekBar(m);

							hiex::flushmessage_win32(EM_MOUSE, floating_window);

							break;
						}
					}
				}
			}

			//窗口穿透
			if (choose.select == false && IsInRect(m.x, m.y, { floating_windows.width - 96 + 4, floating_windows.height - 256 + 8, floating_windows.width - 96 + 4 + 88, floating_windows.height - 256 + 8 + 40 }))
			{
				if (m.lbutton)
				{
					int lx = m.x, ly = m.y;
					while (1)
					{
						ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
						if (IsInRect(m.x, m.y, { floating_windows.width - 96 + 4, floating_windows.height - 256 + 8, floating_windows.width - 96 + 4 + 88, floating_windows.height - 256 + 8 + 40 }))
						{
							if (!m.lbutton)
							{
								if (penetrate.select)
								{
									penetrate.select = false;
									if (FreezeFrame.mode == 2) FreezeFrame.mode = 1;
								}
								else
								{
									if (FreezeFrame.mode == 1) FreezeFrame.mode = 2;
									penetrate.select = true;
								}

								break;
							}
						}
						else
						{
							hiex::flushmessage_win32(EM_MOUSE, floating_window);

							break;
						}
					}
					hiex::flushmessage_win32(EM_MOUSE, floating_window);
				}
			}
			//窗口定格
			if (ppt_show == NULL && IsInRect(m.x, m.y, { floating_windows.width - 96 + 4, floating_windows.height - 256 + 50, floating_windows.width - 96 + 4 + 88, floating_windows.height - 256 + 50 + 40 }))
			{
				if (m.lbutton)
				{
					int lx = m.x, ly = m.y;
					while (1)
					{
						ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
						if (IsInRect(m.x, m.y, { floating_windows.width - 96 + 4, floating_windows.height - 256 + 50, floating_windows.width - 96 + 4 + 88, floating_windows.height - 256 + 50 + 40 }))
						{
							if (!m.lbutton)
							{
								if (FreezeFrame.mode != 1)
								{
									penetrate.select = false;

									if (choose.select == true) FreezeFrame.select = true;
									FreezeFrame.mode = 1;
								}
								else FreezeFrame.mode = 0, FreezeFrame.select = false;

								break;
							}
						}
						else
						{
							hiex::flushmessage_win32(EM_MOUSE, floating_window);

							break;
						}
					}
					hiex::flushmessage_win32(EM_MOUSE, floating_window);
				}
			}

			//选择
			if (IsInRect(m.x, m.y, { 0 + 8, floating_windows.height - 156 + 8, 0 + 8 + 80, floating_windows.height - 156 + 8 + 80 }))
			{
				if (m.lbutton)
				{
					int lx = m.x, ly = m.y;
					while (1)
					{
						ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
						if (IsInRect(m.x, m.y, { 0 + 8, floating_windows.height - 156 + 8, 0 + 8 + 80, floating_windows.height - 156 + 8 + 80 }))
						{
							if (!m.lbutton)
							{
								if (choose.select == false)
								{
									state = 1;
									if (!FreezeFrame.select || penetrate.select) FreezeFrame.mode = 0, FreezeFrame.select = false;

									brush.select = false;
									rubber.select = false;
									choose.select = true;
									penetrate.select = false;
								}
								else if (choose.mode == true)
								{
									choose.mode = false;
								}
								else if (choose.mode == false)
								{
									choose.mode = true;
								}

								break;
							}
						}
						else
						{
							hiex::flushmessage_win32(EM_MOUSE, floating_window);

							break;
						}
					}
					hiex::flushmessage_win32(EM_MOUSE, floating_window);
				}
			}
			//画笔
			if (IsInRect(m.x, m.y, { 96 + 8, floating_windows.height - 156 + 8, 96 + 8 + 80, floating_windows.height - 156 + 8 + 80 }))
			{
				if (m.lbutton)
				{
					int lx = m.x, ly = m.y;
					brush_connect = false;

					while (1)
					{
						ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
						if (IsInRect(m.x, m.y, { 96 + 8, floating_windows.height - 156 + 8, 96 + 8 + 80, floating_windows.height - 156 + 8 + 80 }))
						{
							if (abs(ly - m.y) >= 20 && state == 1)
							{
								brush.select = true;
								rubber.select = false;
								choose.select = false;
								state = 1.1, brush_connect = true;

								if (SeewoCamera) FreezeFrame.mode = 1;
							}
							else if (abs(ly - m.y) >= 20) brush_connect = true;
							else
							{
								if (!m.lbutton)
								{
									if (brush.select == false)
									{
										state = 1;
										brush.select = true;
										rubber.select = false;
										choose.select = false;

										if (SeewoCamera) FreezeFrame.mode = 1;
									}
									else if (state == 1)
									{
										state = 1.1;
									}
									else if ((state == 1.1 || state == 1.11 || state == 1.12) && ly - m.y < 20)
									{
										state = 1;
									}

									break;
								}
							}
						}
						else
						{
							hiex::flushmessage_win32(EM_MOUSE, floating_window);

							break;
						}
					}
					hiex::flushmessage_win32(EM_MOUSE, floating_window);
				}
			}
			//画笔选项1
			else if (state == 1.1 || state == 1.11 || state == 1.12)
			{
				if ((state == 1.1 || state == 1.11 || state == 1.12) && IsInRect(m.x, m.y, { int(UIControl[L"RoundRect/PaintThickness/x"].v), int(UIControl[L"RoundRect/PaintThickness/y"].v), int(UIControl[L"RoundRect/PaintThickness/x"].v + UIControl[L"RoundRect/PaintThickness/width"].v), int(UIControl[L"RoundRect/PaintThickness/y"].v + UIControl[L"RoundRect/PaintThickness/height"].v) }))
				{
					if (m.lbutton)
					{
						int lx = m.x, ly = m.y;
						while (1)
						{
							ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
							if (IsInRect(m.x, m.y, { int(UIControl[L"RoundRect/PaintThickness/x"].v), int(UIControl[L"RoundRect/PaintThickness/y"].v), int(UIControl[L"RoundRect/PaintThickness/x"].v + UIControl[L"RoundRect/PaintThickness/width"].v), int(UIControl[L"RoundRect/PaintThickness/y"].v + UIControl[L"RoundRect/PaintThickness/height"].v) }))
							{
								if (!m.lbutton)
								{
									if (state == 1.1 || state == 1.12) state = 1.11;
									else state = 1.1;

									break;
								}
							}
							else
							{
								hiex::flushmessage_win32(EM_MOUSE, floating_window);

								break;
							}
						}
						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (state == 1.11 && IsInRect(m.x, m.y, { 15, floating_windows.height - 312 + 10, 355, floating_windows.height - 312 + 40 }))
				{
					if (m.lbutton)
					{
						POINT pt;

						while (1)
						{
							UIControlTarget[L"RoundRect/PaintThicknessSchedule3/width"].v = UIControlTarget[L"RoundRect/PaintThicknessSchedule3/height"].v = 10;

							GetCursorPos(&pt);

							int idx = max(10, min(320, pt.x - floating_windows.x - 17));

							if (idx <= 200) brush.width = 1 + double(idx - 10) / 190.0 * 49.0;
							else if (idx <= 260) brush.width = 51 + double(idx - 200) / 60.0 * 49.0;
							else brush.width = 101 + double(idx - 260) / 60.0 * 399.0;

							if (!KEY_DOWN(VK_LBUTTON)) break;
						}
						UIControlTarget[L"RoundRect/PaintThicknessSchedule3/width"].v = UIControlTarget[L"RoundRect/PaintThicknessSchedule3/height"].v = 20;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (state == 1.11 && IsInRect(m.x, m.y, { 355, floating_windows.height - 312 + 5, 455, floating_windows.height - 312 + 45 }))
				{
					if (brush.mode != 2)
					{
						if (IsInRect(m.x, m.y, { 365, floating_windows.height - 312 + 5, 395, floating_windows.height - 312 + 45 }))
						{
							if (m.lbutton)
							{
								int lx = m.x, ly = m.y;
								while (1)
								{
									ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
									if (IsInRect(m.x, m.y, { 365, floating_windows.height - 312 + 5, 395, floating_windows.height - 312 + 45 }))
									{
										if (!m.lbutton)
										{
											brush.width = 4;
											UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v = 1;

											break;
										}
									}
									else
									{
										hiex::flushmessage_win32(EM_MOUSE, floating_window);

										break;
									}
								}
								hiex::flushmessage_win32(EM_MOUSE, floating_window);
							}
						}
						if (IsInRect(m.x, m.y, { 395, floating_windows.height - 312 + 5, 425, floating_windows.height - 312 + 45 }))
						{
							if (m.lbutton)
							{
								int lx = m.x, ly = m.y;
								while (1)
								{
									ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
									if (IsInRect(m.x, m.y, { 395, floating_windows.height - 312 + 5, 425, floating_windows.height - 312 + 45 }))
									{
										if (!m.lbutton)
										{
											brush.width = 10;
											UIControlTarget[L"RoundRect/PaintThicknessSchedule5a/ellipse"].v = 2;

											break;
										}
									}
									else
									{
										hiex::flushmessage_win32(EM_MOUSE, floating_window);

										break;
									}
								}
								hiex::flushmessage_win32(EM_MOUSE, floating_window);
							}
						}
						if (IsInRect(m.x, m.y, { 425, floating_windows.height - 312 + 5, 455, floating_windows.height - 312 + 45 }))
						{
							if (m.lbutton)
							{
								int lx = m.x, ly = m.y;
								while (1)
								{
									ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
									if (IsInRect(m.x, m.y, { 425, floating_windows.height - 312 + 5, 455, floating_windows.height - 312 + 45 }))
									{
										if (!m.lbutton)
										{
											brush.width = 20;
											UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v = 10;

											break;
										}
									}
									else
									{
										hiex::flushmessage_win32(EM_MOUSE, floating_window);

										break;
									}
								}
								hiex::flushmessage_win32(EM_MOUSE, floating_window);
							}
						}
					}
					else
					{
						if (IsInRect(m.x, m.y, { 355, floating_windows.height - 312 + 5, 405, floating_windows.height - 312 + 45 }))
						{
							if (m.lbutton)
							{
								int lx = m.x, ly = m.y;
								while (1)
								{
									ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
									if (IsInRect(m.x, m.y, { 355, floating_windows.height - 312 + 5, 405, floating_windows.height - 312 + 45 }))
									{
										if (!m.lbutton)
										{
											brush.width = 40;
											UIControlTarget[L"RoundRect/PaintThicknessSchedule4a/ellipse"].v = 20;

											break;
										}
									}
									else
									{
										hiex::flushmessage_win32(EM_MOUSE, floating_window);

										break;
									}
								}
								hiex::flushmessage_win32(EM_MOUSE, floating_window);
							}
						}
						if (IsInRect(m.x, m.y, { 405, floating_windows.height - 312 + 5, 455, floating_windows.height - 312 + 45 }))
						{
							if (m.lbutton)
							{
								int lx = m.x, ly = m.y;
								while (1)
								{
									ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
									if (IsInRect(m.x, m.y, { 405, floating_windows.height - 312 + 5, 455, floating_windows.height - 312 + 45 }))
									{
										if (!m.lbutton)
										{
											brush.width = 50;
											UIControlTarget[L"RoundRect/PaintThicknessSchedule6a/ellipse"].v = 20;

											break;
										}
									}
									else
									{
										hiex::flushmessage_win32(EM_MOUSE, floating_window);

										break;
									}
								}
								hiex::flushmessage_win32(EM_MOUSE, floating_window);
							}
						}
					}
				}
				else if (state == 1.12 && IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColorChoose/x"].v, (int)UIControlTarget[L"RoundRect/BrushColorChoose/y"].v, (int)UIControlTarget[L"RoundRect/BrushColorChoose/x"].v + (int)UIControlTarget[L"RoundRect/BrushColorChoose/width"].v, (int)UIControlTarget[L"RoundRect/BrushColorChoose/y"].v + (int)UIControlTarget[L"RoundRect/BrushColorChoose/height"].v }))
				{
					if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColorChooseWheel/x"].v, (int)UIControlTarget[L"RoundRect/BrushColorChooseWheel/y"].v, (int)UIControlTarget[L"RoundRect/BrushColorChooseWheel/x"].v + (int)UIControlTarget[L"RoundRect/BrushColorChooseWheel/width"].v, (int)UIControlTarget[L"RoundRect/BrushColorChooseWheel/y"].v + (int)UIControlTarget[L"RoundRect/BrushColorChooseWheel/height"].v }))
					{
						if (m.lbutton)
						{
							POINT center{ int(UIControlTarget[L"RoundRect/BrushColorChooseWheel/x"].v + UIControlTarget[L"RoundRect/BrushColorChooseWheel/width"].v / 2), int(UIControlTarget[L"RoundRect/BrushColorChooseWheel/y"].v + UIControlTarget[L"RoundRect/BrushColorChooseWheel/height"].v / 2) };
							POINT pt;

							BrushColorChoose.last_x = -1, BrushColorChoose.last_y = -1;
							while (1)
							{
								GetCursorPos(&pt);

								pt.x -= floating_windows.x;
								pt.y -= floating_windows.y;

								int len = min(int(UIControlTarget[L"RoundRect/BrushColorChooseWheel/width"].v / 2 - 2), int(sqrt(pow(center.x - pt.x, 2) + pow(center.y - pt.y, 2))));
								double length = sqrt(pow(center.x - pt.x, 2) + pow(center.y - pt.y, 2));

								POINT result;
								result.x = center.x + len * (pt.x - center.x) / length - UIControl[L"RoundRect/BrushColorChooseWheel/x"].v;
								result.y = center.y + len * (pt.y - center.y) / length - UIControl[L"RoundRect/BrushColorChooseWheel/y"].v;

								COLORREF ret;
								std::shared_lock<std::shared_mutex> lock(ColorPaletteSm);

								int width = ColorPaletteImg.getwidth();
								DWORD* pBuffer = GetImageBuffer(&ColorPaletteImg);
								DWORD colorValue = pBuffer[result.y * width + result.x];
								int blue = (colorValue & 0xFF);
								int green = (colorValue >> 8) & 0xFF;
								int red = (colorValue >> 16) & 0xFF;

								lock.unlock();

								brush.color = brush.primary_colour = RGBA(red, green, blue, (brush.color >> 24) & 0xFF);
								if (computeContrast(RGB(red, green, blue), RGB(255, 255, 255)) >= 3) BackgroundColorMode = 0;
								else BackgroundColorMode = 1;

								BrushColorChoose.x = result.x, BrushColorChoose.y = result.y;
								UIControlTarget[L"RoundRect/BrushColorChooseMark/x"].v = UIControl[L"RoundRect/BrushColorChooseMark/x"].v = result.x + UIControl[L"RoundRect/BrushColorChooseWheel/x"].v - 7;
								UIControlTarget[L"RoundRect/BrushColorChooseMark/y"].v = UIControl[L"RoundRect/BrushColorChooseMark/y"].v = result.y + UIControl[L"RoundRect/BrushColorChooseWheel/y"].v - 7;

								if (!KEY_DOWN(VK_LBUTTON)) break;
							}
							BrushColorChoose.last_x = BrushColorChoose.x, BrushColorChoose.last_y = BrushColorChoose.y;

							hiex::flushmessage_win32(EM_MOUSE, floating_window);
						}
					}
				}

				else if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor1/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor1/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor1/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor1/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor1/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor1/height"].v }))
				{
					if (m.lbutton)
					{
						brush.color = brush.primary_colour = SET_ALPHA(UIControlColor[L"RoundRect/BrushColor1/fill"].v, 255);
						BackgroundColorMode = 1;

						BrushColorChoose.x = BrushColorChoose.y = 0;
						if (state == 1.11 || state == 1.12) state = 1.1;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor2/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor2/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor2/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor2/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor2/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor2/height"].v }))
				{
					if (m.lbutton)
					{
						brush.color = brush.primary_colour = SET_ALPHA(UIControlColor[L"RoundRect/BrushColor2/fill"].v, 255);
						BackgroundColorMode = 0;

						BrushColorChoose.x = BrushColorChoose.y = 0;
						if (state == 1.11 || state == 1.12) state = 1.1;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor3/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor3/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor3/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor3/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor3/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor3/height"].v }))
				{
					if (m.lbutton)
					{
						brush.color = brush.primary_colour = SET_ALPHA(UIControlColor[L"RoundRect/BrushColor3/fill"].v, 255);
						BackgroundColorMode = 1;

						BrushColorChoose.x = BrushColorChoose.y = 0;
						if (state == 1.11 || state == 1.12) state = 1.1;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor4/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor4/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor4/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor4/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor4/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor4/height"].v }))
				{
					if (m.lbutton)
					{
						brush.color = brush.primary_colour = SET_ALPHA(UIControlColor[L"RoundRect/BrushColor4/fill"].v, 255);
						BackgroundColorMode = 0;

						BrushColorChoose.x = BrushColorChoose.y = 0;
						if (state == 1.11 || state == 1.12) state = 1.1;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor5/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor5/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor5/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor5/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor5/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor5/height"].v }))
				{
					if (m.lbutton)
					{
						brush.color = brush.primary_colour = SET_ALPHA(UIControlColor[L"RoundRect/BrushColor5/fill"].v, 255);
						BackgroundColorMode = 1;

						BrushColorChoose.x = BrushColorChoose.y = 0;
						if (state == 1.11 || state == 1.12) state = 1.1;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor6/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor6/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor6/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor6/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor6/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor6/height"].v }))
				{
					if (m.lbutton)
					{
						brush.color = brush.primary_colour = SET_ALPHA(UIControlColor[L"RoundRect/BrushColor6/fill"].v, 255);
						BackgroundColorMode = 0;

						BrushColorChoose.x = BrushColorChoose.y = 0;
						if (state == 1.11 || state == 1.12) state = 1.1;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor7/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor7/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor7/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor7/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor7/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor7/height"].v }))
				{
					if (m.lbutton)
					{
						brush.color = brush.primary_colour = SET_ALPHA(UIControlColor[L"RoundRect/BrushColor7/fill"].v, 255);
						BackgroundColorMode = 1;

						BrushColorChoose.x = BrushColorChoose.y = 0;
						if (state == 1.11 || state == 1.12) state = 1.1;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor8/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor8/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor8/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor8/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor8/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor8/height"].v }))
				{
					if (m.lbutton)
					{
						brush.color = brush.primary_colour = SET_ALPHA(UIControlColor[L"RoundRect/BrushColor8/fill"].v, 255);
						BackgroundColorMode = 0;

						BrushColorChoose.x = BrushColorChoose.y = 0;
						if (state == 1.11 || state == 1.12) state = 1.1;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor9/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor9/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor9/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor9/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor9/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor9/height"].v }))
				{
					if (m.lbutton)
					{
						brush.color = brush.primary_colour = SET_ALPHA(UIControlColor[L"RoundRect/BrushColor9/fill"].v, 255);
						BackgroundColorMode = 1;

						BrushColorChoose.x = BrushColorChoose.y = 0;
						if (state == 1.11 || state == 1.12) state = 1.1;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor10/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor10/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor10/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor10/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor10/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor10/height"].v }))
				{
					if (m.lbutton)
					{
						brush.color = brush.primary_colour = SET_ALPHA(UIControlColor[L"RoundRect/BrushColor10/fill"].v, 255);
						BackgroundColorMode = 0;

						BrushColorChoose.x = BrushColorChoose.y = 0;
						if (state == 1.11 || state == 1.12) state = 1.1;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor11/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor11/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor11/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor11/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor11/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor11/height"].v }))
				{
					if (m.lbutton)
					{
						brush.color = brush.primary_colour = SET_ALPHA(UIControlColor[L"RoundRect/BrushColor11/fill"].v, 255);
						BackgroundColorMode = 1;

						BrushColorChoose.x = BrushColorChoose.y = 0;
						if (state == 1.11 || state == 1.12) state = 1.1;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor12/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor12/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor12/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor12/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor12/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor12/height"].v }))
				{
					if (m.lbutton)
					{
						int lx = m.x, ly = m.y;
						while (1)
						{
							ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
							if (IsInRect(m.x, m.y, { (int)UIControlTarget[L"RoundRect/BrushColor12/x"].v, (int)UIControlTarget[L"RoundRect/BrushColor12/y"].v, (int)UIControlTarget[L"RoundRect/BrushColor12/x"].v + (int)UIControlTarget[L"RoundRect/BrushColor12/width"].v, (int)UIControlTarget[L"RoundRect/BrushColor12/y"].v + (int)UIControlTarget[L"RoundRect/BrushColor12/height"].v }))
							{
								if (!m.lbutton)
								{
									if (state == 1.12) state = 1.1;
									else state = 1.12;

									break;
								}
							}
							else
							{
								hiex::flushmessage_win32(EM_MOUSE, floating_window);

								break;
							}
						}
						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}

				if (IsInRect(m.x, m.y, { 5, floating_windows.height - 55 + 5, 5 + 90, floating_windows.height - 55 + 5 + 30 }))
				{
					if (m.lbutton)
					{
						if (brush.mode == 2)
						{
							brush.HighlighterWidthHistory = brush.width;
							brush.width = brush.PenWidthHistory;
						}
						brush.mode = 1;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { 95, floating_windows.height - 55 + 5, 95 + 90, floating_windows.height - 55 + 5 + 30 }))
				{
					if (m.lbutton)
					{
						if (brush.mode != 2)
						{
							brush.PenWidthHistory = brush.width;
							brush.width = brush.HighlighterWidthHistory;
						}
						brush.mode = 2;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { 195, floating_windows.height - 55 + 5,195 + 90, floating_windows.height - 55 + 5 + 30 }))
				{
					if (m.lbutton)
					{
						if (brush.mode != 1 && brush.mode != 2)
						{
							brush.mode = 1;
						}

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { 285, floating_windows.height - 55 + 5,285 + 90, floating_windows.height - 55 + 5 + 30 }))
				{
					if (m.lbutton)
					{
						if (brush.mode == 2)
						{
							brush.HighlighterWidthHistory = brush.width;
							brush.width = brush.PenWidthHistory;
						}
						brush.mode = 3;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				else if (IsInRect(m.x, m.y, { 375, floating_windows.height - 55 + 5,375 + 90, floating_windows.height - 55 + 5 + 30 }))
				{
					if (m.lbutton)
					{
						if (brush.mode == 2)
						{
							brush.HighlighterWidthHistory = brush.width;
							brush.width = brush.PenWidthHistory;
						}
						brush.mode = 4;

						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}

				if (!m.lbutton && (IsInRect(m.x, m.y, { 1, floating_windows.height - 256, 1 + floating_windows.width - 106, floating_windows.height - 256 + 90 }) || IsInRect(m.x, m.y, { 0, floating_windows.height - 50, 0 + floating_windows.width, floating_windows.height - 50 + 50 })) && brush_connect) state = 1;
			}
			//橡皮
			if (rubber.select == false && IsInRect(m.x, m.y, { 192 + 8, floating_windows.height - 156 + 8, 192 + 8 + 80, floating_windows.height - 156 + 8 + 80 }))
			{
				if (m.lbutton)
				{
					int lx = m.x, ly = m.y;
					while (1)
					{
						ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
						if (IsInRect(m.x, m.y, { 192 + 8, floating_windows.height - 156 + 8, 192 + 8 + 80, floating_windows.height - 156 + 8 + 80 }))
						{
							if (!m.lbutton)
							{
								state = 1;
								rubber.select = true;
								brush.select = false;
								choose.select = false;

								if (SeewoCamera) FreezeFrame.mode = 1;

								break;
							}
						}
						else
						{
							hiex::flushmessage_win32(EM_MOUSE, floating_window);

							break;
						}
					}
					hiex::flushmessage_win32(EM_MOUSE, floating_window);
				}
			}
			//恢复画板
			{
				if (choose.select == false && RecallImage.size() > 1 && IsInRect(m.x, m.y, { floating_windows.width - 96, floating_windows.height - 55, floating_windows.width - 96 + 96, floating_windows.height - 50 + 40 }))
				{
					if (m.lbutton)
					{
						int lx = m.x, ly = m.y;
						while (1)
						{
							ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
							if (IsInRect(m.x, m.y, { floating_windows.width - 96, floating_windows.height - 55, floating_windows.width - 96 + 96, floating_windows.height - 50 + 40 }))
							{
								if (!m.lbutton)
								{
									RecallImage.pop_back();
									deque<RecallStruct>(RecallImage).swap(RecallImage); // 使用swap技巧来释放未使用的内存

									if (!RecallImage.empty()) drawpad = RecallImage.back().img;
									else SetImageColor(drawpad, RGBA(0, 0, 0, 1), true);

									extreme_point = RecallImage.back().extreme_point;

									{
										// 设置BLENDFUNCTION结构体
										BLENDFUNCTION blend;
										blend.BlendOp = AC_SRC_OVER;
										blend.BlendFlags = 0;
										blend.SourceConstantAlpha = 255; // 设置透明度，0为全透明，255为不透明
										blend.AlphaFormat = AC_SRC_ALPHA; // 使用源图像的alpha通道
										HDC hdcScreen = GetDC(NULL);
										// 调用UpdateLayeredWindow函数更新窗口
										POINT ptSrc = { 0,0 };
										SIZE sizeWnd = { drawpad.getwidth(),drawpad.getheight() };
										POINT ptDst = { 0,0 }; // 设置窗口位置
										UPDATELAYEREDWINDOWINFO ulwi = { 0 };
										ulwi.cbSize = sizeof(ulwi);
										ulwi.hdcDst = hdcScreen;
										ulwi.pptDst = &ptDst;
										ulwi.psize = &sizeWnd;
										ulwi.pptSrc = &ptSrc;
										ulwi.crKey = RGB(255, 255, 255);
										ulwi.pblend = &blend;
										ulwi.dwFlags = ULW_ALPHA;

										// 定义要更新的矩形区域
										RECT rcDirty = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

										ulwi.hdcSrc = GetImageHDC(&drawpad);
										ulwi.prcDirty = &rcDirty;
										UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
									}

									break;
								}
							}
							else
							{
								hiex::flushmessage_win32(EM_MOUSE, floating_window);

								break;
							}
						}
						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
				if (choose.select == true && RestoreSketchpad && IsInRect(m.x, m.y, { floating_windows.width - 96, floating_windows.height - 55, floating_windows.width - 96 + 96, floating_windows.height - 50 + 40 }))
				{
					if (m.lbutton)
					{
						int lx = m.x, ly = m.y;
						while (1)
						{
							ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
							if (IsInRect(m.x, m.y, { floating_windows.width - 96, floating_windows.height - 55, floating_windows.width - 96 + 96, floating_windows.height - 50 + 40 }))
							{
								if (!m.lbutton)
								{
									empty_drawpad = false;
									brush.select = true;
									rubber.select = false;
									choose.select = false;

									break;
								}
							}
							else
							{
								hiex::flushmessage_win32(EM_MOUSE, floating_window);

								break;
							}
						}
						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
			}
			//选项
			if (IsInRect(m.x, m.y, { 288 + 8, floating_windows.height - 156 + 8, 288 + 8 + 80, floating_windows.height - 156 + 8 + 80 }))
			{
				if (m.lbutton)
				{
					int lx = m.x, ly = m.y;
					while (1)
					{
						ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
						if (IsInRect(m.x, m.y, { 288 + 8, floating_windows.height - 156 + 8, 288 + 8 + 80, floating_windows.height - 156 + 8 + 80 }))
						{
							if (!m.lbutton)
							{
								if (test.select) test.select = false;
								else test.select = true;

								break;
							}
						}
						else
						{
							hiex::flushmessage_win32(EM_MOUSE, floating_window);

							break;
						}
					}
					hiex::flushmessage_win32(EM_MOUSE, floating_window);
				}
			}
			//插件：随机点名
			if (plug_in_RandomRollCall.select == 2)
			{
				if (IsInRect(m.x, m.y, { 2, floating_windows.height - 55, 2 + 100, floating_windows.height - 55 + 40 }))
				{
					if (m.lbutton)
					{
						int lx = m.x, ly = m.y;
						while (1)
						{
							ExMessage m = hiex::getmessage_win32(EM_MOUSE, floating_window);
							if (IsInRect(m.x, m.y, { 2, floating_windows.height - 55, 2 + 100, floating_windows.height - 55 + 40 }))
							{
								if (!m.lbutton)
								{
									if (_waccess((string_to_wstring(global_path) + L"plug-in\\随机点名\\随机点名.exe").c_str(), 0) == 0 && !isProcessRunning((string_to_wstring(global_path) + L"plug-in\\随机点名\\随机点名.exe").c_str()))
									{
										STARTUPINFOA si = { 0 };
										si.cb = sizeof(si);
										PROCESS_INFORMATION pi = { 0 };
										CreateProcessA(NULL, (global_path + "plug-in\\随机点名\\随机点名.exe").data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
										CloseHandle(pi.hProcess);
										CloseHandle(pi.hThread);
									}

									break;
								}
							}
							else
							{
								hiex::flushmessage_win32(EM_MOUSE, floating_window);

								break;
							}
						}
						hiex::flushmessage_win32(EM_MOUSE, floating_window);
					}
				}
			}

			if (m.rbutton && IsInRect(m.x, m.y, { floating_windows.width - 96, floating_windows.height - 156, floating_windows.width, floating_windows.height }))
			{
				off_signal = true;
			}
		}
	}
}

int floating_main()
{
	thread_status[L"floating_main"] = true;
	GetLocalTime(&sys_time);

	thread ppt_state_thread(ppt_state);
	ppt_state_thread.detach();
	thread DrawControlWindow_thread(DrawControlWindow);
	DrawControlWindow_thread.detach();
	thread ControlManipulation_thread(ControlManipulation);
	ControlManipulation_thread.detach();
	//thread GetTime_thread(GetTime);
	//GetTime_thread.detach();
	thread DrawScreen_thread(DrawScreen);
	DrawScreen_thread.detach();

	thread black_block_thread(black_block);
	black_block_thread.detach();

	if (!debug)
	{
		if (_waccess((string_to_wstring(global_path) + L"api").c_str(), 0) == -1)
		{
			//创建路径
			filesystem::create_directory(string_to_wstring(global_path) + L"api");

			if (_waccess((string_to_wstring(global_path) + L"api\\智绘教CrashedHandler.exe").c_str(), 0) == -1 || _waccess((string_to_wstring(global_path) + L"api\\智绘教CrashedHandlerClose.exe").c_str(), 0) == -1)
			{
				ExtractResource((string_to_wstring(global_path) + L"api\\智绘教CrashedHandler.exe").c_str(), L"EXE", MAKEINTRESOURCE(201));
				ExtractResource((string_to_wstring(global_path) + L"api\\智绘教CrashedHandlerClose.exe").c_str(), L"EXE", MAKEINTRESOURCE(202));
			}
			//注册ppt
			{
				ExtractResource((string_to_wstring(global_path) + L"PptInterface.dll").c_str(), L"EXE", MAKEINTRESOURCE(203));
				ExtractResource((string_to_wstring(global_path) + L"PptInterface.tlb").c_str(), L"EXE", MAKEINTRESOURCE(204));

				//未完待续
			}
		}
		if (_waccess((string_to_wstring(global_path) + L"PptInterface.dll").c_str(), 0) == -1 || _waccess((string_to_wstring(global_path) + L"PptInterface.tlb").c_str(), 0) == -1)
		{
			ExtractResource((string_to_wstring(global_path) + L"PptInterface.dll").c_str(), L"EXE", MAKEINTRESOURCE(203));
			ExtractResource((string_to_wstring(global_path) + L"PptInterface.tlb").c_str(), L"EXE", MAKEINTRESOURCE(204));

			//未完待续
		}

		/*
		//注册icu
		if (_waccess((string_to_wstring(global_path) + L"icudt73.dll").c_str(), 0) == -1 || _waccess((string_to_wstring(global_path) + L"icuin73.dll").c_str(), 0) == -1 || _waccess((string_to_wstring(global_path) + L"icuuc73.dll").c_str(), 0) == -1)
		{
			ExtractResource((string_to_wstring(global_path) + L"icudt73.dll").c_str(), L"DLL", MAKEINTRESOURCE(207));
			ExtractResource((string_to_wstring(global_path) + L"icuin73.dll").c_str(), L"DLL", MAKEINTRESOURCE(208));
			ExtractResource((string_to_wstring(global_path) + L"icuuc73.dll").c_str(), L"DLL", MAKEINTRESOURCE(209));
		}
		*/

		thread CrashedHandler_thread(CrashedHandler);
		CrashedHandler_thread.detach();
		thread AutomaticUpdate_thread(AutomaticUpdate);
		AutomaticUpdate_thread.detach();
	}

	MouseInteraction();

	while (1)
	{
		if (!thread_status[L"CrashedHandler"] /*&& !thread_status[L"ppt_state"] && !thread_status[L"ControlManipulation"] */ && !thread_status[L"GetTime"] && !thread_status[L"DrawScreen"] && !thread_status[L"api_read_pipe"] && !thread_status[L"black_block"])
			break;
	}

	thread_status[L"floating_main"] = false;
	return 0;
}

// 画板部分 ==========

bool main_open, draw_content;

RECT DrawGradientLine(HDC hdc, int x1, int y1, int x2, int y2, float width, Color color)
{
	Graphics graphics(hdc);
	graphics.SetSmoothingMode(SmoothingModeHighQuality);

	Pen pen(color);
	pen.SetEndCap(LineCapRound);

	pen.SetWidth(width);
	graphics.DrawLine(&pen, x1, y1, x2, y2);

	// 计算外切矩形
	RECT rect;
	rect.left = max(0, min(x1, x2) - width);
	rect.top = max(0, min(y1, y2) - width);
	rect.right = max(x1, x2) + width;
	rect.bottom = max(y1, y2) + width;

	return rect;
}
bool checkIntersection(RECT rect1, RECT rect2)
{
	if (rect1.right < rect2.left || rect1.left > rect2.right) {
		return false;
	}
	if (rect1.bottom < rect2.top || rect1.top > rect2.bottom) {
		return false;
	}
	return true;
}
double EuclideanDistance(POINT a, POINT b)
{
	return std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2));
}

void removeEmptyFolders(std::wstring path)
{
	for (const auto& entry : filesystem::directory_iterator(path)) {
		if (entry.is_directory()) {
			removeEmptyFolders(entry.path());
			if (filesystem::is_empty(entry)) {
				filesystem::remove(entry);
			}
		}
	}
}
void removeUnknownFiles(std::wstring path, std::deque<std::wstring> knownFiles)
{
	for (const auto& entry : filesystem::recursive_directory_iterator(path)) {
		if (entry.is_regular_file()) {
			auto it = std::find(knownFiles.begin(), knownFiles.end(), entry.path().wstring());
			if (it == knownFiles.end()) {
				filesystem::remove(entry);
			}
		}
	}
}
deque<wstring> getPrevTwoDays(const std::wstring& date, int day = 7)
{
	deque<wstring> ret;

	std::wistringstream ss(date);
	std::tm t = {};
	ss >> std::get_time(&t, L"%Y-%m-%d");

	for (int i = 1; i <= day; i++)
	{
		std::mktime(&t);
		std::wostringstream os1;
		os1 << std::put_time(&t, L"%Y-%m-%d");
		ret.push_back(os1.str());

		t.tm_mday -= 1;
	}

	return ret;
}
//保存图像到指定目录
void SaveScreenShot(IMAGE img)
{
	wstring date = CurrentDate(), time = CurrentTime();

	if (_waccess((string_to_wstring(global_path) + L"ScreenShot").c_str(), 0 == -1)) CreateDirectory((string_to_wstring(global_path) + L"ScreenShot").c_str(), NULL);
	if (_waccess((string_to_wstring(global_path) + L"ScreenShot\\" + date).c_str(), 0 == -1)) CreateDirectory((string_to_wstring(global_path) + L"ScreenShot\\" + date).c_str(), NULL);

	/*
	//if (xkl_windows != drawpad_window) Test();
	//bool hide_xkl = (xkl_windows == drawpad_window ? false : true);
	//if (hide_xkl) ShowWindow(xkl_windows, SW_HIDE);
	ShowWindow(floating_window, SW_HIDE);

	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);
	IMAGE desktop(width, height);
	BitBlt(GetImageHDC(&desktop), 0, 0, width, height, GetDC(NULL), 0, 0, SRCCOPY);

	ShowWindow(floating_window, SW_SHOW);
	//if (hide_xkl) ShowWindow(xkl_windows, SW_SHOW);
	*/

	std::shared_lock<std::shared_mutex> lock1(MagnificationBackgroundSm);
	IMAGE blending = MagnificationBackground;
	lock1.unlock();

	saveImageToPNG(img, wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\" + date + L"\\" + time + L".png").c_str(), true);
	saveImageToPNG(blending, wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\" + date + L"\\" + time + L"_background.png").c_str());

	hiex::TransparentImage(&blending, 0, 0, &img);
	//saveImageToJPG(blending, wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\" + date + L"\\" + time + L"_blending.jpg").c_str(),50);
	saveimage((string_to_wstring(global_path) + L"ScreenShot\\" + date + L"\\" + time + L"_blending.jpg").c_str(), &blending);

	//图像目录书写
	if (_waccess((string_to_wstring(global_path) + L"ScreenShot\\attribute_directory.json").c_str(), 4) == 0)
	{
		Json::Reader reader;
		Json::Value root;

		ifstream readjson;
		readjson.imbue(locale("zh_CN.UTF8"));
		readjson.open(wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\attribute_directory.json").c_str());

		if (reader.parse(readjson, root))
		{
			readjson.close();

			Json::StyledWriter outjson;
			Json::Value set;
			set["date"] = Json::Value(convert_to_utf8(wstring_to_string(date)));
			set["time"] = Json::Value(convert_to_utf8(wstring_to_string(time)));
			set["drawpad"] = Json::Value(convert_to_utf8(wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\" + date + L"\\" + time + L".png")));
			set["background"] = Json::Value(convert_to_utf8(wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\" + date + L"\\" + time + L"_background.png")));
			set["blending"] = Json::Value(convert_to_utf8(wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\" + date + L"\\" + time + L"_blending.jpg")));
			root["Image_Properties"].insert(0, set);

			Json::StreamWriterBuilder OutjsonBuilder;
			OutjsonBuilder.settings_["emitUTF8"] = true;
			std::unique_ptr<Json::StreamWriter> writer(OutjsonBuilder.newStreamWriter());
			ofstream writejson;
			writejson.imbue(locale("zh_CN.UTF8"));
			writejson.open(wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\attribute_directory.json").c_str());
			writer->write(root, &writejson);
			writejson.close();
		}
		else readjson.close();
	}
	else
	{
		Json::StyledWriter outjson;
		Json::Value root;
		Json::Value set;
		set["date"] = Json::Value(convert_to_utf8(wstring_to_string(date)));
		set["time"] = Json::Value(convert_to_utf8(wstring_to_string(time)));
		set["drawpad"] = Json::Value(convert_to_utf8(wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\" + date + L"\\" + time + L".png")));
		set["background"] = Json::Value(convert_to_utf8(wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\" + date + L"\\" + time + L"_background.png")));
		set["blending"] = Json::Value(convert_to_utf8(wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\" + date + L"\\" + time + L"_blending.jpg")));
		root["Image_Properties"].append(set);

		Json::StreamWriterBuilder OutjsonBuilder;
		OutjsonBuilder.settings_["emitUTF8"] = true;
		std::unique_ptr<Json::StreamWriter> writer(OutjsonBuilder.newStreamWriter());
		ofstream writejson;
		writejson.imbue(locale("zh_CN.UTF8"));
		writejson.open(wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\attribute_directory.json").c_str());
		writer->write(root, &writejson);
		writejson.close();
	}
}

//ppt 控件
void DrawControlWindow()
{
	thread_status[L"DrawControlWindow"] = true;

	IMAGE ppt_background = CreateImageColor(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), RGBA(0, 0, 0, 0), true);
	Graphics graphics(GetImageHDC(&ppt_background));
	//ppt窗口初始化
	{
		DisableResizing(ppt_window, true);//禁止窗口拉伸
		SetWindowLong(ppt_window, GWL_STYLE, GetWindowLong(ppt_window, GWL_STYLE) & ~WS_CAPTION);//隐藏标题栏
		SetWindowPos(ppt_window, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_DRAWFRAME | SWP_NOACTIVATE);
		SetWindowLong(ppt_window, GWL_EXSTYLE, WS_EX_TOOLWINDOW);//隐藏任务栏
	}

	// 设置BLENDFUNCTION结构体
	BLENDFUNCTION blend;
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.SourceConstantAlpha = 255; // 设置透明度，0为全透明，255为不透明
	blend.AlphaFormat = AC_SRC_ALPHA; // 使用源图像的alpha通道
	HDC hdcScreen = GetDC(NULL);
	// 调用UpdateLayeredWindow函数更新窗口
	POINT ptSrc = { 0,0 };
	SIZE sizeWnd = { ppt_background.getwidth(),ppt_background.getheight() };
	POINT ptDst = { 0,0 }; // 设置窗口位置
	UPDATELAYEREDWINDOWINFO ulwi = { 0 };
	ulwi.cbSize = sizeof(ulwi);
	ulwi.hdcDst = hdcScreen;
	ulwi.pptDst = &ptDst;
	ulwi.psize = &sizeWnd;
	ulwi.pptSrc = &ptSrc;
	ulwi.crKey = RGB(255, 255, 255);
	ulwi.pblend = &blend;
	ulwi.dwFlags = ULW_ALPHA;
	LONG nRet = ::GetWindowLong(ppt_window, GWL_EXSTYLE);
	nRet |= WS_EX_LAYERED;
	::SetWindowLong(ppt_window, GWL_EXSTYLE, nRet);

	magnificationWindowReady++;
	for (int for_i = 1; !off_signal; for_i = 2)
	{
		int currentSlides = ppt_info.currentSlides;
		int totalSlides = ppt_info.totalSlides;

		if (totalSlides != -1)
		{
			SetImageColor(ppt_background, RGBA(0, 0, 0, 0), true);

			//左侧翻页
			{
				hiex::EasyX_Gdiplus_FillRoundRect(5, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, 50, 50, 20, 20, SET_ALPHA(RGB(100, 100, 100), 255), SET_ALPHA(WHITE, 160), 1, true, SmoothingModeHighQuality, &ppt_background);
				hiex::TransparentImage(&ppt_background, 5 + 5, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 5, &ppt_icon[1]);

				hiex::EasyX_Gdiplus_FillRoundRect(55 + 5, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, 70, 50, 20, 20, SET_ALPHA(RGB(100, 100, 100), 255), SET_ALPHA(WHITE, 160), 1, true, SmoothingModeHighQuality, &ppt_background);
				{
					wstring text = to_wstring(currentSlides == -1 ? totalSlides : currentSlides);
					text += L"/";
					text += to_wstring(totalSlides);

					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, text.length() > 5 ? 16 : (text.length() > 3 ? 20 : 24), FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(100, 100, 100), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						pptwords_rect.left = (55 + 5);
						pptwords_rect.top = (GetSystemMetrics(SM_CYSCREEN) - 60 + 5) + 13;
						pptwords_rect.right = (55 + 5) + 70;
						pptwords_rect.bottom = (GetSystemMetrics(SM_CYSCREEN) - 60 + 5) + 40;
					}

					graphics.DrawString(text.c_str(), -1, &gp_font, hiex::RECTToRectF(pptwords_rect), &stringFormat, &WordBrush);
				}

				hiex::EasyX_Gdiplus_FillRoundRect(130 + 5, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, 50, 50, 20, 20, SET_ALPHA(RGB(100, 100, 100), 255), SET_ALPHA(WHITE, 160), 1, true, SmoothingModeHighQuality, &ppt_background);
				hiex::TransparentImage(&ppt_background, 130 + 5 + 5, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 5, &ppt_icon[2]);
			}
			//中间控件
			{
				hiex::EasyX_Gdiplus_FillRoundRect(GetSystemMetrics(SM_CXSCREEN) / 2 - 70, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, 140, 50, 20, 20, SET_ALPHA(RGB(100, 100, 100), 255), SET_ALPHA(WHITE, 160), 1, true, SmoothingModeHighQuality, &ppt_background);
				{
					hiex::TransparentImage(&ppt_background, GetSystemMetrics(SM_CXSCREEN) / 2 - 70 + 10, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 10, &ppt_icon[3]);

					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(100, 100, 100), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						pptwords_rect.left = (GetSystemMetrics(SM_CXSCREEN) / 2 - 70) + 40;
						pptwords_rect.top = (GetSystemMetrics(SM_CYSCREEN) - 60 + 5) + 13;
						pptwords_rect.right = (GetSystemMetrics(SM_CXSCREEN) / 2 - 70) + 140;
						pptwords_rect.bottom = (GetSystemMetrics(SM_CYSCREEN) - 60 + 5) + 40;
					}

					graphics.DrawString(L"结束放映", -1, &gp_font, hiex::RECTToRectF(pptwords_rect), &stringFormat, &WordBrush);
				}
			}
			//右侧翻页
			{
				hiex::EasyX_Gdiplus_FillRoundRect(GetSystemMetrics(SM_CXSCREEN) - 185, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, 50, 50, 20, 20, SET_ALPHA(RGB(130, 130, 130), 255), SET_ALPHA(WHITE, 160), 1, true, SmoothingModeHighQuality, &ppt_background);
				hiex::TransparentImage(&ppt_background, GetSystemMetrics(SM_CXSCREEN) - 185 + 5, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 5, &ppt_icon[1]);

				hiex::EasyX_Gdiplus_FillRoundRect(GetSystemMetrics(SM_CXSCREEN) - 130, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, 70, 50, 20, 20, SET_ALPHA(RGB(130, 130, 130), 255), SET_ALPHA(WHITE, 160), 1, true, SmoothingModeHighQuality, &ppt_background);
				{
					wstring text = to_wstring(currentSlides == -1 ? totalSlides : currentSlides);
					text += L"/";
					text += to_wstring(totalSlides);

					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, text.length() > 5 ? 16 : (text.length() > 3 ? 20 : 24), FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(100, 100, 100), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						pptwords_rect.left = (GetSystemMetrics(SM_CXSCREEN) - 130);
						pptwords_rect.top = (GetSystemMetrics(SM_CYSCREEN) - 60 + 5) + 13;
						pptwords_rect.right = (GetSystemMetrics(SM_CXSCREEN) - 130) + 70;
						pptwords_rect.bottom = (GetSystemMetrics(SM_CYSCREEN) - 60 + 5) + 40;
					}

					graphics.DrawString(text.c_str(), -1, &gp_font, hiex::RECTToRectF(pptwords_rect), &stringFormat, &WordBrush);
				}

				hiex::EasyX_Gdiplus_FillRoundRect(GetSystemMetrics(SM_CXSCREEN) - 55, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, 50, 50, 20, 20, SET_ALPHA(RGB(130, 130, 130), 255), SET_ALPHA(WHITE, 160), 1, true, SmoothingModeHighQuality, &ppt_background);
				hiex::TransparentImage(&ppt_background, GetSystemMetrics(SM_CXSCREEN) - 55 + 5, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 5, &ppt_icon[2]);
			}

			{
				ulwi.hdcSrc = GetImageHDC(&ppt_background);
				UpdateLayeredWindowIndirect(ppt_window, &ulwi);
			}

			SetWindowPos(ppt_window, floating_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			if (!IsWindowVisible(ppt_window))
			{
				ShowWindow(ppt_window, SW_SHOWNOACTIVATE);
				ppt_show = GetPptShow();

				FreezePPT = true;
			}
		}
		else
		{
			if (IsWindowVisible(ppt_window))
			{
				ppt_img.is_save = false;
				ppt_img.is_saved.clear();
				ppt_img.image.clear();

				ShowWindow(ppt_window, SW_HIDE);
				ppt_show = NULL;

				FreezePPT = false;
			}
		}

		Sleep(100);
	}

	thread_status[L"DrawControlWindow"] = false;
}
void ControlManipulation()
{
	ExMessage m;
	while (!off_signal)
	{
		if (ppt_info.totalSlides != -1)
		{
			hiex::getmessage_win32(&m, EM_MOUSE, ppt_window);

			//左侧
			{
				//上一页
				if (IsInRect(m.x, m.y, { 5, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, 5 + 50, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 50 }))
				{
					if (m.lbutton)
					{
						int lx = m.x, ly = m.y;
						while (1)
						{
							ExMessage m = hiex::getmessage_win32(EM_MOUSE, ppt_window);
							if (IsInRect(m.x, m.y, { 5, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, 5 + 50, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 50 }))
							{
								if (!m.lbutton)
								{
									SetForegroundWindow(ppt_show);
									keybd_event(VK_LEFT, 0, 0, 0);
									keybd_event(VK_LEFT, 0, KEYEVENTF_KEYUP, 0);

									break;
								}
							}
							else
							{
								hiex::flushmessage_win32(EM_MOUSE, ppt_window);

								break;
							}
						}
						hiex::flushmessage_win32(EM_MOUSE, ppt_window);
					}
				}

				//下一页
				if (IsInRect(m.x, m.y, { 130 + 5, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, 130 + 5 + 50, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 50 }))
				{
					if (m.lbutton)
					{
						int lx = m.x, ly = m.y;
						while (1)
						{
							ExMessage m = hiex::getmessage_win32(EM_MOUSE, ppt_window);
							if (IsInRect(m.x, m.y, { 130 + 5, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, 130 + 5 + 50, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 50 }))
							{
								if (!m.lbutton)
								{
									SetForegroundWindow(ppt_show);
									keybd_event(VK_RIGHT, 0, 0, 0);
									keybd_event(VK_RIGHT, 0, KEYEVENTF_KEYUP, 0);

									break;
								}
							}
							else
							{
								hiex::flushmessage_win32(EM_MOUSE, ppt_window);

								break;
							}
						}
						hiex::flushmessage_win32(EM_MOUSE, ppt_window);
					}
				}
			}
			//中间
			{
				//结束放映
				if (IsInRect(m.x, m.y, { GetSystemMetrics(SM_CXSCREEN) / 2 - 70, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, GetSystemMetrics(SM_CXSCREEN) / 2 - 70 + 140, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 50 }))
				{
					if (m.lbutton)
					{
						int lx = m.x, ly = m.y;
						while (1)
						{
							ExMessage m = hiex::getmessage_win32(EM_MOUSE, ppt_window);
							if (IsInRect(m.x, m.y, { GetSystemMetrics(SM_CXSCREEN) / 2 - 70, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, GetSystemMetrics(SM_CXSCREEN) / 2 - 70 + 140, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 50 }))
							{
								if (!m.lbutton)
								{
									EndPptShow();

									break;
								}
							}
							else
							{
								hiex::flushmessage_win32(EM_MOUSE, ppt_window);

								break;
							}
						}
						hiex::flushmessage_win32(EM_MOUSE, ppt_window);
					}
				}
			}
			//右侧
			{
				//上一页
				if (IsInRect(m.x, m.y, { GetSystemMetrics(SM_CXSCREEN) - 185, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, GetSystemMetrics(SM_CXSCREEN) - 185 + 50, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 50 }))
				{
					if (m.lbutton)
					{
						int lx = m.x, ly = m.y;
						while (1)
						{
							ExMessage m = hiex::getmessage_win32(EM_MOUSE, ppt_window);
							if (IsInRect(m.x, m.y, { GetSystemMetrics(SM_CXSCREEN) - 185, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, GetSystemMetrics(SM_CXSCREEN) - 185 + 50, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 50 }))
							{
								if (!m.lbutton)
								{
									SetForegroundWindow(ppt_show);
									keybd_event(VK_LEFT, 0, 0, 0);
									keybd_event(VK_LEFT, 0, KEYEVENTF_KEYUP, 0);

									break;
								}
							}
							else
							{
								hiex::flushmessage_win32(EM_MOUSE, ppt_window);

								break;
							}
						}
						hiex::flushmessage_win32(EM_MOUSE, ppt_window);
					}
				}

				//下一页
				if (IsInRect(m.x, m.y, { GetSystemMetrics(SM_CXSCREEN) - 55, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, GetSystemMetrics(SM_CXSCREEN) - 55 + 50, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 50 }))
				{
					if (m.lbutton)
					{
						int lx = m.x, ly = m.y;
						while (1)
						{
							ExMessage m = hiex::getmessage_win32(EM_MOUSE, ppt_window);
							if (IsInRect(m.x, m.y, { GetSystemMetrics(SM_CXSCREEN) - 55, GetSystemMetrics(SM_CYSCREEN) - 60 + 5, GetSystemMetrics(SM_CXSCREEN) - 55 + 50, GetSystemMetrics(SM_CYSCREEN) - 60 + 5 + 50 }))
							{
								if (!m.lbutton)
								{
									SetForegroundWindow(ppt_show);
									keybd_event(VK_RIGHT, 0, 0, 0);
									keybd_event(VK_RIGHT, 0, KEYEVENTF_KEYUP, 0);

									break;
								}
							}
							else
							{
								hiex::flushmessage_win32(EM_MOUSE, ppt_window);

								break;
							}
						}
						hiex::flushmessage_win32(EM_MOUSE, ppt_window);
					}
				}
			}
		}
		else Sleep(10);
	}
}

int TestMainMode = 1;
void ControlTestMain()
{
	ExMessage m;
	while (!off_signal)
	{
		hiex::getmessage_win32(&m, EM_MOUSE, test_window);

		if (TestMainMode == 1 && IsInRect(m.x, m.y, { 220, 625, 980, 675 }))
		{
			if (m.lbutton)
			{
				int lx = m.x, ly = m.y;
				while (1)
				{
					ExMessage m = hiex::getmessage_win32(EM_MOUSE, test_window);
					if (IsInRect(m.x, m.y, { 20, 625, 780, 675 }))
					{
						if (!m.lbutton)
						{
							if (setlist.experimental_functions) setlist.experimental_functions = false;
							else setlist.experimental_functions = true;

							{
								Json::StyledWriter outjson;
								Json::Value root;

								root["edition"] = Json::Value(edition_date);
								root["startup"] = Json::Value(setlist.startup);
								root["experimental_functions"] = Json::Value(setlist.experimental_functions);

								ofstream writejson;
								writejson.imbue(locale("zh_CN.UTF8"));
								writejson.open(wstring_to_string(string_to_wstring(global_path) + L"opt\\deploy.json").c_str());
								writejson << outjson.write(root);
								writejson.close();
							}

							break;
						}
					}
					else
					{
						hiex::flushmessage_win32(EM_MOUSE, test_window);

						break;
					}
				}
				hiex::flushmessage_win32(EM_MOUSE, test_window);
			}
		}

		else if (IsInRect(m.x, m.y, { 10, 15, 190, 75 }))
		{
			if (m.lbutton)
			{
				int lx = m.x, ly = m.y;
				while (1)
				{
					ExMessage m = hiex::getmessage_win32(EM_MOUSE, test_window);
					if (IsInRect(m.x, m.y, { 10, 15, 190, 75 }))
					{
						if (!m.lbutton)
						{
							TestMainMode = 1;

							break;
						}
					}
					else
					{
						hiex::flushmessage_win32(EM_MOUSE, test_window);

						break;
					}
				}
				hiex::flushmessage_win32(EM_MOUSE, test_window);
			}
		}
		else if (IsInRect(m.x, m.y, { 10, 85, 190, 145 }))
		{
			if (m.lbutton)
			{
				int lx = m.x, ly = m.y;
				while (1)
				{
					ExMessage m = hiex::getmessage_win32(EM_MOUSE, test_window);
					if (IsInRect(m.x, m.y, { 10, 85, 190, 145 }))
					{
						if (!m.lbutton)
						{
							TestMainMode = 2;

							break;
						}
					}
					else
					{
						hiex::flushmessage_win32(EM_MOUSE, test_window);

						break;
					}
				}
				hiex::flushmessage_win32(EM_MOUSE, test_window);
			}
		}
		else if (IsInRect(m.x, m.y, { 10, 155, 190, 215 }))
		{
			if (m.lbutton)
			{
				int lx = m.x, ly = m.y;
				while (1)
				{
					ExMessage m = hiex::getmessage_win32(EM_MOUSE, test_window);
					if (IsInRect(m.x, m.y, { 10, 155, 190, 215 }))
					{
						if (!m.lbutton)
						{
							TestMainMode = 3;

							break;
						}
					}
					else
					{
						hiex::flushmessage_win32(EM_MOUSE, test_window);

						break;
					}
				}
				hiex::flushmessage_win32(EM_MOUSE, test_window);
			}
		}
	}
}

LRESULT CALLBACK TestWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
		return 0;

	default:
		return HIWINDOW_DEFAULT_PROC;
	}
	return 0;
}
int test_main()
{
	this_thread::sleep_for(chrono::seconds(3));
	while (!already) this_thread::sleep_for(chrono::milliseconds(50));

	thread_status[L"test_main"] = true;

	loadimage(&test_sign[1], L"PNG", L"sign2");
	loadimage(&test_sign[2], L"PNG", L"sign3");
	loadimage(&test_sign[3], L"PNG", L"sign4");

	IMAGE test_icon[5];
	loadimage(&test_icon[1], L"PNG", L"test_icon1");
	loadimage(&test_icon[2], L"PNG", L"test_icon2");
	loadimage(&test_icon[3], L"PNG", L"test_icon3");

	DisableResizing(test_window, true);//禁止窗口拉伸
	DisableSystemMenu(test_window, true);

	LONG style = GetWindowLong(test_window, GWL_STYLE);
	style &= ~WS_SYSMENU;
	SetWindowLong(test_window, GWL_STYLE, style);

	hiex::SetWndProcFunc(test_window, TestWndProc);

	// 设置BLENDFUNCTION结构体
	BLENDFUNCTION blend;
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.SourceConstantAlpha = 255; // 设置透明度，0为全透明，255为不透明
	blend.AlphaFormat = AC_SRC_ALPHA; // 使用源图像的alpha通道
	HDC hdcScreen = GetDC(NULL);
	// 调用UpdateLayeredWindow函数更新窗口
	POINT ptSrc = { 0,0 };
	SIZE sizeWnd = { 1010, 750 };
	POINT ptDst = { 0,0 }; // 设置窗口位置
	UPDATELAYEREDWINDOWINFO ulwi = { 0 };
	ulwi.cbSize = sizeof(ulwi);
	ulwi.hdcDst = hdcScreen;
	ulwi.pptDst = &ptDst;
	ulwi.psize = &sizeWnd;
	ulwi.pptSrc = &ptSrc;
	ulwi.crKey = RGB(255, 255, 255);
	ulwi.pblend = &blend;
	ulwi.dwFlags = ULW_ALPHA;
	LONG nRet = ::GetWindowLong(test_window, GWL_EXSTYLE);
	nRet |= WS_EX_LAYERED;
	::SetWindowLong(test_window, GWL_EXSTYLE, nRet);

	thread ControlTestMain_thread(ControlTestMain);
	ControlTestMain_thread.detach();

	POINT pt;
	IMAGE test_title(1010, 750), test_background(1010, 710), test_content(800, 700);

	magnificationWindowReady++;
	while (!off_signal)
	{
		Sleep(500);

		if (test.select == true)
		{
			wstring ppt_LinkTest = LinkTest();
			wstring ppt_IsPptDependencyLoaded = IsPptDependencyLoaded();
			Graphics graphics(GetImageHDC(&test_content));

			if (!IsWindowVisible(test_window)) ShowWindow(test_window, SW_SHOW);

			while (!off_signal)
			{
				if (!choose.select) SetWindowPos(test_window, drawpad_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				else if (ppt_show != NULL) SetWindowPos(test_window, ppt_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				else SetWindowPos(test_window, floating_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

				SetImageColor(test_background, RGBA(0, 0, 0, 0), true);
				hiex::EasyX_Gdiplus_SolidRoundRect(0, 0, 1000, 700, 20, 20, RGBA(245, 245, 245, 255), false, SmoothingModeHighQuality, &test_background);

				if (TestMainMode == 1)
				{
					SetImageColor(test_content, RGBA(0, 0, 0, 0), true);
					hiex::EasyX_Gdiplus_SolidRoundRect(0, 0, 800, 700, 20, 20, RGBA(255, 255, 255, 255), false, SmoothingModeHighQuality, &test_content);

					{
						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(0, 0, 0, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 0;
							dwords_rect.top = 0;
							dwords_rect.right = 800;
							dwords_rect.bottom = 30;
						}
						graphics.DrawString(L"关闭此页面需再次点击 选项 按钮", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}

					hiex::TransparentImage(&test_content, 50, 140, &test_sign[2]);

					if (!server_feedback.empty() && server_feedback != "")
					{
						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(0, 0, 0, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 0;
							dwords_rect.top = 480;
							dwords_rect.right = 800;
							dwords_rect.bottom = 560;
						}
						graphics.DrawString(string_to_wstring(server_feedback).c_str(), -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}

					if (AutomaticUpdateStep == 0)
					{
						hiex::EasyX_Gdiplus_RoundRect(20, 560, 760, 50, 20, 20, RGBA(130, 130, 130, 255), 2, false, SmoothingModeHighQuality, &test_content);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 25, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(130, 130, 130, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 20;
							dwords_rect.top = 560;
							dwords_rect.right = 20 + 760;
							dwords_rect.bottom = 560 + 50;
						}
						graphics.DrawString(L"程序自动更新待启用", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}
					else if (AutomaticUpdateStep == 1)
					{
						hiex::EasyX_Gdiplus_RoundRect(20, 560, 760, 50, 20, 20, RGBA(106, 156, 45, 255), 2, false, SmoothingModeHighQuality, &test_content);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 25, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(106, 156, 45, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 20;
							dwords_rect.top = 560;
							dwords_rect.right = 20 + 760;
							dwords_rect.bottom = 560 + 50;
						}
						graphics.DrawString(L"程序版本已经是最新", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}
					else if (AutomaticUpdateStep == 2)
					{
						hiex::EasyX_Gdiplus_RoundRect(20, 560, 760, 50, 20, 20, RGBA(245, 166, 35, 255), 2, false, SmoothingModeHighQuality, &test_content);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 25, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(245, 166, 35, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 20;
							dwords_rect.top = 560;
							dwords_rect.right = 20 + 760;
							dwords_rect.bottom = 560 + 50;
						}
						graphics.DrawString(L"新版本排队下载中", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}
					else if (AutomaticUpdateStep == 3)
					{
						hiex::EasyX_Gdiplus_RoundRect(20, 560, 760, 50, 20, 20, RGBA(106, 156, 45, 255), 2, false, SmoothingModeHighQuality, &test_content);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 25, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(106, 156, 45, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 20;
							dwords_rect.top = 560;
							dwords_rect.right = 20 + 760;
							dwords_rect.bottom = 560 + 50;
						}
						graphics.DrawString(L"重启软件以更新到最新版本", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}

					if (setlist.experimental_functions)
					{
						hiex::EasyX_Gdiplus_RoundRect(20, 620, 760, 50, 20, 20, RGBA(0, 111, 225, 255), 2, false, SmoothingModeHighQuality, &test_content);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 25, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(0, 111, 225, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 20;
							dwords_rect.top = 620;
							dwords_rect.right = 20 + 760;
							dwords_rect.bottom = 620 + 50;
						}
						graphics.DrawString(L"程序实验性功能 已启用（点击禁用）", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}
					else
					{
						hiex::EasyX_Gdiplus_RoundRect(20, 620, 760, 50, 20, 20, RGBA(130, 130, 130, 255), 2, false, SmoothingModeHighQuality, &test_content);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 25, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(130, 130, 130, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 20;
							dwords_rect.top = 620;
							dwords_rect.right = 20 + 760;
							dwords_rect.bottom = 620 + 50;
						}
						graphics.DrawString(L"程序实验性功能 已禁用（点击启用）", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}
				}
				if (TestMainMode == 2)
				{
					SetImageColor(test_content, RGBA(0, 0, 0, 0), true);
					hiex::EasyX_Gdiplus_SolidRoundRect(0, 0, 800, 700, 20, 20, RGBA(255, 255, 255, 255), false, SmoothingModeHighQuality, &test_content);

					{
						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(0, 0, 0, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 0;
							dwords_rect.top = 0;
							dwords_rect.right = 800;
							dwords_rect.bottom = 30;
						}
						graphics.DrawString(L"关闭此页面需再次点击 选项 按钮", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}

					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(0, 0, 0), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						dwords_rect.left = 0;
						dwords_rect.top = 0;
						dwords_rect.right = 800;
						dwords_rect.bottom = 700;
					}

					GetCursorPos(&pt);

					wstring text = L"\n\n鼠标左键按下：";
					text += KEY_DOWN(VK_LBUTTON) ? L"是" : L"否";
					text += L"\n光标坐标 " + to_wstring(pt.x) + L"," + to_wstring(pt.y);

					text += L"\n\n此程序使用 RealTimeStylus 触控库（测试触控需进入画笔模式）";
					text += L"\n其兼容 WinXP 及以上版本的系统，效率较高，支持多点触控";

					if (uRealTimeStylus == 2) text += L"\n\nRealTimeStylus 消息：按下";
					else if (uRealTimeStylus == 3) text += L"\n\nRealTimeStylus 消息：抬起";
					else if (uRealTimeStylus == 4) text += L"\n\nRealTimeStylus 消息：移动";
					else text += L"\n\nRealTimeStylus 消息：就绪";

					text += L"\n触控按下：";
					text += touchDown ? L"是" : L"否";
					text += L"\n按下触控点数量：";
					text += to_wstring(touchNum);

					for (int i = 0; i < touchNum; i++)
					{
						std::shared_lock<std::shared_mutex> lock1(PointPosSm);
						POINT pt = TouchPos[TouchList[i]].pt;
						lock1.unlock();

						std::shared_lock<std::shared_mutex> lock2(TouchSpeedSm);
						double speed = TouchSpeed[TouchList[i]];
						lock2.unlock();

						text += L"\n触控点" + to_wstring(i + 1) + L" pid" + to_wstring(TouchList[i]) + L" 坐标" + to_wstring(pt.x) + L"," + to_wstring(pt.y) + L" 速度" + to_wstring(speed);
					}

					text += L"\n\nTouchList ";
					for (const auto& val : TouchList)
					{
						text += to_wstring(val) + L" ";
					}
					text += L"\nTouchTemp ";
					for (size_t i = 0; i < TouchTemp.size(); ++i)
					{
						text += to_wstring(TouchTemp[i].pid) + L" ";
					}

					text += L"\n\n撤回库当前大小：" + to_wstring(max(0, int(RecallImage.size()) - 1));

					text += L"\n\nCOM二进制接口 联动组件 状态：\n";
					text += ppt_LinkTest;
					text += L"\nPPT 联动组件状态：";
					text += ppt_IsPptDependencyLoaded;

					text += L"\n\nPPT 状态：";
					text += ppt_info_stay.TotalPage != -1 ? L"正在播放" : L"未播放";
					text += L"\nPPT 总页面数：";
					text += to_wstring(ppt_info_stay.TotalPage);
					text += L"\nPPT 当前页序号：";
					text += to_wstring(ppt_info_stay.CurrentPage);

					graphics.DrawString(text.c_str(), -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
				}
				if (TestMainMode == 3)
				{
					SetImageColor(test_content, RGBA(0, 0, 0, 0), true);
					hiex::EasyX_Gdiplus_SolidRoundRect(0, 0, 800, 700, 20, 20, RGBA(255, 255, 255, 255), false, SmoothingModeHighQuality, &test_content);

					hiex::TransparentImage(&test_content, 100, 20, &test_sign[1]);
					{
						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 20, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(0, 0, 0, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 0;
							dwords_rect.top = 0;
							dwords_rect.right = 800;
							dwords_rect.bottom = 30;
						}
						graphics.DrawString(L"关闭此页面需再次点击 选项 按钮", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}

					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 18, FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGB(0, 0, 0), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						dwords_rect.left = 0;
						dwords_rect.top = 305;
						dwords_rect.right = 800;
						dwords_rect.bottom = 700;
					}

					wstring text = L"程序版本：" + string_to_wstring(edition_code);
					text += L"\n程序发布版本：" + string_to_wstring(edition_date) + L" 测试分支（将于 10月20日 前切换到默认分支）";
					text += L"\n程序构建时间：" + buildTime;
					text += L"\n用户ID：" + userid;

					text += L"\n\n更新通道：LTS";
					if (!server_code.empty() && server_code != "")
					{
						text += L"\n联网版本代号：" + string_to_wstring(server_code);
						if (server_code == "GWSR") text += L" （广外专用版本）";
					}
					if (procedure_updata_error == 1) text += L"\n程序自动更新：已启用";
					else if (procedure_updata_error == 2) text += L"\n程序自动更新：发生网络错误";
					else text += L"\n程序自动更新：载入中（等待服务器反馈）";

					if (!server_feedback.empty() && server_feedback != "") text += L"\n服务器反馈信息：" + string_to_wstring(server_feedback);
					if (server_updata_error)
					{
						text += L"\n\n服务器通信错误：Error" + to_wstring(server_updata_error);
						if (!server_updata_error_reason.empty()) text += L"\n" + server_updata_error_reason;
					}

					/*
					GetCursorPos(&pt);

					text += L"\n\n鼠标左键按下：";
					text += KEY_DOWN(VK_LBUTTON) ? L"是" : L"否";
					text += L"\n光标坐标 " + to_wstring(pt.x) + L"," + to_wstring(pt.y);

					text += L"\n\n此程序使用 RealTimeStylus 触控库（测试触控需进入画笔模式）";
					text += L"\n其兼容 WinXP 及以上版本的系统，效率较高，支持多点触控";

					if (uRealTimeStylus == 2) text += L"\n\nRealTimeStylus 消息：按下";
					else if (uRealTimeStylus == 3) text += L"\n\nRealTimeStylus 消息：抬起";
					else if (uRealTimeStylus == 4) text += L"\n\nRealTimeStylus 消息：移动";
					else text += L"\n\nRealTimeStylus 消息：就绪";

					text += L"\n触控按下：";
					text += touchDown ? L"是" : L"否";
					text += L"\n按下触控点数量：";
					text += to_wstring(touchNum);

					for (int i = 0; i < 20; i++)
					{
						if (touchNum > i)
						{
							pt = TouchPos[TouchList[i]].pt;
							text += L"\n触控点" + to_wstring(i + 1) + L" pid" + to_wstring(TouchList[i]) + L" 坐标 " + to_wstring(pt.x) + L"," + to_wstring(pt.y);
						}
						else break;
					}

					text += L"\n\nTouchList ";
					for (const auto& val : TouchList)
					{
						text += to_wstring(val) + L" ";
					}
					text += L"\nTouchTemp ";
					for (size_t i = 0; i < TouchTemp.size(); ++i)
					{
						text += to_wstring(TouchTemp[i].pid) + L" ";
					}

					text += L"\n\n撤回库当前大小：" + to_wstring(max(0, int(RecallImage.size()) - 1));

					text += L"\n\nCOM二进制接口 联动组件 状态：\n";
					text += ppt_LinkTest;
					text += L"\nPPT 联动组件状态：";
					text += ppt_IsPptDependencyLoaded;

					text += L"\n\nPPT 状态：";
					text += ppt_info_stay.TotalPage != -1 ? L"正在播放" : L"未播放";
					text += L"\nPPT 总页面数：";
					text += to_wstring(ppt_info_stay.TotalPage);
					text += L"\nPPT 当前页序号：";
					text += to_wstring(ppt_info_stay.CurrentPage);
					*/

					graphics.DrawString(text.c_str(), -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
				}

				//侧栏
				{
					Graphics graphics(GetImageHDC(&test_background));

					if (TestMainMode == 1)
					{
						hiex::EasyX_Gdiplus_FillRoundRect(10, 10, 180, 60, 20, 20, RGBA(0, 111, 225, 255), RGBA(230, 230, 230, 255), 3, false, SmoothingModeHighQuality, &test_background);

						ChangeColor(test_icon[1], RGBA(0, 111, 225, 255));
						hiex::TransparentImage(&test_background, 20, 20, &test_icon[1]);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 24, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(0, 111, 225, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 70;
							dwords_rect.top = 10;
							dwords_rect.right = 70 + 110;
							dwords_rect.bottom = 10 + 63;
						}
						graphics.DrawString(L"主页", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}
					else
					{
						hiex::EasyX_Gdiplus_FillRoundRect(10, 10, 180, 60, 20, 20, RGBA(230, 230, 230, 255), RGBA(230, 230, 230, 255), 3, false, SmoothingModeHighQuality, &test_background);

						ChangeColor(test_icon[1], RGBA(130, 130, 130, 255));
						hiex::TransparentImage(&test_background, 20, 20, &test_icon[1]);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 24, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(130, 130, 130, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 70;
							dwords_rect.top = 10;
							dwords_rect.right = 70 + 110;
							dwords_rect.bottom = 10 + 63;
						}
						graphics.DrawString(L"主页", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}

					if (TestMainMode == 2)
					{
						hiex::EasyX_Gdiplus_FillRoundRect(10, 80, 180, 60, 20, 20, RGBA(0, 111, 225, 255), RGBA(230, 230, 230, 255), 3, false, SmoothingModeHighQuality, &test_background);

						ChangeColor(test_icon[2], RGBA(0, 111, 225, 255));
						hiex::TransparentImage(&test_background, 20, 90, &test_icon[2]);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 24, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(0, 111, 225, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 70;
							dwords_rect.top = 80;
							dwords_rect.right = 70 + 110;
							dwords_rect.bottom = 80 + 63;
						}
						graphics.DrawString(L"调测", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}
					else
					{
						hiex::EasyX_Gdiplus_FillRoundRect(10, 80, 180, 60, 20, 20, RGBA(230, 230, 230, 255), RGBA(230, 230, 230, 255), 3, false, SmoothingModeHighQuality, &test_background);

						ChangeColor(test_icon[2], RGBA(130, 130, 130, 255));
						hiex::TransparentImage(&test_background, 20, 90, &test_icon[2]);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 24, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(130, 130, 130, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 70;
							dwords_rect.top = 80;
							dwords_rect.right = 70 + 110;
							dwords_rect.bottom = 80 + 63;
						}
						graphics.DrawString(L"调测", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}

					if (TestMainMode == 3)
					{
						hiex::EasyX_Gdiplus_FillRoundRect(10, 150, 180, 60, 20, 20, RGBA(0, 111, 225, 255), RGBA(230, 230, 230, 255), 3, false, SmoothingModeHighQuality, &test_background);

						ChangeColor(test_icon[3], RGBA(0, 111, 225, 255));
						hiex::TransparentImage(&test_background, 20, 160, &test_icon[3]);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 24, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(0, 111, 225, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 70;
							dwords_rect.top = 150;
							dwords_rect.right = 70 + 110;
							dwords_rect.bottom = 150 + 63;
						}
						graphics.DrawString(L"关于", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}
					else
					{
						hiex::EasyX_Gdiplus_FillRoundRect(10, 150, 180, 60, 20, 20, RGBA(230, 230, 230, 255), RGBA(230, 230, 230, 255), 3, false, SmoothingModeHighQuality, &test_background);

						ChangeColor(test_icon[3], RGBA(130, 130, 130, 255));
						hiex::TransparentImage(&test_background, 20, 160, &test_icon[3]);

						Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 24, FontStyleRegular, UnitPixel);
						SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(130, 130, 130, 255), false));
						graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
						{
							dwords_rect.left = 70;
							dwords_rect.top = 150;
							dwords_rect.right = 70 + 110;
							dwords_rect.bottom = 150 + 63;
						}
						graphics.DrawString(L"关于", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
					}
				}
				//标题栏
				{
					SetImageColor(test_title, RGBA(0, 0, 0, 0), true);
					hiex::EasyX_Gdiplus_SolidRoundRect(5, 5, 1000, 740, 20, 20, RGBA(0, 111, 225, 200), true, SmoothingModeHighQuality, &test_title);

					Graphics graphics(GetImageHDC(&test_title));
					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 22, FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(255, 255, 255, 255), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						words_rect.left = 5;
						words_rect.top = 5;
						words_rect.right = 1005;
						words_rect.bottom = 50;
					}
					graphics.DrawString(L"智绘教 程序选项", -1, &gp_font, hiex::RECTToRectF(words_rect), &stringFormat, &WordBrush);
				}
				hiex::TransparentImage(&test_background, 200, 0, &test_content);
				hiex::TransparentImage(&test_title, 5, 45, &test_background);
				hiex::EasyX_Gdiplus_RoundRect(5, 5, 1000, 740, 20, 20, RGBA(0, 111, 225, 255), 3, false, SmoothingModeHighQuality, &test_title);

				{
					RECT rect;
					GetWindowRect(test_window, &rect);

					POINT ptDst = { rect.left, rect.top };
					ulwi.pptDst = &ptDst;
					ulwi.hdcSrc = GetImageHDC(&test_title);
					UpdateLayeredWindowIndirect(test_window, &ulwi);
				}

				if (off_signal || !test.select) break;

				Sleep(20);
			}
		}
		else if (IsWindowVisible(test_window)) ShowWindow(test_window, SW_HIDE);
	}

	ShowWindow(test_window, SW_HIDE);
	thread_status[L"test_main"] = false;

	return 0;
}

void FreezeFrameWindow()
{
	this_thread::sleep_for(chrono::seconds(3));
	while (magnificationWindowReady != -1) this_thread::sleep_for(chrono::milliseconds(50));

	DisableResizing(freeze_window, true);//禁止窗口拉伸
	SetWindowLong(freeze_window, GWL_STYLE, GetWindowLong(freeze_window, GWL_STYLE) & ~WS_CAPTION);//隐藏标题栏
	SetWindowPos(freeze_window, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_DRAWFRAME);
	SetWindowLong(freeze_window, GWL_EXSTYLE, WS_EX_TOOLWINDOW);//隐藏任务栏

	IMAGE freeze_background(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

	// 设置BLENDFUNCTION结构体
	BLENDFUNCTION blend;
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.SourceConstantAlpha = 255; // 设置透明度，0为全透明，255为不透明
	blend.AlphaFormat = AC_SRC_ALPHA; // 使用源图像的alpha通道
	HDC hdcScreen = GetDC(NULL);
	// 调用UpdateLayeredWindow函数更新窗口
	POINT ptSrc = { 0,0 };
	SIZE sizeWnd = { freeze_background.getwidth(),freeze_background.getheight() };
	POINT ptDst = { 0,0 }; // 设置窗口位置
	UPDATELAYEREDWINDOWINFO ulwi = { 0 };
	ulwi.cbSize = sizeof(ulwi);
	ulwi.hdcDst = hdcScreen;
	ulwi.pptDst = &ptDst;
	ulwi.psize = &sizeWnd;
	ulwi.pptSrc = &ptSrc;
	ulwi.crKey = RGB(255, 255, 255);
	ulwi.pblend = &blend;
	ulwi.dwFlags = ULW_ALPHA;
	LONG nRet = ::GetWindowLong(freeze_window, GWL_EXSTYLE);
	nRet |= WS_EX_LAYERED;
	::SetWindowLong(freeze_window, GWL_EXSTYLE, nRet);

	FreezeFrame.update = true;
	int wait = 0;

	while (!off_signal)
	{
		Sleep(20);

		if (FreezeFrame.mode == 1)
		{
			IMAGE MagnificationTmp;
			if (!IsWindowVisible(freeze_window))
			{
				std::shared_lock<std::shared_mutex> lock1(MagnificationBackgroundSm);
				MagnificationTmp = MagnificationBackground;
				lock1.unlock();

				hiex::TransparentImage(&freeze_background, 0, 0, &MagnificationTmp);
				ulwi.hdcSrc = GetImageHDC(&freeze_background);
				UpdateLayeredWindowIndirect(freeze_window, &ulwi);

				ShowWindow(freeze_window, SW_SHOW);
			}

			if (SeewoCamera) wait = 500;
			while (!off_signal)
			{
				if (FreezeFrame.mode != 1 || ppt_show != NULL) break;

				if (FreezeFrame.update)
				{
					std::shared_lock<std::shared_mutex> lock1(MagnificationBackgroundSm);
					MagnificationTmp = MagnificationBackground;
					lock1.unlock();

					hiex::TransparentImage(&freeze_background, 0, 0, &MagnificationTmp);
					ulwi.hdcSrc = GetImageHDC(&freeze_background);
					UpdateLayeredWindowIndirect(freeze_window, &ulwi);

					FreezeFrame.update = false;
				}
				else if (wait > 0 && SeewoCamera)
				{
					hiex::TransparentImage(&freeze_background, 0, 0, &MagnificationTmp);

					hiex::EasyX_Gdiplus_FillRoundRect(GetSystemMetrics(SM_CXSCREEN) / 2 - 160, GetSystemMetrics(SM_CYSCREEN) - 200, 320, 50, 20, 20, RGBA(255, 255, 225, min(255, wait)), RGBA(0, 0, 0, min(150, wait)), 2, true, SmoothingModeHighQuality, &freeze_background);

					Graphics graphics(GetImageHDC(&freeze_background));
					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 24, FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(255, 255, 255, min(255, wait)), true));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						dwords_rect.left = GetSystemMetrics(SM_CXSCREEN) / 2 - 160;
						dwords_rect.top = GetSystemMetrics(SM_CYSCREEN) - 200;
						dwords_rect.right = GetSystemMetrics(SM_CXSCREEN) / 2 + 160;
						dwords_rect.bottom = GetSystemMetrics(SM_CYSCREEN) - 200 + 52;
					}
					graphics.DrawString(L"智绘教已自动开启 画面定格", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);

					ulwi.hdcSrc = GetImageHDC(&freeze_background);
					UpdateLayeredWindowIndirect(freeze_window, &ulwi);

					wait -= 5;

					if (wait == 0)
					{
						hiex::TransparentImage(&freeze_background, 0, 0, &MagnificationTmp);

						ulwi.hdcSrc = GetImageHDC(&freeze_background);
						UpdateLayeredWindowIndirect(freeze_window, &ulwi);
					}
				}

				if (test.select) SetWindowPos(freeze_window, test_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				else if (!choose.select) SetWindowPos(freeze_window, drawpad_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				else if (ppt_show != NULL) SetWindowPos(freeze_window, ppt_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				else SetWindowPos(freeze_window, floating_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

				Sleep(20);
			}

			if (ppt_show != NULL) FreezeFrame.mode = 0;
			FreezeFrame.update = true;
		}
		else if (IsWindowVisible(freeze_window)) ShowWindow(freeze_window, SW_HIDE);

		if (FreezeFrame.mode != 1 && FreezePPT)
		{
			for (int i = -10; i <= 60 && FreezePPT; i++)
			{
				SetImageColor(freeze_background, RGBA(0, 0, 0, 140), true);
				hiex::TransparentImage(&freeze_background, GetSystemMetrics(SM_CXSCREEN) / 2 - 500, GetSystemMetrics(SM_CYSCREEN) / 2 - 163, &test_sign[3]);

				hiex::EasyX_Gdiplus_SolidRoundRect(GetSystemMetrics(SM_CXSCREEN) / 2 - 300, GetSystemMetrics(SM_CYSCREEN) / 2 + 200, 600, 10, 10, 10, RGBA(255, 255, 255, 100), true, SmoothingModeHighQuality, &freeze_background);
				hiex::EasyX_Gdiplus_SolidRoundRect(GetSystemMetrics(SM_CXSCREEN) / 2 - 300, GetSystemMetrics(SM_CYSCREEN) / 2 + 200, max(0, min(50, i)) * 12, 10, 10, 10, RGBA(255, 255, 255, 255), false, SmoothingModeHighQuality, &freeze_background);

				{
					Graphics graphics(GetImageHDC(&freeze_background));
					Gdiplus::Font gp_font(&HarmonyOS_fontFamily, 24, FontStyleRegular, UnitPixel);
					SolidBrush WordBrush(hiex::ConvertToGdiplusColor(RGBA(255, 255, 255, 255), false));
					graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
					{
						dwords_rect.left = GetSystemMetrics(SM_CXSCREEN) / 2 - 500;
						dwords_rect.top = GetSystemMetrics(SM_CYSCREEN) / 2 + 250;
						dwords_rect.right = GetSystemMetrics(SM_CXSCREEN) / 2 + 500;
						dwords_rect.bottom = GetSystemMetrics(SM_CYSCREEN) / 2 + 300;
					}
					graphics.DrawString(L"Tips：无需点击选择，点击下方按钮即可翻页", -1, &gp_font, hiex::RECTToRectF(dwords_rect), &stringFormat, &WordBrush);
				}

				ulwi.hdcSrc = GetImageHDC(&freeze_background);
				UpdateLayeredWindowIndirect(freeze_window, &ulwi);

				if (test.select) SetWindowPos(freeze_window, test_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				else if (!choose.select) SetWindowPos(freeze_window, drawpad_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
				else SetWindowPos(freeze_window, ppt_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

				if (!IsWindowVisible(freeze_window)) ShowWindow(freeze_window, SW_SHOWNOACTIVATE);

				hiex::DelayFPS(24);
			}

			FreezePPT = false;
		}
	}
}

int drawpad_main()
{
	thread_status[L"drawpad_main"] = true;

	//画笔初始化
	{
		brush.width = 4;
	}
	//窗口初始化
	{
		{
			/*
			if (IsWindows8OrGreater())
			{
				HMODULE hUser32 = LoadLibrary(TEXT("user32.dll"));
				if (hUser32)
				{
					pGetPointerFrameTouchInfo = (GetPointerFrameTouchInfoType)GetProcAddress(hUser32, "GetPointerFrameTouchInfo");
					uGetPointerFrameTouchInfo = true;
					//当完成使用时, 可以调用 FreeLibrary(hUser32);
				}
			}
			*/

			//hiex::SetWndProcFunc(drawpad_window, WndProc);
		}

		setbkmode(TRANSPARENT);
		setbkcolor(RGB(255, 255, 255));

		HiBeginDraw();
		cleardevice();
		hiex::FlushDrawing({ 0 }); HiEndDraw();

		DisableResizing(drawpad_window, true);//禁止窗口拉伸
		SetWindowLong(drawpad_window, GWL_STYLE, GetWindowLong(drawpad_window, GWL_STYLE) & ~WS_CAPTION);//隐藏标题栏
		SetWindowPos(drawpad_window, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_DRAWFRAME);
		SetWindowLong(drawpad_window, GWL_EXSTYLE, WS_EX_TOOLWINDOW);//隐藏任务栏

		//SetWindowTransparent(drawpad_window, true, 255);

		// 屏幕置顶
		//SetWindowPos(drawpad_window, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	}
	//媒体初始化
	{
		loadimage(&ppt_icon[1], L"PNG", L"ppt1");
		loadimage(&ppt_icon[2], L"PNG", L"ppt2");
		loadimage(&ppt_icon[3], L"PNG", L"ppt3");
	}

	//初始化数值
	{
		//屏幕快照处理
		if (_waccess((string_to_wstring(global_path) + L"ScreenShot\\attribute_directory.json").c_str(), 4) == 0)
		{
			Json::Reader reader;
			Json::Value root;

			ifstream readjson;
			readjson.imbue(locale("zh_CN.UTF8"));
			readjson.open(wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\attribute_directory.json").c_str());

			deque<wstring> authenticated_file;
			authenticated_file.push_back(string_to_wstring(global_path) + L"ScreenShot\\attribute_directory.json");
			if (reader.parse(readjson, root))
			{
				readjson.close();

				for (int i = 0; i < root["Image_Properties"].size(); i++)
				{
					deque<wstring> date = getPrevTwoDays(CurrentDate());

					auto it = find(date.begin(), date.end(), string_to_wstring(root["Image_Properties"][i]["date"].asString()));
					if (it == date.end())
					{
						remove(convert_to_gbk(root["Image_Properties"][i]["drawpad"].asString()).c_str());
						remove(convert_to_gbk(root["Image_Properties"][i]["background"].asString()).c_str());
						remove(convert_to_gbk(root["Image_Properties"][i]["blending"].asString()).c_str());

						root["Image_Properties"].removeIndex(i, nullptr);
						i--;
					}
					else if (_waccess(string_to_wstring(convert_to_gbk(root["Image_Properties"][i]["drawpad"].asString())).c_str(), 0) == -1)
					{
						remove(convert_to_gbk(root["Image_Properties"][i]["drawpad"].asString()).c_str());
						remove(convert_to_gbk(root["Image_Properties"][i]["background"].asString()).c_str());
						remove(convert_to_gbk(root["Image_Properties"][i]["blending"].asString()).c_str());

						root["Image_Properties"].removeIndex(i, nullptr);
						i--;
					}
					else
					{
						authenticated_file.push_back(string_to_wstring(convert_to_gbk(root["Image_Properties"][i]["drawpad"].asString())));
						authenticated_file.push_back(string_to_wstring(convert_to_gbk(root["Image_Properties"][i]["background"].asString())));
						authenticated_file.push_back(string_to_wstring(convert_to_gbk(root["Image_Properties"][i]["blending"].asString())));
					}
				}
				removeUnknownFiles(string_to_wstring(global_path) + L"ScreenShot", authenticated_file);
				removeEmptyFolders(string_to_wstring(global_path) + L"ScreenShot");

				Json::StyledWriter outjson;

				ofstream writejson;
				writejson.imbue(locale("zh_CN.UTF8"));
				writejson.open(wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\attribute_directory.json").c_str());
				writejson << outjson.write(root);
				writejson.close();
			}
			else readjson.close();
		}
		else
		{
			//创建路径
			filesystem::create_directory(string_to_wstring(global_path) + L"ScreenShot");

			Json::Value root;
			deque<wstring> authenticated_file;
			authenticated_file.push_back(string_to_wstring(global_path) + L"ScreenShot\\attribute_directory.json");

			removeUnknownFiles(string_to_wstring(global_path) + L"ScreenShot", authenticated_file);
			removeEmptyFolders(string_to_wstring(global_path) + L"ScreenShot");

			root["Image_Properties"].append(Json::Value("nullptr"));
			root["Image_Properties"].removeIndex(0, nullptr);

			Json::StyledWriter outjson;

			ofstream writejson;
			writejson.imbue(locale("zh_CN.UTF8"));
			writejson.open(wstring_to_string(string_to_wstring(global_path) + L"ScreenShot\\attribute_directory.json").c_str());
			writejson << outjson.write(root);
			writejson.close();
		}
	}

	// 设置BLENDFUNCTION结构体
	BLENDFUNCTION blend;
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.SourceConstantAlpha = 255; // 设置透明度，0为全透明，255为不透明
	blend.AlphaFormat = AC_SRC_ALPHA; // 使用源图像的alpha通道
	HDC hdcScreen = GetDC(NULL);
	// 调用UpdateLayeredWindow函数更新窗口
	POINT ptSrc = { 0,0 };
	SIZE sizeWnd = { drawpad.getwidth(),drawpad.getheight() };
	POINT ptDst = { 0,0 }; // 设置窗口位置
	UPDATELAYEREDWINDOWINFO ulwi = { 0 };
	ulwi.cbSize = sizeof(ulwi);
	ulwi.hdcDst = hdcScreen;
	ulwi.pptDst = &ptDst;
	ulwi.psize = &sizeWnd;
	ulwi.pptSrc = &ptSrc;
	ulwi.crKey = RGB(255, 255, 255);
	ulwi.pblend = &blend;
	ulwi.dwFlags = ULW_ALPHA;

	LONG nRet = ::GetWindowLong(drawpad_window, GWL_EXSTYLE);
	nRet |= WS_EX_LAYERED;
	::SetWindowLong(drawpad_window, GWL_EXSTYLE, nRet);

	magnificationWindowReady++;
	{
		SetImageColor(drawpad, RGBA(0, 0, 0, 1), true);
		SetImageColor(alpha_drawpad, RGBA(0, 0, 0, 0), true);
		{
			ulwi.hdcSrc = GetImageHDC(&background);
			UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
		}
		ShowWindow(drawpad_window, SW_SHOW);

		struct Mouse
		{
			int x = 0, y = 0;
			int last_x = 0, last_y = 0;
			int last_length = 0;
		}mouse;
		POINT pt = { 0,0 };
		double speed;

		SetWindowPos(drawpad_window, floating_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		LONG style = GetWindowLong(drawpad_window, GWL_EXSTYLE);
		style |= WS_EX_NOACTIVATE;
		SetWindowLong(drawpad_window, GWL_EXSTYLE, style);

		//启动绘图库程序
		hiex::Gdiplus_Try_Starup();
		MagInitialize();
		while (!off_signal)
		{
			if (choose.select == true)
			{
				if (draw_content) last_drawpad = drawpad;

				SetWindowRgn(drawpad_window, CreateRectRgn(0, 0, 0, 0), true);

				if (draw_content)
				{
					thread SaveScreenShot_thread(SaveScreenShot, last_drawpad);
					SaveScreenShot_thread.detach();
				}
				while (choose.select)
				{
					Sleep(50);

					ppt_info.currentSlides = ppt_info_stay.CurrentPage;
					ppt_info.totalSlides = ppt_info_stay.TotalPage;

					if (off_signal) goto drawpad_main_end;
				}

				if (empty_drawpad)
				{
					SetImageColor(drawpad, RGBA(0, 0, 0, 1), true);
					while (!RecallImage.empty())
					{
						RecallImage.pop_back();
					}
					deque<RecallStruct>(RecallImage).swap(RecallImage); // 使用swap技巧来释放未使用的内存

					extreme_point.clear();
				}
				else
				{
					drawpad = last_drawpad;
				}
				empty_drawpad = true;

				draw_content = false;
				{
					ulwi.hdcSrc = GetImageHDC(&drawpad);
					UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
				}
				SetWindowRgn(drawpad_window, CreateRectRgn(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)), true);

				TouchTemp.clear();
			}
			if (penetrate.select == true)
			{
				LONG nRet = ::GetWindowLong(drawpad_window, GWL_EXSTYLE);
				nRet |= WS_EX_TRANSPARENT;
				::SetWindowLong(drawpad_window, GWL_EXSTYLE, nRet);

				while (1)
				{
					SetWindowPos(drawpad_window, floating_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
					Sleep(50);

					if (off_signal) goto drawpad_main_end;
					if (penetrate.select == true) continue;
					break;
				}

				nRet = ::GetWindowLong(drawpad_window, GWL_EXSTYLE);
				nRet &= ~WS_EX_TRANSPARENT;
				::SetWindowLong(drawpad_window, GWL_EXSTYLE, nRet);

				TouchTemp.clear();
			}

			if (ppt_show != NULL) SetWindowPos(drawpad_window, ppt_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			else SetWindowPos(drawpad_window, floating_window, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

			std::shared_lock<std::shared_mutex> lock1(PointTempSm);
			bool start = !TouchTemp.empty();
			lock1.unlock();
			//开始绘图
			if (start)
			{
				if (rubber.select != true && (brush.mode == 1 || brush.mode == 2) && int(state) == 1) target_status = 0;

				std::shared_lock<std::shared_mutex> lock1(PointTempSm);
				LONG pid = TouchTemp.front().pid;
				pt = TouchTemp.front().pt;
				lock1.unlock();

				std::unique_lock<std::shared_mutex> lock2(PointTempSm);
				TouchTemp.pop_front();
				lock2.unlock();

				//hiex::PreSetWindowShowState(SW_HIDE);
				//HWND draw_window = initgraph(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

				if (rubber.select == true)
				{
					RECT rcDirty;
					draw_content = true;

					mouse.last_x = pt.x, mouse.last_y = pt.y;

					double rubbersize = 15, trubbersize = -1;
					while (1)
					{
						std::shared_lock<std::shared_mutex> lock0(PointPosSm);
						bool unfind = TouchPos.find(pid) == TouchPos.end();
						lock0.unlock();

						if (unfind) break;

						std::shared_lock<std::shared_mutex> lock1(PointPosSm);
						pt = TouchPos[pid].pt;
						lock1.unlock();

						std::shared_lock<std::shared_mutex> lock2(TouchSpeedSm);
						double speed = TouchSpeed[pid];
						lock2.unlock();

						std::shared_lock<std::shared_mutex> lock3(PointListSm);
						auto it = std::find(TouchList.begin(), TouchList.end(), pid);
						lock3.unlock();

						if (it == TouchList.end()) break;

						mouse.x = pt.x, mouse.y = pt.y;

						if (setlist.experimental_functions)
						{
							if (speed <= 0.1) trubbersize = 60;
							else if (speed <= 30) trubbersize = max(25, speed * 2.33 + 2.33);
							else trubbersize = min(200, speed + 30);

							if (trubbersize == -1) trubbersize = rubbersize;
							if (rubbersize < trubbersize) rubbersize = rubbersize + max(0.001, (trubbersize - rubbersize) / 50);
							else rubbersize = rubbersize + min(-0.001, (trubbersize - rubbersize) / 50);
						}
						else rubbersize = 60;

						if (pt.x == mouse.last_x && pt.y == mouse.last_y)
						{
							Graphics eraser(GetImageHDC(&drawpad));
							GraphicsPath path;
							path.AddEllipse(mouse.last_x - (int)rubbersize / 2, mouse.last_y - (int)rubbersize / 2, rubbersize, rubbersize);

							Region region(&path);
							eraser.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
							eraser.SetClip(&region, CombineModeReplace);
							eraser.Clear(Color(1, 0, 0, 0));
							eraser.ResetClip();

							for (const auto& [point, value] : extreme_point)
							{
								if (value == true && region.IsVisible(point.first, point.second))
								{
									extreme_point[{point.first, point.second}] = false;
								}
							}

							putout = drawpad;
							hiex::EasyX_Gdiplus_Ellipse(mouse.x - double(rubbersize) / 2, mouse.y - double(rubbersize) / 2, rubbersize, rubbersize, RGBA(130, 130, 130, 200), 3, true, SmoothingModeHighQuality, &putout);
						}
						else
						{
							Graphics eraser(GetImageHDC(&drawpad));
							GraphicsPath path;
							path.AddLine(mouse.last_x, mouse.last_y, mouse.x, mouse.y);

							Pen pen(Color(0, 0, 0, 0), rubbersize);
							pen.SetStartCap(LineCapRound);
							pen.SetEndCap(LineCapRound);

							GraphicsPath* widenedPath = path.Clone();
							widenedPath->Widen(&pen);
							Region region(widenedPath);
							eraser.SetClip(&region, CombineModeReplace);
							eraser.Clear(Color(1, 0, 0, 0));
							eraser.ResetClip();

							for (const auto& [point, value] : extreme_point)
							{
								if (value == true && region.IsVisible(point.first, point.second))
								{
									extreme_point[{point.first, point.second}] = false;
								}
							}

							delete widenedPath;

							putout = drawpad;
							hiex::EasyX_Gdiplus_Ellipse(mouse.x - double(rubbersize) / 2, mouse.y - double(rubbersize) / 2, rubbersize, rubbersize, RGBA(130, 130, 130, 200), 3, true, SmoothingModeHighQuality, &putout);
						}

						mouse.last_x = mouse.x, mouse.last_y = mouse.y;

						{
							RECT rcDirty = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

							ulwi.hdcSrc = GetImageHDC(&putout);
							ulwi.prcDirty = &rcDirty;
							UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
						}
					}

					{
						// 定义要更新的矩形区域
						RECT rcDirty = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

						ulwi.hdcSrc = GetImageHDC(&drawpad);
						ulwi.prcDirty = &rcDirty;
						UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
					}
				}
				else if (brush.select == true)
				{
					mouse.last_x = pt.x, mouse.last_y = pt.y;
					RECT rcDirty;
					int draw_width = brush.width;
					draw_content = true;

					double writing_distance = 0;
					vector<Point> points = { Point(mouse.last_x, mouse.last_y) };
					RECT circumscribed_rectangle = { -1,-1,-1,-1 };

					if (RecallImage.empty()) RecallImage.push_back({ drawpad, extreme_point });

					{
						if (brush.mode == 3 || brush.mode == 4);
						else
						{
							if (brush.mode == 1)
							{
								hiex::EasyX_Gdiplus_SolidEllipse(mouse.last_x - double(draw_width) / 2.0, mouse.last_y - double(draw_width) / 2.0, draw_width, draw_width, brush.color, false, SmoothingModeHighQuality, &drawpad);
								{
									// 定义要更新的矩形区域
									RECT rcDirty = { mouse.last_x - draw_width, mouse.last_y - draw_width, mouse.last_x + draw_width, mouse.last_y + draw_width };

									ulwi.hdcSrc = GetImageHDC(&drawpad);
									ulwi.prcDirty = &rcDirty;
									UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
								}
							}
							else
							{
								hiex::EasyX_Gdiplus_SolidEllipse(mouse.last_x - double(draw_width) / 2.0, mouse.last_y - double(draw_width) / 2.0, draw_width, draw_width, brush.color, false, SmoothingModeHighQuality, &alpha_drawpad);
								putout = drawpad;
								{
									HDC dstDC = GetImageHDC(&putout);
									HDC srcDC = GetImageHDC(&alpha_drawpad);
									int w = alpha_drawpad.getwidth();
									int h = alpha_drawpad.getheight();

									// 结构体的第三个成员表示额外的透明度，0 表示全透明，255 表示不透明。
									BLENDFUNCTION bf = { AC_SRC_OVER, 0, 130, AC_SRC_ALPHA };
									// 使用 Windows GDI 函数实现半透明位图
									AlphaBlend(dstDC, 0, 0, w, h, srcDC, 0, 0, w, h, bf);
								}

								{
									// 定义要更新的矩形区域
									RECT rcDirty = { mouse.last_x - draw_width * 10, mouse.last_y - draw_width * 10, mouse.last_x + draw_width * 10, mouse.last_y + draw_width * 10 };

									ulwi.hdcSrc = GetImageHDC(&putout);
									ulwi.prcDirty = &rcDirty;
									UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
								}
							}
						}
					}

					while (1)
					{
						std::shared_lock<std::shared_mutex> lock0(PointPosSm);
						bool unfind = TouchPos.find(pid) == TouchPos.end();
						lock0.unlock();

						if (unfind) break;

						std::shared_lock<std::shared_mutex> lock1(PointPosSm);
						pt = TouchPos[pid].pt;
						lock1.unlock();

						std::shared_lock<std::shared_mutex> lock2(PointListSm);
						auto it = std::find(TouchList.begin(), TouchList.end(), pid);
						lock2.unlock();

						if (it == TouchList.end()) break;

						if (pt.x == mouse.last_x && pt.y == mouse.last_y) continue;

						if (brush.mode == 3)
						{
							SetWorkingImage(&alpha_drawpad);
							setfillcolor(SET_ALPHA(BLACK, 0));
							solidrectangle(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
							SetWorkingImage(hiex::GetWindowImage());

							Graphics graphics(GetImageHDC(&alpha_drawpad));
							graphics.SetSmoothingMode(SmoothingModeHighQuality);

							Pen pen(hiex::ConvertToGdiplusColor(brush.color, false));
							pen.SetStartCap(LineCapRound);
							pen.SetEndCap(LineCapRound);

							pen.SetWidth(draw_width);
							graphics.DrawLine(&pen, mouse.last_x, mouse.last_y, pt.x, pt.y);

							rcDirty = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
							putout = drawpad;
							{
								HDC dstDC = GetImageHDC(&putout);
								HDC srcDC = GetImageHDC(&alpha_drawpad);
								int w = alpha_drawpad.getwidth();
								int h = alpha_drawpad.getheight();

								// 结构体的第三个成员表示额外的透明度，0 表示全透明，255 表示不透明。
								BLENDFUNCTION bf = { AC_SRC_OVER, 0, (brush.color >> 24), AC_SRC_ALPHA };
								// 使用 Windows GDI 函数实现半透明位图
								AlphaBlend(dstDC, 0, 0, w, h, srcDC, 0, 0, w, h, bf);
							}

							{
								ulwi.hdcSrc = GetImageHDC(&putout);
								ulwi.prcDirty = &rcDirty;
								UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
							}
						}
						else if (brush.mode == 4)
						{
							SetWorkingImage(&alpha_drawpad);
							setfillcolor(SET_ALPHA(BLACK, 0));
							solidrectangle(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
							SetWorkingImage(hiex::GetWindowImage());

							int rectangle_x = min(mouse.last_x, pt.x), rectangle_y = min(mouse.last_y, pt.y);
							int rectangle_heigth = abs(mouse.last_x - pt.x) + 1, rectangle_width = abs(mouse.last_y - pt.y) + 1;
							hiex::EasyX_Gdiplus_RoundRect(rectangle_x, rectangle_y, rectangle_heigth, rectangle_width, 3, 3, brush.color, draw_width, false, SmoothingModeHighQuality, &alpha_drawpad);

							if (points.size() < 2) points.emplace_back(Point(pt.x, pt.y));
							else points[1] = Point(pt.x, pt.y);

							rcDirty = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
							putout = drawpad;
							{
								HDC dstDC = GetImageHDC(&putout);
								HDC srcDC = GetImageHDC(&alpha_drawpad);
								int w = alpha_drawpad.getwidth();
								int h = alpha_drawpad.getheight();

								// 结构体的第三个成员表示额外的透明度，0 表示全透明，255 表示不透明。
								BLENDFUNCTION bf = { AC_SRC_OVER, 0, (brush.color >> 24), AC_SRC_ALPHA };
								// 使用 Windows GDI 函数实现半透明位图
								AlphaBlend(dstDC, 0, 0, w, h, srcDC, 0, 0, w, h, bf);
							}

							{
								ulwi.hdcSrc = GetImageHDC(&putout);
								ulwi.prcDirty = &rcDirty;
								UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
							}
						}
						else
						{
							writing_distance += EuclideanDistance({ mouse.last_x, mouse.last_y }, { pt.x, pt.y });

							if (brush.mode == 1)
							{
								rcDirty = DrawGradientLine(GetImageHDC(&drawpad), mouse.last_x, mouse.last_y, (mouse.x = pt.x), (mouse.y = pt.y), draw_width, hiex::ConvertToGdiplusColor(brush.color, false));

								{
									if (mouse.x < circumscribed_rectangle.left || circumscribed_rectangle.left == -1) circumscribed_rectangle.left = mouse.x;
									if (mouse.y < circumscribed_rectangle.top || circumscribed_rectangle.top == -1) circumscribed_rectangle.top = mouse.y;
									if (mouse.x > circumscribed_rectangle.right || circumscribed_rectangle.right == -1) circumscribed_rectangle.right = mouse.x;
									if (mouse.y > circumscribed_rectangle.bottom || circumscribed_rectangle.bottom == -1) circumscribed_rectangle.bottom = mouse.y;

									points.push_back(Point(mouse.x, mouse.y));
								}

								{
									ulwi.hdcSrc = GetImageHDC(&drawpad);
									ulwi.prcDirty = &rcDirty;
									UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
								}
							}
							else
							{
								rcDirty = DrawGradientLine(GetImageHDC(&alpha_drawpad), mouse.last_x, mouse.last_y, (mouse.x = pt.x), (mouse.y = pt.y), draw_width, hiex::ConvertToGdiplusColor(brush.color, false));
								putout = drawpad;
								{
									HDC dstDC = GetImageHDC(&putout);
									HDC srcDC = GetImageHDC(&alpha_drawpad);
									int w = alpha_drawpad.getwidth();
									int h = alpha_drawpad.getheight();

									// 结构体的第三个成员表示额外的透明度，0 表示全透明，255 表示不透明。
									BLENDFUNCTION bf = { AC_SRC_OVER, 0, 130, AC_SRC_ALPHA };
									// 使用 Windows GDI 函数实现半透明位图
									AlphaBlend(dstDC, 0, 0, w, h, srcDC, 0, 0, w, h, bf);
								}

								{
									ulwi.hdcSrc = GetImageHDC(&putout);
									ulwi.prcDirty = &rcDirty;
									UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
								}
							}

							mouse.last_x = mouse.x, mouse.last_y = mouse.y;
						}
					}
					if (brush.mode == 3 || brush.mode == 4)
					{
						drawpad = putout;
						SetWorkingImage(&alpha_drawpad);
						setfillcolor(SET_ALPHA(BLACK, 0));
						solidrectangle(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
						SetWorkingImage(hiex::GetWindowImage());
					}
					else if (brush.mode == 2)
					{
						drawpad = putout;
						SetWorkingImage(&alpha_drawpad);
						setfillcolor(SET_ALPHA(BLACK, 0));
						solidrectangle(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
						SetWorkingImage(hiex::GetWindowImage());
					}

					//智能绘图模块
					if (brush.mode == 1)
					{
						double redundance = max(GetSystemMetrics(SM_CXSCREEN) / 192, min((GetSystemMetrics(SM_CXSCREEN)) / 76.8, double(GetSystemMetrics(SM_CXSCREEN)) / double((-0.036) * writing_distance + 135)));

						//直线绘制
						if (writing_distance >= 120 && (abs(circumscribed_rectangle.left - circumscribed_rectangle.right) >= 120 || abs(circumscribed_rectangle.top - circumscribed_rectangle.bottom) >= 120) && isLine(points, redundance))
						{
							Point start(points[0]), end(points[points.size() - 1]);

							//端点匹配
							{
								//起点匹配
								{
									Point start_target = start;
									double distance = 10;
									for (const auto& [point, value] : extreme_point)
									{
										if (value == true)
										{
											if (EuclideanDistance({ point.first,point.second }, { start.X,start.Y }) <= distance)
											{
												distance = EuclideanDistance({ point.first,point.second }, { start.X,start.Y });
												start_target = { point.first,point.second };
											}
										}
									}
									start = start_target;
								}
								//终点匹配
								{
									Point end_target = end;
									double distance = 10;
									for (const auto& [point, value] : extreme_point)
									{
										if (value == true)
										{
											if (EuclideanDistance({ point.first,point.second }, { end.X,end.Y }) <= distance)
											{
												distance = EuclideanDistance({ point.first,point.second }, { end.X,end.Y });
												end_target = { point.first,point.second };
											}
										}
									}
									end = end_target;
								}
							}
							extreme_point[{start.X, start.Y}] = extreme_point[{end.X, end.Y}] = true;

							if (!RecallImage.empty()) drawpad = RecallImage.back().img;
							else SetImageColor(drawpad, RGBA(0, 0, 0, 1), true);

							Graphics graphics(GetImageHDC(&drawpad));
							graphics.SetSmoothingMode(SmoothingModeHighQuality);

							Pen pen(hiex::ConvertToGdiplusColor(brush.color, false));
							pen.SetStartCap(LineCapRound);
							pen.SetEndCap(LineCapRound);

							pen.SetWidth(draw_width);
							graphics.DrawLine(&pen, start.X, start.Y, end.X, end.Y);
						}
						//平滑曲线
						else if (points.size() > 2)
						{
							if (!RecallImage.empty()) drawpad = RecallImage.back().img;
							else SetImageColor(drawpad, RGBA(0, 0, 0, 1), true);

							Graphics graphics(GetImageHDC(&drawpad));
							graphics.SetSmoothingMode(SmoothingModeHighQuality);

							Pen pen(hiex::ConvertToGdiplusColor(brush.color, false));
							pen.SetLineJoin(LineJoinRound);
							pen.SetStartCap(LineCapRound);
							pen.SetEndCap(LineCapRound);

							pen.SetWidth(draw_width);
							graphics.DrawCurve(&pen, points.data(), points.size(), 0.4f);
						}
					}
					else if (brush.mode == 4)
					{
						//端点匹配
						if (points.size() == 2)
						{
							Point l1 = points[0];
							Point l2 = Point(points[0].X, points[points.size() - 1].Y);
							Point r1 = Point(points[points.size() - 1].X, points[0].Y);
							Point r2 = points[points.size() - 1];

							//端点匹配
							{
								{
									Point idx = l1;
									double distance = 10;
									for (const auto& [point, value] : extreme_point)
									{
										if (value == true)
										{
											if (EuclideanDistance({ point.first,point.second }, { l1.X,l1.Y }) <= distance)
											{
												distance = EuclideanDistance({ point.first,point.second }, { l1.X,l1.Y });
												idx = { point.first,point.second };
											}
										}
									}
									l1 = idx;

									l2.X = l1.X;
									r1.Y = l1.Y;
								}
								{
									Point idx = l2;
									double distance = 10;
									for (const auto& [point, value] : extreme_point)
									{
										if (value == true)
										{
											if (EuclideanDistance({ point.first,point.second }, { l2.X,l2.Y }) <= distance)
											{
												distance = EuclideanDistance({ point.first,point.second }, { l2.X,l2.Y });
												idx = { point.first,point.second };
											}
										}
									}
									l2 = idx;

									l1.X = l2.X;
									r2.Y = l2.Y;
								}
								{
									Point idx = r1;
									double distance = 10;
									for (const auto& [point, value] : extreme_point)
									{
										if (value == true)
										{
											if (EuclideanDistance({ point.first,point.second }, { r1.X,r1.Y }) <= distance)
											{
												distance = EuclideanDistance({ point.first,point.second }, { r1.X,r1.Y });
												idx = { point.first,point.second };
											}
										}
									}
									r1 = idx;

									r2.X = r1.X;
									l1.Y = r1.Y;
								}
								{
									Point idx = r2;
									double distance = 10;
									for (const auto& [point, value] : extreme_point)
									{
										if (value == true)
										{
											if (EuclideanDistance({ point.first,point.second }, { r2.X,r2.Y }) <= distance)
											{
												distance = EuclideanDistance({ point.first,point.second }, { r2.X,r2.Y });
												idx = { point.first,point.second };
											}
										}
									}
									r2 = idx;

									r1.X = r2.X;
									r2.Y = r2.Y;
								}
							}
							extreme_point[{l1.X, l1.Y}] = true;
							extreme_point[{l2.X, l2.Y}] = true;
							extreme_point[{r1.X, r1.Y}] = true;
							extreme_point[{r2.X, r2.Y}] = true;

							if (!RecallImage.empty()) drawpad = RecallImage.back().img;
							else SetImageColor(drawpad, RGBA(0, 0, 0, 1), true);

							int x = min(l1.X, r2.X);
							int y = min(l1.Y, r2.Y);
							int w = abs(l1.X - r2.X) + 1;
							int h = abs(l1.Y - r2.Y) + 1;

							hiex::EasyX_Gdiplus_RoundRect(x, y, w, h, 3, 3, brush.color, draw_width, false, SmoothingModeHighQuality, &drawpad);
						}
					}
				}

				std::unique_lock<std::shared_mutex> lock3(PointPosSm);
				TouchPos.erase(pid);
				lock3.unlock();

				if (RecallImage.empty() || (!RecallImage.empty() && !CompareImagesWithBuffer(&drawpad, &RecallImage.back().img)))
				{
					RecallImage.push_back({ drawpad, extreme_point });
					if (RecallImage.size() > 16)
					{
						while (RecallImage.size() > 16)
						{
							RecallImage.pop_front();
						}
						deque<RecallStruct>(RecallImage).swap(RecallImage); // 使用swap技巧来释放未使用的内存
					}
				}

				{
					// 定义要更新的矩形区域
					RECT rcDirty = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

					ulwi.hdcSrc = GetImageHDC(&drawpad);
					ulwi.prcDirty = &rcDirty;
					UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
				}
			}

			Sleep(1);

			if (ppt_info.currentSlides != ppt_info_stay.CurrentPage || ppt_info.totalSlides != ppt_info_stay.TotalPage)
			{
				if (ppt_info.currentSlides != ppt_info_stay.CurrentPage && ppt_info.totalSlides == ppt_info_stay.TotalPage)
				{
					if (draw_content)
					{
						ppt_img.is_save = true;
						ppt_img.is_saved[ppt_info.currentSlides] = true;
						ppt_img.image[ppt_info.currentSlides] = drawpad;
						//SaveScreenShot(drawpad);
					}

					if (ppt_img.is_saved[ppt_info_stay.CurrentPage] == true)
					{
						drawpad = ppt_img.image[ppt_info_stay.CurrentPage];
					}
					else
					{
						if (ppt_info_stay.TotalPage != -1) SetImageColor(drawpad, RGBA(0, 0, 0, 1), true);
					}

					while (!RecallImage.empty())
					{
						RecallImage.pop_back();
						deque<RecallStruct>(RecallImage).swap(RecallImage); // 使用swap技巧来释放未使用的内存
					}
				}
				ppt_info.currentSlides = ppt_info_stay.CurrentPage;
				ppt_info.totalSlides = ppt_info_stay.TotalPage;

				{
					RECT rcDirty = { 0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN) };

					ulwi.hdcSrc = GetImageHDC(&drawpad);
					ulwi.prcDirty = &rcDirty;
					UpdateLayeredWindowIndirect(drawpad_window, &ulwi);
				}
			}
		}
	}

drawpad_main_end:
	ShowWindow(drawpad_window, SW_HIDE);
	thread_status[L"drawpad_main"] = false;
	return 0;
}