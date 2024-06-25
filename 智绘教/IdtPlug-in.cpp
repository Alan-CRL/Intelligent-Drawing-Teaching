/*
 * @file		IdtPlug-in.cpp
 * @brief		IDT plugin linkage | 智绘教插件联动
 * @note		PPT linkage components and other plugins | PPT联动组件和其他插件等
 *
 * @author		AlanCRL
 * @qq			2685549821
 * @email		alan-crl@foxmail.com
*/

// INFO: This source file will take the lead in refactoring the code logic and optimizing the reading experience.
// 提示：这个源文件将率先重构代码逻辑，并优化阅读体验。

// FAQ: Some comments are followed by a '*' symbol, and their explanations are provided below.
// 常问问题：一些注释后带有 '*' 号，它们的解释在下面。
//
// *1
//
// The ·PptInfoStateBuffer· variable is a buffer for the ·PptInfoState· variable. After the DrawpadDrawing function loads the PPT's drawing board,
// the value in the buffer variable will become consistent with ·PptInfoState·. Some functions obtain the value of ·PptInfoStateBuffer· and must wait until
// the PPT drawing board is initialized before making any changes and responding.
//
// PptInfoStateBuffer 变量是 PptInfoState 变量的缓冲，当 DrawpadDrawing 函数加载完成 PPT 的画板后，缓冲变量中的值才会变为和 PptInfoState 一致。
// 一些函数获取 PptInfoStateBuffer 的值，必须要等到 PPT 画板初始化完毕后才会有所改变，并再做出反应。

#include "IdtPlug-in.h"

#include "IdtConfiguration.h"
#include "IdtDisplayManagement.h"
#include "IdtDraw.h"
#include "IdtDrawpad.h"
#include "IdtFloating.h"
#include "IdtMagnification.h"
#include "IdtRts.h"
#include "IdtText.h"
#include "IdtWindow.h"
#include "IdtOther.h"
#include "IdtD2DPreparation.h"
#include "IdtHistoricalDrawpad.h"
#include "IdtImage.h"
#include "IdtState.h"

// --------------------------------------------------
// PPT controls | PPT 控件

// TODO 1 控件缩放、控件置于左右两侧、控件拖动、UI 计算与绘制分离

#import "PptCOM.tlb" // C# Class Library PptCOM Project Library (PptCOM. cs) | C# 类库 PptCOM 项目库 (PptCOM. cs)
using namespace PptCOM;
IPptCOMServerPtr PptCOMPto;

class PptUiWidgetValue
{
public:
	float v, s, e;
};
class PptUiWidgetColor
{
public:
	COLORREF v;
	float s, e;
};

enum PptUiRegionWidgetID
{
	LeftSide,
	MiddleSide,
	RightSide
};
enum PptUiRoundRectWidgetID
{
	LeftSide_PageWidget,
	LeftSide_PageWidget_PreviousPage,
	LeftSide_PageWidget_NextPage,
	MiddleSide_TabSlideWidget,
	MiddleSide_TabSlideWidget_EndShow,
	MiddleSide_TabDrawpadWidget,
	// ...
	RightSide_PageWidget,
	RightSide_PageWidget_PreviousPage,
	RightSide_PageWidget_NextPage
};
enum PptUiImageWidgetID
{
	LeftSide_PreviousPage,
	LeftSide_NextPage,
	MiddleSide_EndShow,
	RightSide_PreviousPage,
	RightSide_NextPage
};
enum PptUiWordsWidgetID
{
	LeftSide_PageNum,
	RightSide_PageNum
};

class PptUiRegionWidgetClass
{
public:
	PptUiWidgetValue X;
	PptUiWidgetValue Y;
	PptUiWidgetValue Width;
	PptUiWidgetValue Height;
};
class PptUiRoundRectWidgetClass
{
public:
	PptUiWidgetValue X;
	PptUiWidgetValue Y;
	PptUiWidgetValue Width;
	PptUiWidgetValue Height;
	PptUiWidgetValue EllipseWidth;
	PptUiWidgetValue EllipseHeight;

	PptUiWidgetValue FrameThickness;
	PptUiWidgetColor FrameColor;
	PptUiWidgetColor FillColor;
};
class PptUiImageWidgetClass
{
public:
	PptUiWidgetValue X;
	PptUiWidgetValue Y;
	PptUiWidgetValue Width;
	PptUiWidgetValue Height;
	PptUiWidgetValue Transparency;
	ID2D1Bitmap* Img;
};
class PptUiWordsWidgetClass
{
public:
	PptUiWidgetValue Left;
	PptUiWidgetValue Top;
	PptUiWidgetValue Right;
	PptUiWidgetValue Bottom;

	PptUiWidgetValue WordsHeight;
	PptUiWidgetColor WordsColor;

	wstring WordsContent;
};

PptUiRoundRectWidgetClass pptUiRoundRectWidget[9], pptUiRoundRectWidgetTarget[9];
PptUiImageWidgetClass pptUiImageWidget[5], pptUiImageWidgetTarget[5];
PptUiWordsWidgetClass pptUiWordsWidget[2], pptUiWordsWidgetTarget[2];

void PptUiWidgetValueTransformation(float* v, float tv, float s, float e, int num = 1)
{
	if (!s || !e) return;

	while (num--)
	{
		if (abs(*v - tv) <= e) *v = tv;
		else if (*v < tv) *v = float(*v + max(e, (tv - *v) / s));
		else *v = float(*v + min(-e, (tv - *v) / s));
	}
}
void PptUiWidgetColorTransformation(COLORREF* v, COLORREF tv, float s, float e, int num = 1)
{
	if (!s || !e) return;

	while (num--)
	{
		float r1 = GetRValue(*v);
		float g1 = GetGValue(*v);
		float b1 = GetBValue(*v);
		float a1 = float(((*v) >> 24) & 0xff);

		float r2 = GetRValue(tv);
		float g2 = GetGValue(tv);
		float b2 = GetBValue(tv);
		float a2 = float((tv >> 24) & 0xff);

		if (abs(r1 - r2) <= e) r1 = r2;
		else if (r1 < r2) r1 = r1 + max(e, (r2 - r1) / s);
		else if (r1 > r2) r1 = r1 + min(-e, (r2 - r1) / s);

		if (abs(g1 - g2) <= e) g1 = g2;
		else if (g1 < g2) g1 = g1 + max(e, (g2 - g1) / s);
		else if (g1 > g2) g1 = g1 + min(-e, (g2 - g1) / s);

		if (abs(b1 - b2) <= e) b1 = b2;
		else if (b1 < b2) b1 = b1 + max(e, (b2 - b1) / s);
		else if (b1 > b2) b1 = b1 + min(-e, (b2 - b1) / s);

		if (abs(a1 - a2) <= e) a1 = a2;
		else if (a1 < a2) a1 = a1 + max(e, (a2 - a1) / s);
		else if (a1 > a2) a1 = a1 + min(-e, (a2 - a1) / s);

		*v = RGBA(max(0, min(255, (int)r1)), max(0, min(255, (int)g1)), max(0, min(255, (int)b1)), max(0, min(255, (int)a1)));
	}
}

float PptUiDistance(float t1, float t2)
{
	return abs(t1 - t2);
}
bool PptUiIsInRoundRect(float x, float y, PptUiRoundRectWidgetClass pptUiRoundRectWidget)
{
	if (pptUiRoundRectWidget.X.v + pptUiRoundRectWidget.EllipseWidth.v / 2.0f <= x && x <= pptUiRoundRectWidget.X.v + pptUiRoundRectWidget.Width.v - pptUiRoundRectWidget.EllipseWidth.v / 2.0f &&
		pptUiRoundRectWidget.Y.v <= y && y <= pptUiRoundRectWidget.Y.v + pptUiRoundRectWidget.Height.v)
		return true;
	if (pptUiRoundRectWidget.X.v <= x && x <= pptUiRoundRectWidget.X.v + pptUiRoundRectWidget.Width.v &&
		pptUiRoundRectWidget.Y.v + pptUiRoundRectWidget.EllipseHeight.v / 2.0f <= y && y <= pptUiRoundRectWidget.Y.v + pptUiRoundRectWidget.Height.v - pptUiRoundRectWidget.EllipseHeight.v / 2.0f)
		return true;

	if (PptUiDistance(x, pptUiRoundRectWidget.X.v + pptUiRoundRectWidget.EllipseWidth.v / 2.0f <= pptUiRoundRectWidget.EllipseWidth.v / 2.0f) &&
		PptUiDistance(y, pptUiRoundRectWidget.Y.v + pptUiRoundRectWidget.EllipseHeight.v / 2.0f <= pptUiRoundRectWidget.EllipseHeight.v / 2.0f))
		return true;
	if (PptUiDistance(x, pptUiRoundRectWidget.X.v + pptUiRoundRectWidget.Width.v + pptUiRoundRectWidget.EllipseWidth.v / 2.0f <= pptUiRoundRectWidget.EllipseWidth.v / 2.0f) &&
		PptUiDistance(y, pptUiRoundRectWidget.Y.v + pptUiRoundRectWidget.EllipseHeight.v / 2.0f <= pptUiRoundRectWidget.EllipseHeight.v / 2.0f))
		return true;
	if (PptUiDistance(x, pptUiRoundRectWidget.X.v + pptUiRoundRectWidget.EllipseWidth.v / 2.0f <= pptUiRoundRectWidget.EllipseWidth.v / 2.0f) &&
		PptUiDistance(y, pptUiRoundRectWidget.Y.v + pptUiRoundRectWidget.Height.v + pptUiRoundRectWidget.EllipseHeight.v / 2.0f <= pptUiRoundRectWidget.EllipseHeight.v / 2.0f))
		return true;
	if (PptUiDistance(x, pptUiRoundRectWidget.X.v + pptUiRoundRectWidget.Width.v + pptUiRoundRectWidget.EllipseWidth.v / 2.0f <= pptUiRoundRectWidget.EllipseWidth.v / 2.0f) &&
		PptUiDistance(y, pptUiRoundRectWidget.Y.v + pptUiRoundRectWidget.Height.v + pptUiRoundRectWidget.EllipseHeight.v / 2.0f <= pptUiRoundRectWidget.EllipseHeight.v / 2.0f))
		return true;

	return false;
}

float PPTUIScale = 1.0f;

PptImgStruct PptImg = { false }; // It stores image data generated during slide shows. | 其存储幻灯片放映时产生的图像数据。
PptInfoStateStruct PptInfoState = { -1, -1 }; // It stores the current status of the slide show software, where First represents the total number of slide pages and Second represents the current slide number. | 其存储幻灯片放映软件当前的状态，First 代表总幻灯片页数，Second 代表当前幻灯片编号。
PptInfoStateStruct PptInfoStateBuffer = { -1, -1 }; // Buffered variables for ·PptInfoState·. *1 | PptInfoState 的缓冲变量。*1

IMAGE PptIcon[5]; // Button icons for PPT controls | PPT 控件的按键图标
IMAGE PptWindowBackground; // PPT window background canvas | PPT 窗口背景画布

bool PptWindowBackgroundUiChange = true;

wstring pptComVersion;
wstring GetPptComVersion()
{
	wstring ret = L"Error: COM库(.dll) 不存在，且发生严重错误，返回值被忽略";

	if (_waccess((StringToWstring(globalPath) + L"PptCOM.dll").c_str(), 4) == 0)
	{
		try
		{
			ret = BstrToWstring(PptCOMPto->GetVersion());
			if (!regex_match(ret, wregex(L"\\d{8}[a-z]"))) ret = L"Error: " + ret;
		}
		catch (_com_error& err)
		{
			ret = L"Error: COM库(.dll) 存在，COM成功初始化，但C++端COM接口异常：" + wstring(err.ErrorMessage());
		}
	}
	else
	{
		wchar_t absolutePath[_MAX_PATH];

		if (_wfullpath(absolutePath, L"PptCOM.dll", _MAX_PATH) != NULL)
		{
			ret = L"Error: COM库(.dll) 不存在，预期调用目录为：\"" + StringToWstring(globalPath) + L"PptCOM.dll\"";
		}
		else ret = L"Error: COM库(.dll) 不存在，预期调用目录测算失败";
	}

	return ret;
}

wstring GetPptTitle()
{
	wstring ret = L"";

	try
	{
		ret = BstrToWstring(PptCOMPto->slideNameIndex());

		return ret;
	}
	catch (_com_error)
	{
	}

	return ret;
}
HWND GetPptShow()
{
	HWND hWnd = NULL;

	try
	{
		_variant_t result = PptCOMPto->GetPptHwnd();
		hWnd = (HWND)result.llVal;

		return hWnd;
	}
	catch (_com_error)
	{
	}

	return NULL;
}

void NextPptSlides(int check)
{
	try
	{
		PptCOMPto->NextSlideShow(check);
	}
	catch (_com_error)
	{
	}
	return;
}
void PreviousPptSlides()
{
	try
	{
		PptCOMPto->PreviousSlideShow();
	}
	catch (_com_error)
	{
	}
	return;
}
bool EndPptShow()
{
	try
	{
		PptCOMPto->EndSlideShow();

		return true;
	}
	catch (_com_error)
	{
	}

	return false;
}

// PPT 状态获取轮询函数
void GetPptState()
{
	threadStatus[L"GetPptState"] = true;

	// 初始化
	{
		bool rel = false;

		try
		{
			_com_util::CheckError(PptCOMPto.CreateInstance(_uuidof(PptCOMServer)));
			rel = PptCOMPto->Initialization(&PptInfoState.TotalPage, &PptInfoState.CurrentPage);
		}
		catch (_com_error)
		{
		}

		pptComVersion = GetPptComVersion();
	}

	while (!offSignal)
	{
		int tmp = -1;
		try
		{
			tmp = PptCOMPto->IsPptOpen();
		}
		catch (_com_error)
		{
		}

		if (tmp <= 0)
		{
			for (int i = 0; i <= 30 && !offSignal; i++)
				this_thread::sleep_for(chrono::milliseconds(100));
		}
	}

	threadStatus[L"GetPptState"] = false;
}

void PptUI()
{
}
void PptDraw()
{
	threadStatus[L"PptDraw"] = true;

	//ppt窗口初始化
	MainMonitorStruct PPTMainMonitor;
	{
		DisableResizing(ppt_window, true);//禁止窗口拉伸
		SetWindowLong(ppt_window, GWL_STYLE, GetWindowLong(ppt_window, GWL_STYLE) & ~WS_CAPTION);//隐藏标题栏
		SetWindowPos(ppt_window, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_DRAWFRAME | SWP_NOACTIVATE);
		SetWindowLong(ppt_window, GWL_EXSTYLE, WS_EX_TOOLWINDOW);//隐藏任务栏

		shared_lock<shared_mutex> DisplaysInfoLock2(DisplaysInfoSm);
		PPTMainMonitor = MainMonitor;
		DisplaysInfoLock2.unlock();

		PptWindowBackground.Resize(PPTMainMonitor.MonitorWidth, PPTMainMonitor.MonitorHeight);
		SetWindowPos(ppt_window, NULL, PPTMainMonitor.rcMonitor.left, PPTMainMonitor.rcMonitor.top, PPTMainMonitor.MonitorWidth, PPTMainMonitor.MonitorHeight, SWP_NOZORDER | SWP_NOACTIVATE);
	}

	// 创建 EasyX 兼容的 DC Render Target
	ID2D1DCRenderTarget* DCRenderTarget = nullptr;
	D2DFactory->CreateDCRenderTarget(&D2DProperty, &DCRenderTarget);

	// 绑定 EasyX DC
	RECT PptBackgroundWindowRect = { 0, 0, PptWindowBackground.getwidth(), PptWindowBackground.getheight() };
	DCRenderTarget->BindDC(GetImageHDC(&PptWindowBackground), &PptBackgroundWindowRect);

	// 设置抗锯齿
	DCRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE::D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

	//媒体初始化
	ID2D1Bitmap* PptIconBitmap[5] = { NULL };
	{
		loadimage(&PptIcon[1], L"PNG", L"ppt1");
		loadimage(&PptIcon[2], L"PNG", L"ppt2");
		loadimage(&PptIcon[3], L"PNG", L"ppt3");

		ChangeColor(PptIcon[1], RGB(50, 50, 50));
		ChangeColor(PptIcon[2], RGB(50, 50, 50));
		ChangeColor(PptIcon[3], RGB(50, 50, 50));

		{
			int width = PptIcon[1].getwidth();
			int height = PptIcon[1].getheight();
			DWORD* pMem = GetImageBuffer(&PptIcon[1]);

			unsigned char* data = new unsigned char[width * height * 4];
			for (int y = 0; y < height; ++y)
			{
				for (int x = 0; x < width; ++x)
				{
					DWORD color = pMem[y * width + x];
					unsigned char alpha = (color & 0xFF000000) >> 24;
					if (alpha != 0)
					{
						data[(y * width + x) * 4 + 0] = unsigned char(((color & 0x00FF0000) >> 16) * 255 / alpha);
						data[(y * width + x) * 4 + 1] = unsigned char(((color & 0x0000FF00) >> 8) * 255 / alpha);
						data[(y * width + x) * 4 + 2] = unsigned char(((color & 0x000000FF) >> 0) * 255 / alpha);
					}
					else
					{
						data[(y * width + x) * 4 + 0] = 0;
						data[(y * width + x) * 4 + 1] = 0;
						data[(y * width + x) * 4 + 2] = 0;
					}
					data[(y * width + x) * 4 + 3] = alpha;
				}
			}

			D2D1_BITMAP_PROPERTIES bitmapProps = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
			DCRenderTarget->CreateBitmap(D2D1::SizeU(width, height), data, width * 4, bitmapProps, &PptIconBitmap[1]);
			delete[] data;
		}
		{
			int width = PptIcon[2].getwidth();
			int height = PptIcon[2].getheight();
			DWORD* pMem = GetImageBuffer(&PptIcon[2]);

			unsigned char* data = new unsigned char[width * height * 4];
			for (int y = 0; y < height; ++y)
			{
				for (int x = 0; x < width; ++x)
				{
					DWORD color = pMem[y * width + x];
					unsigned char alpha = (color & 0xFF000000) >> 24;
					if (alpha != 0)
					{
						data[(y * width + x) * 4 + 0] = unsigned char(((color & 0x00FF0000) >> 16) * 255 / alpha);
						data[(y * width + x) * 4 + 1] = unsigned char(((color & 0x0000FF00) >> 8) * 255 / alpha);
						data[(y * width + x) * 4 + 2] = unsigned char(((color & 0x000000FF) >> 0) * 255 / alpha);
					}
					else
					{
						data[(y * width + x) * 4 + 0] = 0;
						data[(y * width + x) * 4 + 1] = 0;
						data[(y * width + x) * 4 + 2] = 0;
					}
					data[(y * width + x) * 4 + 3] = alpha;
				}
			}

			D2D1_BITMAP_PROPERTIES bitmapProps = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
			DCRenderTarget->CreateBitmap(D2D1::SizeU(width, height), data, width * 4, bitmapProps, &PptIconBitmap[2]);
			delete[] data;
		}
		{
			int width = PptIcon[3].getwidth();
			int height = PptIcon[3].getheight();
			DWORD* pMem = GetImageBuffer(&PptIcon[3]);

			unsigned char* data = new unsigned char[width * height * 4];
			for (int y = 0; y < height; ++y)
			{
				for (int x = 0; x < width; ++x)
				{
					DWORD color = pMem[y * width + x];
					unsigned char alpha = (color & 0xFF000000) >> 24;
					if (alpha != 0)
					{
						data[(y * width + x) * 4 + 0] = unsigned char(((color & 0x00FF0000) >> 16) * 255 / alpha);
						data[(y * width + x) * 4 + 1] = unsigned char(((color & 0x0000FF00) >> 8) * 255 / alpha);
						data[(y * width + x) * 4 + 2] = unsigned char(((color & 0x000000FF) >> 0) * 255 / alpha);
					}
					else
					{
						data[(y * width + x) * 4 + 0] = 0;
						data[(y * width + x) * 4 + 1] = 0;
						data[(y * width + x) * 4 + 2] = 0;
					}
					data[(y * width + x) * 4 + 3] = alpha;
				}
			}

			D2D1_BITMAP_PROPERTIES bitmapProps = D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
			DCRenderTarget->CreateBitmap(D2D1::SizeU(width, height), data, width * 4, bitmapProps, &PptIconBitmap[3]);
			delete[] data;
		}
	}

	//UI 初始化
	{
		PPTUIScale = 1.0f;

		// 左侧控件
		{
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].X = { (5) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Y = { (float)PPTMainMonitor.MonitorHeight + (5) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Width = { (185) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Height = { (60) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].EllipseWidth = { (30) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].EllipseHeight = { (30) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].FrameThickness = { (1) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].FillColor = { RGBA(225, 225, 225, 0), 10, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].FrameColor = { RGBA(200, 200, 200, 0), 10, 1 };

			{
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].X = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].X.v + (5) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Y = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Y.v + (5) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Width = { (50) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Height = { (50) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].EllipseWidth = { (35) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].EllipseHeight = { (35) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].FrameThickness = { (1) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].FillColor = { RGBA(250, 250, 250, 0), 3, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].FrameColor = { RGBA(200, 200, 200, 0), 10, 1 };

				pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].X = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].X.v + (5) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].Y = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Y.v + (5) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].Width = { (40) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].Height = { (40) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].Transparency = { 0, 10, 1 };
				pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].Img = NULL;
			}
			{
				pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].Left = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].X.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Width.v + (5) * PPTUIScale, 5, 1 };
				pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].Top = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Y.v + (5) * PPTUIScale, 5, 1 };
				pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].Right = { pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].Left.v + (65) * PPTUIScale, 5, 1 };
				pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].Bottom = { pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].Top.v + (55) * PPTUIScale, 5, 1 };
				pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].WordsHeight = { (20) * PPTUIScale, 5, 1 };
				pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].WordsColor = { RGBA(50, 50, 50, 0), 5, 1 };
				pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].WordsContent = L"Inkeys";
			}
			{
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].X = { pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].Right.v + (5) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].Y = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Y.v + (5) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].Width = { (50) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].Height = { (50) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].EllipseWidth = { (35) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].EllipseHeight = { (35) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].FrameThickness = { (1) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].FillColor = { RGBA(250, 250, 250, 0), 3, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].FrameColor = { RGBA(200, 200, 200, 0), 10, 1 };

				pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].X = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].X.v + (5) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].Y = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].Y.v + (5) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].Width = { (40) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].Height = { (40) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].Transparency = { 0, 20, 1 };
				pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].Img = NULL;
			}
		}
		// 中间控件
		{
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].X = { (float)PPTMainMonitor.MonitorWidth / 2 - (30) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].Y = { (float)PPTMainMonitor.MonitorHeight + (5) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].Width = { (60) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].Height = { (60) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].EllipseWidth = { (30) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].EllipseHeight = { (30) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].FrameThickness = { (1) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].FillColor = { RGBA(225, 225, 225, 0), 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].FrameColor = { RGBA(200, 200, 200, 0), 10, 1 };

			{
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].X = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].X.v + (5) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Y = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].Y.v + (5) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Width = { (50) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Height = { (50) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].EllipseWidth = { (35) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Height = { (35) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].FrameThickness = { (1) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].FillColor = { RGBA(250, 250, 250, 0), 3, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].FrameColor = { RGBA(200, 200, 200, 0), 10, 1 };

				pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].X = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].X.v + (5) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].Y = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Y.v + (5) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].Width = { (40) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].Height = { (40) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].Transparency = { 0, 10, 1 };
				pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].Img = NULL;
			}
		}
		// 右侧控件
		{
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].X = { (float)PPTMainMonitor.MonitorWidth - (190) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].Y = { (float)PPTMainMonitor.MonitorHeight + (5) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].Width = { (185) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].Height = { (60) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].EllipseWidth = { (30) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].EllipseHeight = { (30) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].FrameThickness = { (1) * PPTUIScale, 5, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].FillColor = { RGBA(225, 225, 225, 0), 10, 1 };
			pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].FrameColor = { RGBA(200, 200, 200, 0), 10, 1 };

			{
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].X = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].X.v + (5) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Y = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].Y.v + (5) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Width = { (50) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Height = { (50) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].EllipseWidth = { (35) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].EllipseHeight = { (35) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].FrameThickness = { (1) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].FillColor = { RGBA(250, 250, 250, 0), 3, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].FrameColor = { RGBA(200, 200, 200, 0), 10, 1 };

				pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].X = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].X.v + (5) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].Y = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Y.v + (5) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].Width = { (40) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].Height = { (40) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].Transparency = { 0, 10, 1 };
				pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].Img = NULL;
			}
			{
				pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].Left = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].X.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Width.v + (5) * PPTUIScale, 5, 1 };
				pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].Top = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].Y.v + (5) * PPTUIScale, 5, 1 };
				pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].Right = { pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].Left.v + (65) * PPTUIScale, 5, 1 };
				pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].Bottom = { pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].Top.v + (55) * PPTUIScale, 5, 1 };
				pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].WordsHeight = { (20) * PPTUIScale, 5, 1 };
				pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].WordsColor = { RGBA(50, 50, 50, 0), 5, 1 };
				pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].WordsContent = L"Inkeys";
			}
			{
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].X = { pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].Right.v + (5) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].Y = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].Y.v + (5) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].Width = { (50) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].Height = { (50) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].EllipseWidth = { (35) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].EllipseHeight = { (35) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].FrameThickness = { (1) * PPTUIScale, 5, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].FillColor = { RGBA(250, 250, 250, 0), 3, 1 };
				pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].FrameColor = { RGBA(200, 200, 200, 0), 10, 1 };

				pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].X = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].X.v + (5) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].Y = { pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].Y.v + (5) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].Width = { (40) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].Height = { (40) * PPTUIScale, 5, 1 };
				pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].Transparency = { 0, 10, 1 };
				pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].Img = NULL;
			}
		}

		memcpy(pptUiRoundRectWidgetTarget, pptUiRoundRectWidget, sizeof(pptUiRoundRectWidget));
		memcpy(pptUiImageWidgetTarget, pptUiImageWidget, sizeof(pptUiRoundRectWidget));
		memcpy(pptUiWordsWidgetTarget, pptUiWordsWidget, sizeof(pptUiRoundRectWidget));
	}

	// 设置BLENDFUNCTION结构体
	BLENDFUNCTION blend;
	{
		blend.BlendOp = AC_SRC_OVER;
		blend.BlendFlags = 0;
		blend.SourceConstantAlpha = 255; // 设置透明度，0为全透明，255为不透明
		blend.AlphaFormat = AC_SRC_ALPHA; // 使用源图像的alpha通道
	}

	POINT ptSrc = { 0,0 };
	SIZE sizeWnd = { PptWindowBackground.getwidth(),PptWindowBackground.getheight() };
	POINT ptDst = { PPTMainMonitor.rcMonitor.left, PPTMainMonitor.rcMonitor.top };

	// 调用UpdateLayeredWindow函数更新窗口
	UPDATELAYEREDWINDOWINFO ulwi = { 0 };
	{
		ulwi.cbSize = sizeof(ulwi);
		ulwi.hdcDst = GetDC(NULL);
		ulwi.pptDst = &ptDst;
		ulwi.psize = &sizeWnd;
		ulwi.pptSrc = &ptSrc;
		ulwi.crKey = RGB(255, 255, 255);
		ulwi.pblend = &blend;
		ulwi.dwFlags = ULW_ALPHA;
	}

	while (!(GetWindowLong(ppt_window, GWL_EXSTYLE) & WS_EX_LAYERED))
	{
		SetWindowLong(ppt_window, GWL_EXSTYLE, GetWindowLong(ppt_window, GWL_EXSTYLE) | WS_EX_LAYERED);
		if (GetWindowLong(ppt_window, GWL_EXSTYLE) & WS_EX_LAYERED) break;

		this_thread::sleep_for(chrono::milliseconds(10));
	}
	while (!(GetWindowLong(ppt_window, GWL_EXSTYLE) & WS_EX_NOACTIVATE))
	{
		SetWindowLong(ppt_window, GWL_EXSTYLE, GetWindowLong(ppt_window, GWL_EXSTYLE) | WS_EX_NOACTIVATE);
		if (GetWindowLong(ppt_window, GWL_EXSTYLE) & WS_EX_NOACTIVATE) break;

		this_thread::sleep_for(chrono::milliseconds(10));
	}

	int TotalSlides = -1, TotalSlidesLast = -2;
	int CurrentSlides = -1;

	bool Initialization = false; // 控件初始化完毕
	clock_t tRecord = clock();
	for (bool IsShowWindow = false; !offSignal;)
	{
		int TotalSlides = PptInfoState.TotalPage;
		int CurrentSlides = PptInfoState.CurrentPage;

		// 监视器监测
		{
			shared_lock<shared_mutex> DisplaysInfoLock2(DisplaysInfoSm);
			bool MainMonitorDifferent = (PPTMainMonitor != MainMonitor);
			if (MainMonitorDifferent) PPTMainMonitor = MainMonitor;
			DisplaysInfoLock2.unlock();

			if (MainMonitorDifferent)
			{
				PptWindowBackground.Resize(PPTMainMonitor.MonitorWidth, PPTMainMonitor.MonitorHeight);
				SetWindowPos(ppt_window, NULL, PPTMainMonitor.rcMonitor.left, PPTMainMonitor.rcMonitor.top, PPTMainMonitor.MonitorWidth, PPTMainMonitor.MonitorHeight, SWP_NOZORDER | SWP_NOACTIVATE);

				sizeWnd = { PPTMainMonitor.MonitorWidth, PPTMainMonitor.MonitorHeight };
				POINT ptDst = { PPTMainMonitor.rcMonitor.left, PPTMainMonitor.rcMonitor.top };

				RECT PptBackgroundWindowRect = { 0, 0, PPTMainMonitor.MonitorWidth, PPTMainMonitor.MonitorHeight };
				DCRenderTarget->BindDC(GetImageHDC(&PptWindowBackground), &PptBackgroundWindowRect);
			}
		}
		// UI 计算部分
		{
			// UI 单次修改计算（例如按钮）
			if (TotalSlides != TotalSlidesLast)
			{
				TotalSlidesLast = TotalSlides;
				if (TotalSlides != -1)
				{
					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].FillColor.v, 160);
					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].FrameColor.v, 160);

					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].FillColor.v, 160);
					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].FrameColor.v, 160);

					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].FillColor.v, 160);
					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].FrameColor.v, 160);

					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].FillColor.v, 160);
					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].FrameColor.v, 160);

					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].FillColor.v, 160);
					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].FrameColor.v, 160);
				}
				else if (TotalSlides == -1)
				{
					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].FillColor.v, 0);
					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].FrameColor.v, 0);

					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].FillColor.v, 0);
					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].FrameColor.v, 0);

					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].FillColor.v, 0);
					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].FrameColor.v, 0);

					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].FillColor.v, 0);
					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].FrameColor.v, 0);

					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].FillColor.v, 0);
					SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].FrameColor.v, 0);
				}
			}

			// UI 控件实时计算
			{
				// 左侧控件
				{
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].X.v = (5) * PPTUIScale;
					if (TotalSlides == -1) pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Y.v = PPTMainMonitor.MonitorHeight + (5) * PPTUIScale;
					else pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Y.v = PPTMainMonitor.MonitorHeight - (65) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Width.v = (185) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Height.v = (60) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].EllipseWidth.v = (30) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].EllipseHeight.v = (30) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].FrameThickness.v = (1) * PPTUIScale;
					if (TotalSlides == -1)
					{
						SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].FillColor.v, 0);
						SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].FrameColor.v, 0);
					}
					else
					{
						SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].FillColor.v, 160);
						SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].FrameColor.v, 160);
					}

					{
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].X.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].X.v + (5) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Y.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Y.v + (5) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Width.v = (50) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Height.v = (50) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].EllipseWidth.v = (35) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].EllipseHeight.v = (35) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].FrameThickness.v = (1) * PPTUIScale;

						{
							pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_PreviousPage].X.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].X.v + (5) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_PreviousPage].Y.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Y.v + (5) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_PreviousPage].Width.v = (40) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_PreviousPage].Height.v = (40) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_PreviousPage].Transparency.v = 255;
							pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_PreviousPage].Img = PptIconBitmap[1];
						}
					}
					{
						pptUiWordsWidgetTarget[PptUiWordsWidgetID::LeftSide_PageNum].Left.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].X.v + pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Width.v + (5) * PPTUIScale;
						pptUiWordsWidgetTarget[PptUiWordsWidgetID::LeftSide_PageNum].Top.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Y.v + (5) * PPTUIScale;
						pptUiWordsWidgetTarget[PptUiWordsWidgetID::LeftSide_PageNum].Right.v = pptUiWordsWidgetTarget[PptUiWordsWidgetID::LeftSide_PageNum].Left.v + (65) * PPTUIScale;
						pptUiWordsWidgetTarget[PptUiWordsWidgetID::LeftSide_PageNum].Bottom.v = pptUiWordsWidgetTarget[PptUiWordsWidgetID::LeftSide_PageNum].Top.v + (55) * PPTUIScale;
						pptUiWordsWidgetTarget[PptUiWordsWidgetID::LeftSide_PageNum].WordsHeight.v = (20) * PPTUIScale;
						SetAlpha(pptUiWordsWidgetTarget[PptUiWordsWidgetID::LeftSide_PageNum].WordsColor.v, 255);
						{
							wstring temp;
							if (CurrentSlides >= 100 || TotalSlides >= 100)
							{
								temp = CurrentSlides == -1 ? L"-" : to_wstring(CurrentSlides);
								temp += L"\n";
								temp += to_wstring(TotalSlides);
							}
							else
							{
								temp = CurrentSlides == -1 ? L"-" : to_wstring(CurrentSlides);
								temp += L"/";
								temp += to_wstring(TotalSlides);
							}
							pptUiWordsWidgetTarget[PptUiWordsWidgetID::LeftSide_PageNum].WordsContent = temp;
						}
					}
					{
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].X.v = pptUiWordsWidgetTarget[PptUiWordsWidgetID::LeftSide_PageNum].Right.v + (5) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].Y.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Y.v + (5) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].Width.v = (50) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].Height.v = (50) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].EllipseWidth.v = (35) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].EllipseHeight.v = (35) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].FrameThickness.v = (1) * PPTUIScale;

						{
							pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_NextPage].X.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].X.v + (5) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_NextPage].Y.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].Y.v + (5) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_NextPage].Width.v = (40) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_NextPage].Height.v = (40) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_NextPage].Transparency.v = 255;
							if (CurrentSlides == -1) pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_NextPage].Img = PptIconBitmap[3];
							else pptUiImageWidgetTarget[PptUiImageWidgetID::LeftSide_NextPage].Img = PptIconBitmap[2];
						}
					}
				}
				// 中间控件
				{
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].X.v = PPTMainMonitor.MonitorWidth / 2 - (30) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].Y.v = PPTMainMonitor.MonitorHeight - (65) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].Width.v = (60) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].Height.v = (60) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].EllipseWidth.v = (30) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].EllipseHeight.v = (30) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].FrameThickness.v = (1) * PPTUIScale;
					if (TotalSlides == -1)
					{
						SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].FillColor.v, 0);
						SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].FrameColor.v, 0);
					}
					else
					{
						SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].FillColor.v, 160);
						SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].FrameColor.v, 160);
					}

					{
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].X.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].X.v + (5) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Y.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].Y.v + (5) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Width.v = (50) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Height.v = (50) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].EllipseWidth.v = (35) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Width.v = (35) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].FrameThickness.v = (1) * PPTUIScale;

						{
							pptUiImageWidgetTarget[PptUiImageWidgetID::MiddleSide_EndShow].X.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].X.v + (5) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::MiddleSide_EndShow].Y.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Y.v + (5) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::MiddleSide_EndShow].Width.v = (40) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::MiddleSide_EndShow].Height.v = (40) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::MiddleSide_EndShow].Transparency.v = 255;
							pptUiImageWidgetTarget[PptUiImageWidgetID::MiddleSide_EndShow].Img = PptIconBitmap[3];
						}
					}
				}
				// 右侧控件
				{
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].X.v = PPTMainMonitor.MonitorWidth - (190) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].Y.v = PPTMainMonitor.MonitorHeight - (65) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].Width.v = (185) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].Height.v = (60) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].EllipseWidth.v = (30) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].EllipseHeight.v = (30) * PPTUIScale;
					pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].FrameThickness.v = (1) * PPTUIScale;
					if (TotalSlides == -1)
					{
						SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].FillColor.v, 0);
						SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].FrameColor.v, 0);
					}
					else
					{
						SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].FillColor.v, 160);
						SetAlpha(pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].FrameColor.v, 160);
					}

					{
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].X.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].X.v + (5) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Y.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].Y.v + (5) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Width.v = (50) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Height.v = (50) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].EllipseWidth.v = (35) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].EllipseHeight.v = (35) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].FrameThickness.v = (1) * PPTUIScale;

						{
							pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_PreviousPage].X.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].X.v + (5) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_PreviousPage].Y.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Y.v + (5) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_PreviousPage].Width.v = (40) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_PreviousPage].Height.v = (40) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_PreviousPage].Transparency.v = 255;
							pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_PreviousPage].Img = PptIconBitmap[1];
						}
					}
					{
						pptUiWordsWidgetTarget[PptUiWordsWidgetID::RightSide_PageNum].Left.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].X.v + pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Width.v + (5) * PPTUIScale;
						pptUiWordsWidgetTarget[PptUiWordsWidgetID::RightSide_PageNum].Top.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].Y.v + (5) * PPTUIScale;
						pptUiWordsWidgetTarget[PptUiWordsWidgetID::RightSide_PageNum].Right.v = pptUiWordsWidgetTarget[PptUiWordsWidgetID::RightSide_PageNum].Left.v + (65) * PPTUIScale;
						pptUiWordsWidgetTarget[PptUiWordsWidgetID::RightSide_PageNum].Bottom.v = pptUiWordsWidgetTarget[PptUiWordsWidgetID::RightSide_PageNum].Top.v + (55) * PPTUIScale;
						pptUiWordsWidgetTarget[PptUiWordsWidgetID::RightSide_PageNum].WordsHeight.v = (20) * PPTUIScale;
						SetAlpha(pptUiWordsWidgetTarget[PptUiWordsWidgetID::RightSide_PageNum].WordsColor.v, 255);
						{
							pptUiWordsWidgetTarget[PptUiWordsWidgetID::RightSide_PageNum].WordsContent = pptUiWordsWidgetTarget[PptUiWordsWidgetID::LeftSide_PageNum].WordsContent;
						}
					}
					{
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].X.v = pptUiWordsWidgetTarget[PptUiWordsWidgetID::RightSide_PageNum].Right.v + (5) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].Y.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget].Y.v + (5) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].Width.v = (50) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].Height.v = (50) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].EllipseWidth.v = (35) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].EllipseHeight.v = (35) * PPTUIScale;
						pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].FrameThickness.v = (1) * PPTUIScale;

						{
							pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_NextPage].X.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].X.v + (5) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_NextPage].Y.v = pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].Y.v + (5) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_NextPage].Width.v = (40) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_NextPage].Height.v = (40) * PPTUIScale;
							pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_NextPage].Transparency.v = 255;
							if (CurrentSlides == -1) pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_NextPage].Img = PptIconBitmap[3];
							else pptUiImageWidgetTarget[PptUiImageWidgetID::RightSide_NextPage].Img = PptIconBitmap[2];
						}
					}
				}
			}

			// UI 变化调整
			// 矩形
			for (int i = 0; i < size(pptUiRoundRectWidgetTarget); i++)
			{
				if (pptUiRoundRectWidget[i].X.v != pptUiRoundRectWidgetTarget[i].X.v)
				{
					PptUiWidgetValueTransformation(&pptUiRoundRectWidget[i].X.v, pptUiRoundRectWidgetTarget[i].X.v, pptUiRoundRectWidget[i].X.s, pptUiRoundRectWidget[i].X.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiRoundRectWidget[i].Y.v != pptUiRoundRectWidgetTarget[i].Y.v)
				{
					PptUiWidgetValueTransformation(&pptUiRoundRectWidget[i].Y.v, pptUiRoundRectWidgetTarget[i].Y.v, pptUiRoundRectWidget[i].Y.s, pptUiRoundRectWidget[i].Y.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiRoundRectWidget[i].Width.v != pptUiRoundRectWidgetTarget[i].Width.v)
				{
					PptUiWidgetValueTransformation(&pptUiRoundRectWidget[i].Width.v, pptUiRoundRectWidgetTarget[i].Width.v, pptUiRoundRectWidget[i].Width.s, pptUiRoundRectWidget[i].Width.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiRoundRectWidget[i].Height.v != pptUiRoundRectWidgetTarget[i].Height.v)
				{
					PptUiWidgetValueTransformation(&pptUiRoundRectWidget[i].Height.v, pptUiRoundRectWidgetTarget[i].Height.v, pptUiRoundRectWidget[i].Height.s, pptUiRoundRectWidget[i].Height.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiRoundRectWidget[i].EllipseWidth.v != pptUiRoundRectWidgetTarget[i].EllipseWidth.v)
				{
					PptUiWidgetValueTransformation(&pptUiRoundRectWidget[i].EllipseWidth.v, pptUiRoundRectWidgetTarget[i].EllipseWidth.v, pptUiRoundRectWidget[i].EllipseWidth.s, pptUiRoundRectWidget[i].EllipseWidth.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiRoundRectWidget[i].EllipseHeight.v != pptUiRoundRectWidgetTarget[i].EllipseHeight.v)
				{
					PptUiWidgetValueTransformation(&pptUiRoundRectWidget[i].EllipseHeight.v, pptUiRoundRectWidgetTarget[i].EllipseHeight.v, pptUiRoundRectWidget[i].EllipseHeight.s, pptUiRoundRectWidget[i].EllipseHeight.e);
					PptWindowBackgroundUiChange = true;
				}

				if (pptUiRoundRectWidget[i].FrameThickness.v != pptUiRoundRectWidgetTarget[i].FrameThickness.v)
				{
					PptUiWidgetValueTransformation(&pptUiRoundRectWidget[i].FrameThickness.v, pptUiRoundRectWidgetTarget[i].FrameThickness.v, pptUiRoundRectWidget[i].FrameThickness.s, pptUiRoundRectWidget[i].FrameThickness.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiRoundRectWidget[i].FrameColor.v != pptUiRoundRectWidgetTarget[i].FrameColor.v)
				{
					PptUiWidgetColorTransformation(&pptUiRoundRectWidget[i].FrameColor.v, pptUiRoundRectWidgetTarget[i].FrameColor.v, pptUiRoundRectWidget[i].FrameColor.s, pptUiRoundRectWidget[i].FrameColor.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiRoundRectWidget[i].FillColor.v != pptUiRoundRectWidgetTarget[i].FillColor.v)
				{
					PptUiWidgetColorTransformation(&pptUiRoundRectWidget[i].FillColor.v, pptUiRoundRectWidgetTarget[i].FillColor.v, pptUiRoundRectWidget[i].FillColor.s, pptUiRoundRectWidget[i].FillColor.e);
					PptWindowBackgroundUiChange = true;
				}
			}
			// 图像
			for (int i = 0; i < size(pptUiImageWidgetTarget); i++)
			{
				if (pptUiImageWidget[i].X.v != pptUiImageWidgetTarget[i].X.v)
				{
					PptUiWidgetValueTransformation(&pptUiImageWidget[i].X.v, pptUiImageWidgetTarget[i].X.v, pptUiImageWidget[i].X.s, pptUiImageWidget[i].X.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiImageWidget[i].Y.v != pptUiImageWidgetTarget[i].Y.v)
				{
					PptUiWidgetValueTransformation(&pptUiImageWidget[i].Y.v, pptUiImageWidgetTarget[i].Y.v, pptUiImageWidget[i].Y.s, pptUiImageWidget[i].Y.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiImageWidget[i].Width.v != pptUiImageWidgetTarget[i].Width.v)
				{
					PptUiWidgetValueTransformation(&pptUiImageWidget[i].Width.v, pptUiImageWidgetTarget[i].Width.v, pptUiImageWidget[i].Width.s, pptUiImageWidget[i].Width.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiImageWidget[i].Height.v != pptUiImageWidgetTarget[i].Height.v)
				{
					PptUiWidgetValueTransformation(&pptUiImageWidget[i].Height.v, pptUiImageWidgetTarget[i].Height.v, pptUiImageWidget[i].Height.s, pptUiImageWidget[i].Height.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiImageWidget[i].Transparency.v != pptUiImageWidgetTarget[i].Transparency.v)
				{
					PptUiWidgetValueTransformation(&pptUiImageWidget[i].Transparency.v, pptUiImageWidgetTarget[i].Transparency.v, pptUiImageWidget[i].Transparency.s, pptUiImageWidget[i].Transparency.e);
					PptWindowBackgroundUiChange = true;
				}

				if (pptUiImageWidget[i].Img != pptUiImageWidgetTarget[i].Img)
				{
					pptUiImageWidget[i].Img = pptUiImageWidgetTarget[i].Img;
					PptWindowBackgroundUiChange = true;
				}
			}
			// 文字
			for (int i = 0; i < size(pptUiWordsWidgetTarget); i++)
			{
				if (pptUiWordsWidget[i].Left.v != pptUiWordsWidgetTarget[i].Left.v)
				{
					PptUiWidgetValueTransformation(&pptUiWordsWidget[i].Left.v, pptUiWordsWidgetTarget[i].Left.v, pptUiWordsWidget[i].Left.s, pptUiWordsWidget[i].Left.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiWordsWidget[i].Top.v != pptUiWordsWidgetTarget[i].Top.v)
				{
					PptUiWidgetValueTransformation(&pptUiWordsWidget[i].Top.v, pptUiWordsWidgetTarget[i].Top.v, pptUiWordsWidget[i].Top.s, pptUiWordsWidget[i].Top.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiWordsWidget[i].Right.v != pptUiWordsWidgetTarget[i].Right.v)
				{
					PptUiWidgetValueTransformation(&pptUiWordsWidget[i].Right.v, pptUiWordsWidgetTarget[i].Right.v, pptUiWordsWidget[i].Right.s, pptUiWordsWidget[i].Right.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiWordsWidget[i].Bottom.v != pptUiWordsWidgetTarget[i].Bottom.v)
				{
					PptUiWidgetValueTransformation(&pptUiWordsWidget[i].Bottom.v, pptUiWordsWidgetTarget[i].Bottom.v, pptUiWordsWidget[i].Bottom.s, pptUiWordsWidget[i].Bottom.e);
					PptWindowBackgroundUiChange = true;
				}

				if (pptUiWordsWidget[i].WordsHeight.v != pptUiWordsWidgetTarget[i].WordsHeight.v)
				{
					PptUiWidgetValueTransformation(&pptUiWordsWidget[i].WordsHeight.v, pptUiWordsWidgetTarget[i].WordsHeight.v, pptUiWordsWidget[i].WordsHeight.s, pptUiWordsWidget[i].WordsHeight.e);
					PptWindowBackgroundUiChange = true;
				}
				if (pptUiWordsWidget[i].WordsColor.v != pptUiWordsWidgetTarget[i].WordsColor.v)
				{
					PptUiWidgetColorTransformation(&pptUiWordsWidget[i].WordsColor.v, pptUiWordsWidgetTarget[i].WordsColor.v, pptUiWordsWidget[i].WordsColor.s, pptUiWordsWidget[i].WordsColor.e);
					PptWindowBackgroundUiChange = true;
				}

				if (pptUiWordsWidget[i].WordsContent != pptUiWordsWidgetTarget[i].WordsContent)
				{
					pptUiWordsWidget[i].WordsContent = pptUiWordsWidgetTarget[i].WordsContent;
					PptWindowBackgroundUiChange = true;
				}
			}
		}

		// 控件加载部分
		if (!Initialization && TotalSlides != -1)
		{
			ppt_show = GetPptShow();

			std::wstringstream ss(GetPptTitle());
			getline(ss, ppt_title);
			getline(ss, ppt_software);

			if (ppt_software.find(L"WPS") != ppt_software.npos) ppt_software = L"WPS";
			else ppt_software = L"PowerPoint";

			if (!ppt_title_recond[ppt_title]) FreezePPT = true;
			Initialization = true;
		}
		else if (Initialization && TotalSlides == -1)
		{
			PptImg.IsSave = false;
			PptImg.IsSaved.clear();
			PptImg.Image.clear();

			ppt_show = NULL, ppt_software = L"";

			FreezePPT = false;
			Initialization = false;
		}

		// 绘制部分
		if (PptWindowBackgroundUiChange)
		{
			SetImageColor(PptWindowBackground, RGBA(0, 0, 0, 0), true);

			DCRenderTarget->BeginDraw();

			// 左侧控件
			{
				// pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget]
				{
					ID2D1SolidColorBrush* pFrameBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].FrameColor.v), &pFrameBrush);
					ID2D1SolidColorBrush* pFillBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].FillColor.v), &pFillBrush);

					D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].X.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].X.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].X.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Width.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].X.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].Height.v),
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].EllipseWidth.v / 2.0f,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].EllipseHeight.v / 2.0f
					);

					DCRenderTarget->FillRoundedRectangle(&roundedRect, pFillBrush);
					DCRenderTarget->DrawRoundedRectangle(&roundedRect, pFrameBrush, pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget].FrameThickness.v);

					DxObjectSafeRelease(&pFrameBrush);
					DxObjectSafeRelease(&pFillBrush);
				}

				// pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage]
				{
					ID2D1SolidColorBrush* pFrameBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].FrameColor.v), &pFrameBrush);
					ID2D1SolidColorBrush* pFillBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].FillColor.v), &pFillBrush);

					D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].X.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Y.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].X.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Width.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Y.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].Height.v),
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].EllipseWidth.v / 2.0f,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].EllipseHeight.v / 2.0f
					);

					DCRenderTarget->FillRoundedRectangle(&roundedRect, pFillBrush);
					DCRenderTarget->DrawRoundedRectangle(&roundedRect, pFrameBrush, pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage].FrameThickness.v);

					DxObjectSafeRelease(&pFrameBrush);
					DxObjectSafeRelease(&pFillBrush);
				}
				// pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage]
				{
					DCRenderTarget->DrawBitmap(pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].Img, D2D1::RectF(pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].X.v, pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].Y.v, pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].X.v + pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].Width.v, pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].Y.v + pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].Height.v), pptUiImageWidget[PptUiImageWidgetID::LeftSide_PreviousPage].Transparency.v / 255.0f);
				}

				// pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum]
				{
					IDWriteTextFormat* textFormat = NULL;

					D2DTextFactory->CreateTextFormat(
						L"HarmonyOS Sans SC",
						D2DFontCollection,
						DWRITE_FONT_WEIGHT_NORMAL,
						DWRITE_FONT_STYLE_NORMAL,
						DWRITE_FONT_STRETCH_NORMAL,
						pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].WordsHeight.v,
						L"zh-cn",
						&textFormat
					);

					ID2D1SolidColorBrush* pBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(D2D1::ColorF(ConvertToD2DColor(pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].WordsColor.v)), &pBrush);

					textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
					textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

					DCRenderTarget->DrawText(
						pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].WordsContent.c_str(),  // 文本
						wcslen(pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].WordsContent.c_str()),  // 文本长度
						textFormat,  // 文本格式
						D2D1::RectF(
							pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].Left.v,
							pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].Top.v,
							pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].Right.v,
							pptUiWordsWidget[PptUiWordsWidgetID::LeftSide_PageNum].Bottom.v
						),
						pBrush
					);

					DxObjectSafeRelease(&pBrush);
				}

				// pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage]
				{
					ID2D1SolidColorBrush* pFrameBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].FrameColor.v), &pFrameBrush);
					ID2D1SolidColorBrush* pFillBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].FillColor.v), &pFillBrush);

					D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].X.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].Y.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].X.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].Width.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].Y.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].Height.v),
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].EllipseWidth.v / 2.0f,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].EllipseHeight.v / 2.0f
					);

					DCRenderTarget->FillRoundedRectangle(&roundedRect, pFillBrush);
					DCRenderTarget->DrawRoundedRectangle(&roundedRect, pFrameBrush, pptUiRoundRectWidget[PptUiRoundRectWidgetID::LeftSide_PageWidget_NextPage].FrameThickness.v);

					DxObjectSafeRelease(&pFrameBrush);
					DxObjectSafeRelease(&pFillBrush);
				}
				// pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage]
				{
					DCRenderTarget->DrawBitmap(pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].Img, D2D1::RectF(pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].X.v, pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].Y.v, pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].X.v + pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].Width.v, pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].Y.v + pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].Height.v), pptUiImageWidget[PptUiImageWidgetID::LeftSide_NextPage].Transparency.v / 255.0f);
				}
			}
			// 中间控件
			{
				// pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget]
				{
					ID2D1SolidColorBrush* pFrameBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].FrameColor.v), &pFrameBrush);
					ID2D1SolidColorBrush* pFillBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].FillColor.v), &pFillBrush);

					D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].X.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].Y.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].X.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].Width.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].Y.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].Height.v),
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].EllipseWidth.v / 2.0f,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].EllipseHeight.v / 2.0f
					);

					DCRenderTarget->FillRoundedRectangle(&roundedRect, pFillBrush);
					DCRenderTarget->DrawRoundedRectangle(&roundedRect, pFrameBrush, pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget].FrameThickness.v);

					DxObjectSafeRelease(&pFrameBrush);
					DxObjectSafeRelease(&pFillBrush);
				}

				// pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow]
				{
					ID2D1SolidColorBrush* pFrameBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].FrameColor.v), &pFrameBrush);
					ID2D1SolidColorBrush* pFillBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].FillColor.v), &pFillBrush);

					D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].X.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Y.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].X.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Width.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Y.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].Height.v),
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].EllipseWidth.v / 2.0f,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].EllipseHeight.v / 2.0f
					);

					DCRenderTarget->FillRoundedRectangle(&roundedRect, pFillBrush);
					DCRenderTarget->DrawRoundedRectangle(&roundedRect, pFrameBrush, pptUiRoundRectWidget[PptUiRoundRectWidgetID::MiddleSide_TabSlideWidget_EndShow].FrameThickness.v);

					DxObjectSafeRelease(&pFrameBrush);
					DxObjectSafeRelease(&pFillBrush);
				}
				// pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow]
				{
					DCRenderTarget->DrawBitmap(pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].Img, D2D1::RectF(pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].X.v, pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].Y.v, pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].X.v + pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].Width.v, pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].Y.v + pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].Height.v), pptUiImageWidget[PptUiImageWidgetID::MiddleSide_EndShow].Transparency.v / 255.0f);
				}
			}
			// 右侧控件
			{
				// pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget]
				{
					ID2D1SolidColorBrush* pFrameBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].FrameColor.v), &pFrameBrush);
					ID2D1SolidColorBrush* pFillBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].FillColor.v), &pFillBrush);

					D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].X.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].Y.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].X.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].Width.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].Y.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].Height.v),
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].EllipseWidth.v / 2.0f,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].EllipseHeight.v / 2.0f
					);

					DCRenderTarget->FillRoundedRectangle(&roundedRect, pFillBrush);
					DCRenderTarget->DrawRoundedRectangle(&roundedRect, pFrameBrush, pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget].FrameThickness.v);

					DxObjectSafeRelease(&pFrameBrush);
					DxObjectSafeRelease(&pFillBrush);
				}

				// pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage]
				{
					ID2D1SolidColorBrush* pFrameBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].FrameColor.v), &pFrameBrush);
					ID2D1SolidColorBrush* pFillBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].FillColor.v), &pFillBrush);

					D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].X.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Y.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].X.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Width.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Y.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].Height.v),
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].EllipseWidth.v / 2.0f,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].EllipseHeight.v / 2.0f
					);

					DCRenderTarget->FillRoundedRectangle(&roundedRect, pFillBrush);
					DCRenderTarget->DrawRoundedRectangle(&roundedRect, pFrameBrush, pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_PreviousPage].FrameThickness.v);

					DxObjectSafeRelease(&pFrameBrush);
					DxObjectSafeRelease(&pFillBrush);
				}
				// pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage]
				{
					DCRenderTarget->DrawBitmap(pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].Img, D2D1::RectF(pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].X.v, pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].Y.v, pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].X.v + pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].Width.v, pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].Y.v + pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].Height.v), pptUiImageWidget[PptUiImageWidgetID::RightSide_PreviousPage].Transparency.v / 255.0f);
				}

				// pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum]
				{
					IDWriteTextFormat* textFormat = NULL;
					D2DTextFactory->CreateTextFormat(
						L"HarmonyOS Sans SC",
						D2DFontCollection,
						DWRITE_FONT_WEIGHT_NORMAL,
						DWRITE_FONT_STYLE_NORMAL,
						DWRITE_FONT_STRETCH_NORMAL,
						pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].WordsHeight.v,
						L"zh-cn",
						&textFormat
					);

					ID2D1SolidColorBrush* pBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(D2D1::ColorF(ConvertToD2DColor(pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].WordsColor.v)), &pBrush);

					textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
					textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

					DCRenderTarget->DrawText(
						pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].WordsContent.c_str(),  // 文本
						wcslen(pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].WordsContent.c_str()),  // 文本长度
						textFormat,  // 文本格式
						D2D1::RectF(
							pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].Left.v,
							pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].Top.v,
							pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].Right.v,
							pptUiWordsWidget[PptUiWordsWidgetID::RightSide_PageNum].Bottom.v
						),
						pBrush
					);

					DxObjectSafeRelease(&pBrush);
				}

				// pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage]
				{
					ID2D1SolidColorBrush* pFrameBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].FrameColor.v), &pFrameBrush);
					ID2D1SolidColorBrush* pFillBrush = NULL;
					DCRenderTarget->CreateSolidColorBrush(ConvertToD2DColor(pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].FillColor.v), &pFillBrush);

					D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].X.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].Y.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].X.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].Width.v,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].Y.v + pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].Height.v),
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].EllipseWidth.v / 2.0f,
						pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].EllipseHeight.v / 2.0f
					);

					DCRenderTarget->FillRoundedRectangle(&roundedRect, pFillBrush);
					DCRenderTarget->DrawRoundedRectangle(&roundedRect, pFrameBrush, pptUiRoundRectWidget[PptUiRoundRectWidgetID::RightSide_PageWidget_NextPage].FrameThickness.v);

					DxObjectSafeRelease(&pFrameBrush);
					DxObjectSafeRelease(&pFillBrush);
				}
				// pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage]
				{
					DCRenderTarget->DrawBitmap(pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].Img, D2D1::RectF(pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].X.v, pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].Y.v, pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].X.v + pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].Width.v, pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].Y.v + pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].Height.v), pptUiImageWidget[PptUiImageWidgetID::RightSide_NextPage].Transparency.v / 255.0f);
				}
			}

			DCRenderTarget->EndDraw();

			{
				ulwi.hdcSrc = GetImageHDC(&PptWindowBackground);
				UpdateLayeredWindowIndirect(ppt_window, &ulwi);

				if (!IsShowWindow)
				{
					IdtWindowsIsVisible.pptWindow = true;
					//ShowWindow(ppt_window, SW_SHOW);

					IsShowWindow = true;
				}
				// 动态平衡帧率
				if (tRecord)
				{
					int delay = 1000 / 24 - (clock() - tRecord);
					if (delay > 0) std::this_thread::sleep_for(std::chrono::milliseconds(delay));
				}
				tRecord = clock();

				PptWindowBackgroundUiChange = false;
			}
		}
		else this_thread::sleep_for(chrono::milliseconds(100));
	}

	for (int r = 0; r < (int)size(PptIconBitmap); r++) DxObjectSafeRelease(&PptIconBitmap[r]);
	DxObjectSafeRelease(&DCRenderTarget);

	threadStatus[L"PptDraw"] = false;
}
void PptInteract()
{
	ExMessage m;
	int last_x = -1, last_y = -1;

	while (!offSignal)
	{
		if (PptInfoStateBuffer.TotalPage != -1)
		{
			hiex::getmessage_win32(&m, EM_MOUSE, ppt_window);
			if (PptInfoStateBuffer.TotalPage == -1)
			{
				hiex::flushmessage_win32(EM_MOUSE, ppt_window);
				continue;
			}

			// 左侧 上一页
			if (PptUiIsInRoundRect(m.x, m.y, pptUiRoundRectWidgetTarget[PptUiRoundRectWidgetID::LeftSide_PageWidget_PreviousPage]))
			{
				if (last_x != m.x || last_y != m.y) PPTUIControlColorTarget[L"RoundRect/RoundRectLeft1/fill"].v = PPTUIControlColor[L"RoundRect/RoundRectLeft1/fill"].v = RGBA(225, 225, 225, 255);
				else PPTUIControlColorTarget[L"RoundRect/RoundRectLeft1/fill"].v = RGBA(250, 250, 250, 160);
				PptWindowBackgroundUiChange = true;

				if (m.message == WM_LBUTTONDOWN)
				{
					SetForegroundWindow(ppt_show);

					PreviousPptSlides();
					PPTUIControlColor[L"RoundRect/RoundRectLeft1/fill"].v = RGBA(200, 200, 200, 255);

					std::chrono::high_resolution_clock::time_point KeyboardInteractionManipulated = std::chrono::high_resolution_clock::now();
					while (1)
					{
						if (!KeyBoradDown[VK_LBUTTON]) break;
						if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - KeyboardInteractionManipulated).count() >= 400)
						{
							PreviousPptSlides();
							PPTUIControlColor[L"RoundRect/RoundRectLeft1/fill"].v = RGBA(200, 200, 200, 255);
						}

						this_thread::sleep_for(chrono::milliseconds(15));
					}

					PPTUIControlColorTarget[L"RoundRect/RoundRectLeft1/fill"].v = RGBA(250, 250, 250, 160);
					hiex::flushmessage_win32(EM_MOUSE, ppt_window);

					POINT pt;
					GetCursorPos(&pt);
					last_x = pt.x, last_y = pt.y;
				}
			}
			else if (PptInfoStateBuffer.TotalPage != -1) PPTUIControlColorTarget[L"RoundRect/RoundRectLeft1/fill"].v = RGBA(250, 250, 250, 160);
			// 左侧 下一页
			if (IsInRect(m.x, m.y, { (int)PPTUIControl[L"RoundRect/RoundRectLeft2/x"].v,(int)PPTUIControl[L"RoundRect/RoundRectLeft2/y"].v,(int)PPTUIControl[L"RoundRect/RoundRectLeft2/x"].v + (int)PPTUIControl[L"RoundRect/RoundRectLeft2/width"].v,(int)PPTUIControl[L"RoundRect/RoundRectLeft2/y"].v + (int)PPTUIControl[L"RoundRect/RoundRectLeft2/height"].v }))
			{
				if (last_x != m.x || last_y != m.y) PPTUIControlColorTarget[L"RoundRect/RoundRectLeft2/fill"].v = PPTUIControlColor[L"RoundRect/RoundRectLeft2/fill"].v = RGBA(225, 225, 225, 255);
				else PPTUIControlColorTarget[L"RoundRect/RoundRectLeft2/fill"].v = RGBA(250, 250, 250, 160);
				PptWindowBackgroundUiChange = true;

				if (m.message == WM_LBUTTONDOWN)
				{
					int temp_currentpage = PptInfoState.CurrentPage;
					if (temp_currentpage == -1 && stateMode.StateModeSelect != StateModeSelectEnum::IdtSelection && penetrate.select == false)
					{
						if (MessageBox(floating_window, L"当前处于画板模式，结束放映将会清空画板内容。\n\n结束放映？", L"智绘教警告", MB_OKCANCEL | MB_ICONWARNING | MB_SYSTEMMODAL) == 1)
						{
							ChangeStateModeToSelection();
						}
					}
					else if (temp_currentpage == -1)
					{
						EndPptShow();
					}
					else
					{
						SetForegroundWindow(ppt_show);

						NextPptSlides(temp_currentpage);
						PPTUIControlColor[L"RoundRect/RoundRectLeft2/fill"].v = RGBA(200, 200, 200, 255);

						std::chrono::high_resolution_clock::time_point KeyboardInteractionManipulated = std::chrono::high_resolution_clock::now();
						while (1)
						{
							if (!KeyBoradDown[VK_LBUTTON]) break;

							if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - KeyboardInteractionManipulated).count() >= 400)
							{
								temp_currentpage = PptInfoState.CurrentPage;
								if (temp_currentpage == -1 && stateMode.StateModeSelect != StateModeSelectEnum::IdtSelection && penetrate.select == false)
								{
									if (MessageBox(floating_window, L"当前处于画板模式，结束放映将会清空画板内容。\n\n结束放映？", L"智绘教警告", MB_OKCANCEL | MB_ICONWARNING | MB_SYSTEMMODAL) == 1)
									{
										EndPptShow();

										ChangeStateModeToSelection();
									}
									break;
								}
								else if (temp_currentpage != -1)
								{
									NextPptSlides(temp_currentpage);
									PPTUIControlColor[L"RoundRect/RoundRectLeft2/fill"].v = RGBA(200, 200, 200, 255);
								}
							}

							this_thread::sleep_for(chrono::milliseconds(15));
						}
					}

					PPTUIControlColorTarget[L"RoundRect/RoundRectLeft2/fill"].v = RGBA(250, 250, 250, 160);
					hiex::flushmessage_win32(EM_MOUSE, ppt_window);

					POINT pt;
					GetCursorPos(&pt);
					last_x = pt.x, last_y = pt.y;
				}
			}
			else if (PptInfoStateBuffer.TotalPage != -1) PPTUIControlColorTarget[L"RoundRect/RoundRectLeft2/fill"].v = RGBA(250, 250, 250, 160);

			// 中间 结束放映
			if (IsInRect(m.x, m.y, { (int)PPTUIControl[L"RoundRect/RoundRectMiddleLeft1/x"].v,(int)PPTUIControl[L"RoundRect/RoundRectMiddleLeft1/y"].v,(int)PPTUIControl[L"RoundRect/RoundRectMiddleLeft1/x"].v + (int)PPTUIControl[L"RoundRect/RoundRectMiddleLeft1/width"].v,(int)PPTUIControl[L"RoundRect/RoundRectMiddleLeft1/y"].v + (int)PPTUIControl[L"RoundRect/RoundRectMiddleLeft1/height"].v }))
			{
				if (last_x != m.x || last_y != m.y) PPTUIControlColorTarget[L"RoundRect/RoundRectMiddleLeft1/fill"].v = PPTUIControlColor[L"RoundRect/RoundRectMiddleLeft1/fill"].v = RGBA(225, 225, 225, 255);
				else PPTUIControlColorTarget[L"RoundRect/RoundRectMiddleLeft1/fill"].v = RGBA(250, 250, 250, 160);
				PptWindowBackgroundUiChange = true;

				if (m.message == WM_LBUTTONDOWN)
				{
					int lx = m.x, ly = m.y;
					while (1)
					{
						ExMessage m = hiex::getmessage_win32(EM_MOUSE, ppt_window);
						if (IsInRect(m.x, m.y, { (int)PPTUIControl[L"RoundRect/RoundRectMiddleLeft1/x"].v,(int)PPTUIControl[L"RoundRect/RoundRectMiddleLeft1/y"].v,(int)PPTUIControl[L"RoundRect/RoundRectMiddleLeft1/x"].v + (int)PPTUIControl[L"RoundRect/RoundRectMiddleLeft1/width"].v,(int)PPTUIControl[L"RoundRect/RoundRectMiddleLeft1/y"].v + (int)PPTUIControl[L"RoundRect/RoundRectMiddleLeft1/height"].v }))
						{
							if (!m.lbutton)
							{
								PPTUIControlColor[L"RoundRect/RoundRectMiddleLeft1/fill"].v = RGBA(200, 200, 200, 255);

								if (stateMode.StateModeSelect != StateModeSelectEnum::IdtSelection && penetrate.select == false)
								{
									if (MessageBox(floating_window, L"当前处于画板模式，结束放映将会清空画板内容。\n\n结束放映？", L"智绘教警告", MB_OKCANCEL | MB_ICONWARNING | MB_SYSTEMMODAL) != 1) break;

									ChangeStateModeToSelection();
								}

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

					POINT pt;
					GetCursorPos(&pt);
					last_x = pt.x, last_y = pt.y;
				}
			}
			else if (PptInfoStateBuffer.TotalPage != -1) PPTUIControlColorTarget[L"RoundRect/RoundRectMiddleLeft1/fill"].v = RGBA(250, 250, 250, 160);

			// 右侧 上一页
			if (IsInRect(m.x, m.y, { (int)PPTUIControl[L"RoundRect/RoundRectRight1/x"].v,(int)PPTUIControl[L"RoundRect/RoundRectRight1/y"].v,(int)PPTUIControl[L"RoundRect/RoundRectRight1/x"].v + (int)PPTUIControl[L"RoundRect/RoundRectRight1/width"].v,(int)PPTUIControl[L"RoundRect/RoundRectRight1/y"].v + (int)PPTUIControl[L"RoundRect/RoundRectRight1/height"].v }))
			{
				if (last_x != m.x || last_y != m.y) PPTUIControlColorTarget[L"RoundRect/RoundRectRight1/fill"].v = PPTUIControlColor[L"RoundRect/RoundRectRight1/fill"].v = RGBA(225, 225, 225, 255);
				else PPTUIControlColorTarget[L"RoundRect/RoundRectRight1/fill"].v = RGBA(250, 250, 250, 160);
				PptWindowBackgroundUiChange = true;

				if (m.message == WM_LBUTTONDOWN)
				{
					SetForegroundWindow(ppt_show);

					PreviousPptSlides();
					PPTUIControlColor[L"RoundRect/RoundRectRight1/fill"].v = RGBA(200, 200, 200, 255);

					std::chrono::high_resolution_clock::time_point KeyboardInteractionManipulated = std::chrono::high_resolution_clock::now();
					while (1)
					{
						if (!KeyBoradDown[VK_LBUTTON]) break;
						if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - KeyboardInteractionManipulated).count() >= 400)
						{
							PreviousPptSlides();
							PPTUIControlColor[L"RoundRect/RoundRectRight1/fill"].v = RGBA(200, 200, 200, 255);
						}

						this_thread::sleep_for(chrono::milliseconds(15));
					}

					PPTUIControlColorTarget[L"RoundRect/RoundRectRight1/fill"].v = RGBA(250, 250, 250, 160);
					hiex::flushmessage_win32(EM_MOUSE, ppt_window);

					POINT pt;
					GetCursorPos(&pt);
					last_x = pt.x, last_y = pt.y;
				}
			}
			else if (PptInfoStateBuffer.TotalPage != -1) PPTUIControlColorTarget[L"RoundRect/RoundRectRight1/fill"].v = RGBA(250, 250, 250, 160);
			// 右侧 下一页
			if (IsInRect(m.x, m.y, { (int)PPTUIControl[L"RoundRect/RoundRectRight2/x"].v,(int)PPTUIControl[L"RoundRect/RoundRectRight2/y"].v,(int)PPTUIControl[L"RoundRect/RoundRectRight2/x"].v + (int)PPTUIControl[L"RoundRect/RoundRectRight2/width"].v,(int)PPTUIControl[L"RoundRect/RoundRectRight2/y"].v + (int)PPTUIControl[L"RoundRect/RoundRectRight2/height"].v }))
			{
				if (last_x != m.x || last_y != m.y) PPTUIControlColorTarget[L"RoundRect/RoundRectRight2/fill"].v = PPTUIControlColor[L"RoundRect/RoundRectRight2/fill"].v = RGBA(225, 225, 225, 255);
				else PPTUIControlColorTarget[L"RoundRect/RoundRectRight2/fill"].v = RGBA(250, 250, 250, 160);
				PptWindowBackgroundUiChange = true;

				if (m.message == WM_LBUTTONDOWN)
				{
					int temp_currentpage = PptInfoState.CurrentPage;
					if (temp_currentpage == -1 && stateMode.StateModeSelect != StateModeSelectEnum::IdtSelection && penetrate.select == false)
					{
						if (MessageBox(floating_window, L"当前处于画板模式，结束放映将会清空画板内容。\n\n结束放映？", L"智绘教警告", MB_OKCANCEL | MB_ICONWARNING | MB_SYSTEMMODAL) == 1)
						{
							EndPptShow();

							ChangeStateModeToSelection();
						}
					}
					else if (temp_currentpage == -1)
					{
						EndPptShow();
					}
					else
					{
						SetForegroundWindow(ppt_show);

						NextPptSlides(temp_currentpage);
						PPTUIControlColor[L"RoundRect/RoundRectRight2/fill"].v = RGBA(200, 200, 200, 255);

						std::chrono::high_resolution_clock::time_point KeyboardInteractionManipulated = std::chrono::high_resolution_clock::now();
						while (1)
						{
							if (!KeyBoradDown[VK_LBUTTON]) break;

							if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - KeyboardInteractionManipulated).count() >= 400)
							{
								temp_currentpage = PptInfoState.CurrentPage;
								if (temp_currentpage == -1 && stateMode.StateModeSelect != StateModeSelectEnum::IdtSelection && penetrate.select == false)
								{
									if (MessageBox(floating_window, L"当前处于画板模式，结束放映将会清空画板内容。\n\n结束放映？", L"智绘教警告", MB_OKCANCEL | MB_ICONWARNING | MB_SYSTEMMODAL) == 1)
									{
										EndPptShow();

										ChangeStateModeToSelection();
									}
									break;
								}
								else if (temp_currentpage != -1)
								{
									NextPptSlides(temp_currentpage);
									PPTUIControlColor[L"RoundRect/RoundRectRight2/fill"].v = RGBA(200, 200, 200, 255);
								}
							}

							this_thread::sleep_for(chrono::milliseconds(15));
						}
					}

					PPTUIControlColorTarget[L"RoundRect/RoundRectRight2/fill"].v = RGBA(250, 250, 250, 160);
					hiex::flushmessage_win32(EM_MOUSE, ppt_window);

					POINT pt;
					GetCursorPos(&pt);
					last_x = pt.x, last_y = pt.y;
				}
			}
			else if (PptInfoStateBuffer.TotalPage != -1) PPTUIControlColorTarget[L"RoundRect/RoundRectRight2/fill"].v = RGBA(250, 250, 250, 160);

			// 滚轮消息
			if (m.message == WM_MOUSEWHEEL)
			{
				// 下一页
				if (m.wheel <= -120)
				{
					int temp_currentpage = PptInfoState.CurrentPage;
					if (temp_currentpage == -1 && stateMode.StateModeSelect != StateModeSelectEnum::IdtSelection && penetrate.select == false)
					{
						if (MessageBox(floating_window, L"当前处于画板模式，结束放映将会清空画板内容。\n\n结束放映？", L"智绘教警告", MB_OKCANCEL | MB_ICONWARNING | MB_SYSTEMMODAL) == 1)
						{
							EndPptShow();

							ChangeStateModeToSelection();
						}
					}
					else if (temp_currentpage == -1)
					{
						EndPptShow();
					}
					else
					{
						SetForegroundWindow(ppt_show);

						NextPptSlides(temp_currentpage);
						PPTUIControlColor[L"RoundRect/RoundRectRight2/fill"].v = RGBA(200, 200, 200, 255);
					}

					PPTUIControlColorTarget[L"RoundRect/RoundRectRight2/fill"].v = RGBA(250, 250, 250, 160);
					hiex::flushmessage_win32(EM_MOUSE, ppt_window);
				}
				// 上一页
				else
				{
					SetForegroundWindow(ppt_show);

					PreviousPptSlides();
					PPTUIControlColor[L"RoundRect/RoundRectRight1/fill"].v = RGBA(200, 200, 200, 255);

					PPTUIControlColorTarget[L"RoundRect/RoundRectRight1/fill"].v = RGBA(250, 250, 250, 160);
					hiex::flushmessage_win32(EM_MOUSE, ppt_window);
				}
			}
		}
		else
		{
			last_x = -1, last_y = -1;

			hiex::flushmessage_win32(EM_MOUSE, ppt_window);
			this_thread::sleep_for(chrono::milliseconds(500));
		}
	}
}
void PPTLinkageMain()
{
	threadStatus[L"PptDraw"] = true;

	thread GetPptState_thread(GetPptState);
	GetPptState_thread.detach();

	thread DrawControlWindowThread(PptDraw);
	DrawControlWindowThread.detach();
	thread ControlManipulationThread(PptInteract);
	ControlManipulationThread.detach();

	while (!offSignal) this_thread::sleep_for(chrono::milliseconds(500));

	int i = 1;
	for (; i <= 5; i++)
	{
		if (!threadStatus[L"GetPptState"] && !threadStatus[L"PptDraw"]) break;
		this_thread::sleep_for(chrono::milliseconds(500));
	}

	threadStatus[L"PptDraw"] = false;
}

// --------------------------------------------------
// 插件

// DesktopDrawpadBlocker 插件
void StartDesktopDrawpadBlocker()
{
	if (ddbSetList.DdbEnable)
	{
		// 配置 json
		{
			if (_waccess((StringToWstring(globalPath) + L"PlugIn\\DDB\\interaction_configuration.json").c_str(), 0) == 0) DdbReadSetting();

			ddbSetList.hostPath = GetCurrentExePath();
			if (ddbSetList.DdbEnhance)
			{
				ddbSetList.mode = 0;
				ddbSetList.restartHost = true;
			}
			else
			{
				ddbSetList.mode = 1;
				ddbSetList.restartHost = true;
			}
		}

		// 配置 EXE
		if (_waccess((StringToWstring(globalPath) + L"PlugIn\\DDB\\DesktopDrawpadBlocker.exe").c_str(), 0) == -1)
		{
			if (_waccess((StringToWstring(globalPath) + L"PlugIn\\DDB").c_str(), 0) == -1)
			{
				error_code ec;
				filesystem::create_directories(StringToWstring(globalPath) + L"PlugIn\\DDB", ec);
			}
			ExtractResource((StringToWstring(globalPath) + L"PlugIn\\DDB\\DesktopDrawpadBlocker.exe").c_str(), L"EXE", MAKEINTRESOURCE(237));
		}
		else
		{
			string hash_sha256;
			{
				hashwrapper* myWrapper = new sha256wrapper();
				hash_sha256 = myWrapper->getHashFromFileW(StringToWstring(globalPath) + L"PlugIn\\DDB\\DesktopDrawpadBlocker.exe");
				delete myWrapper;
			}

			if (hash_sha256 != ddbSetList.DdbSHA256)
			{
				if (isProcessRunning((StringToWstring(globalPath) + L"PlugIn\\DDB\\DesktopDrawpadBlocker.exe").c_str()))
				{
					// 需要关闭旧版 DDB 并更新版本

					DdbWriteSetting(true, true);
					for (int i = 1; i <= 20; i++)
					{
						if (!isProcessRunning((StringToWstring(globalPath) + L"PlugIn\\DDB\\DesktopDrawpadBlocker.exe").c_str()))
							break;
						this_thread::sleep_for(chrono::milliseconds(500));
					}
				}
				ExtractResource((StringToWstring(globalPath) + L"PlugIn\\DDB\\DesktopDrawpadBlocker.exe").c_str(), L"EXE", MAKEINTRESOURCE(237));
			}
		}

		// 创建开机自启标识
		if (ddbSetList.DdbEnhance && _waccess((StringToWstring(globalPath) + L"PlugIn\\DDB\\start_up.signal").c_str(), 0) == -1)
		{
			std::ofstream file((StringToWstring(globalPath) + L"PlugIn\\DDB\\start_up.signal").c_str());
			file.close();
		}
		// 移除开机自启标识
		else if (!ddbSetList.DdbEnhance && _waccess((StringToWstring(globalPath) + L"PlugIn\\DDB\\start_up.signal").c_str(), 0) == 0)
		{
			error_code ec;
			filesystem::remove(StringToWstring(globalPath) + L"PlugIn\\DDB\\start_up.signal", ec);
		}

		// 启动 DDB
		if (!isProcessRunning((StringToWstring(globalPath) + L"PlugIn\\DDB\\DesktopDrawpadBlocker.exe").c_str()))
		{
			DdbWriteSetting(true, false);
			ShellExecute(NULL, NULL, (StringToWstring(globalPath) + L"PlugIn\\DDB\\DesktopDrawpadBlocker.exe").c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
	}
	else if (_waccess((StringToWstring(globalPath) + L"PlugIn\\DDB").c_str(), 0) == 0)
	{
		error_code ec;
		filesystem::remove_all(StringToWstring(globalPath) + L"PlugIn\\DDB", ec);
	}
}