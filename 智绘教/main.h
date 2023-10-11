#pragma comment( linker, "/subsystem:windows /entry:mainCRTStartup" )
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <iostream>
#include <windows.h>
#include "HiEasyX.h"
#include <thread>
#include <tlhelp32.h>
#include <typeinfo>
#include <psapi.h>
#include <filesystem>
#include <Netlistmgr.h>
#include <Wininet.h>
#include <atlbase.h>
#include <rtscom.h>
#include <comutil.h>
#include <TlHelp32.h>
#include <ole2.h>
#include <utility>
#include <intrin.h>
#include <regex>
#include <dwmapi.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <VersionHelpers.h>

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include "stb_image/stb-master/stb_image_write.h"

#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")
using namespace Gdiplus;

#include <magnification.h>
#pragma comment(lib, "Magnification.lib")

#include <rtscom.h>
#include <rtscom_i.c>

#pragma comment(lib, "comsuppw.lib")

#include "json/reader.h"
#include "json/value.h"
#include "json/writer.h"

#include "hashlib2plus/hashlibpp.h"
#include "zip_utils/unzip.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wininet.lib")
#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "wbemuuid.lib")

//我们不开源服务器密钥，以防您纂改我们的服务器
#include "key.h"
using namespace std;

#import "PptCOM.tlb"
using namespace PptCOM;

#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0)
#define HiBeginDraw() BEGIN_TASK()
#define HiEndDraw() END_TASK(); REDRAW_WINDOW()
#define Sleep(int) this_thread::sleep_for(chrono::milliseconds(int))
#define Test() MessageBox(NULL, L"标记处", L"标记", MB_OK | MB_SYSTEMMODAL)
#define Testi(int) MessageBox(NULL, to_wstring(int).c_str(), L"数值标记", MB_OK | MB_SYSTEMMODAL)
#define Testw(wstring) MessageBox(NULL, wstring.c_str(), L"字符标记", MB_OK | MB_SYSTEMMODAL)

bool debug = false;
bool already = false;

wstring buildTime = __DATE__ L" " __TIME__; //构建时间
string edition_date = "20231011a"; //程序发布日期
string edition_code = "2310 - BUG Bash"; //程序版本
string server_feedback, server_code;

double server_updata_error, procedure_updata_error;
wstring server_updata_error_reason;

string global_path; //程序当前路径

wstring userid; //用户ID（主板序列号）

bool off_signal = false;
map <wstring, bool> thread_status; //线程状态管理

struct
{
	bool startup = true;
	bool experimental_functions = true;
}setlist;

///触摸 =====

int uRealTimeStylus;

bool touchDown = false;   // 表示触摸设备是否被按下
int touchNum = 0;         // 触摸点的点击个数

struct TouchMode
{
	POINT pt;
};
unordered_map<LONG, double> TouchSpeed;
unordered_map<LONG, TouchMode> TouchPos;
vector<LONG> TouchList;

struct TouchInfo
{
	LONG pid;
	POINT pt;
};
deque<TouchInfo> TouchTemp;

LONG TouchCnt = 0;
unordered_map<LONG, LONG> TouchPointer;
shared_mutex PointPosSm, TouchSpeedSm, PointListSm, PointTempSm;

IRealTimeStylus* g_pRealTimeStylus = NULL;
IStylusSyncPlugin* g_pSyncEventHandlerRTS = NULL;

IRealTimeStylus* CreateRealTimeStylus(HWND hWnd)
{
	// Check input argument
	if (hWnd == NULL)
	{
		//ASSERT(hWnd && L"CreateRealTimeStylus: invalid argument hWnd");
		return NULL;
	}

	// Create RTS object
	IRealTimeStylus* pRealTimeStylus = NULL;
	HRESULT hr = CoCreateInstance(CLSID_RealTimeStylus, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pRealTimeStylus));
	if (FAILED(hr))
	{
		//ASSERT(SUCCEEDED(hr) && L"CreateRealTimeStylus: failed to CoCreateInstance of RealTimeStylus");
		return NULL;
	}

	// Attach RTS object to a window
	hr = pRealTimeStylus->put_HWND((HANDLE_PTR)hWnd);
	if (FAILED(hr))
	{
		//ASSERT(SUCCEEDED(hr) && L"CreateRealTimeStylus: failed to set window handle");
		pRealTimeStylus->Release();
		return NULL;
	}

	// Register RTS object for receiving multi-touch input.
	IRealTimeStylus3* pRealTimeStylus3 = NULL;
	hr = pRealTimeStylus->QueryInterface(&pRealTimeStylus3);
	if (FAILED(hr))
	{
		//ASSERT(SUCCEEDED(hr) && L"CreateRealTimeStylus: cannot access IRealTimeStylus3");
		pRealTimeStylus->Release();
		return NULL;
	}
	hr = pRealTimeStylus3->put_MultiTouchEnabled(TRUE);
	if (FAILED(hr))
	{
		//ASSERT(SUCCEEDED(hr) && L"CreateRealTimeStylus: failed to enable multi-touch");
		pRealTimeStylus->Release();
		pRealTimeStylus3->Release();
		return NULL;
	}
	pRealTimeStylus3->Release();

	return pRealTimeStylus;
}
bool EnableRealTimeStylus(IRealTimeStylus* pRealTimeStylus)
{
	// Check input arguments
	if (pRealTimeStylus == NULL)
	{
		//ASSERT(pRealTimeStylus && L"EnableRealTimeStylus: invalid argument RealTimeStylus");
		return NULL;
	}

	// Enable RTS object
	HRESULT hr = pRealTimeStylus->put_Enabled(TRUE);
	if (FAILED(hr))
	{
		//ASSERT(SUCCEEDED(hr) && L"EnableRealTimeStylus: failed to enable RealTimeStylus");
		return false;
	}

	return true;
}

class CSyncEventHandlerRTS : public IStylusSyncPlugin
{
	CSyncEventHandlerRTS();
	virtual ~CSyncEventHandlerRTS();

public:
	// Factory method
	static IStylusSyncPlugin* Create(IRealTimeStylus* pRealTimeStylus);

	STDMETHOD(StylusDown)(IRealTimeStylus*, const StylusInfo* pStylusInfo, ULONG cPktCount, LONG* pPacket, LONG** ppInOutPkts);
	STDMETHOD(StylusUp)(IRealTimeStylus*, const StylusInfo* pStylusInfo, ULONG cPktCount, LONG* pPacket, LONG** ppInOutPkts);
	STDMETHOD(Packets)(IRealTimeStylus* piRtsSrc, const StylusInfo* pStylusInfo, ULONG cPktCount, ULONG cPktBuffLength, LONG* pPackets, ULONG* pcInOutPkts, LONG** ppInOutPkts);
	STDMETHOD(DataInterest)(RealTimeStylusDataInterest* pDataInterest);

	// IStylusSyncPlugin methods with trivial inline implementation, they all return S_OK
	STDMETHOD(RealTimeStylusEnabled)(IRealTimeStylus*, ULONG, const TABLET_CONTEXT_ID*) { return S_OK; }
	STDMETHOD(RealTimeStylusDisabled)(IRealTimeStylus*, ULONG, const TABLET_CONTEXT_ID*) { return S_OK; }
	STDMETHOD(StylusInRange)(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID) { return S_OK; }
	STDMETHOD(StylusOutOfRange)(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID) { return S_OK; }
	STDMETHOD(InAirPackets)(IRealTimeStylus*, const StylusInfo*, ULONG, ULONG, LONG*, ULONG*, LONG**) { return S_OK; }
	STDMETHOD(StylusButtonUp)(IRealTimeStylus*, STYLUS_ID, const GUID*, POINT*) { return S_OK; }
	STDMETHOD(StylusButtonDown)(IRealTimeStylus*, STYLUS_ID, const GUID*, POINT*) { return S_OK; }
	STDMETHOD(SystemEvent)(IRealTimeStylus*, TABLET_CONTEXT_ID, STYLUS_ID, SYSTEM_EVENT, SYSTEM_EVENT_DATA) { return S_OK; }
	STDMETHOD(TabletAdded)(IRealTimeStylus*, IInkTablet*) { return S_OK; }
	STDMETHOD(TabletRemoved)(IRealTimeStylus*, LONG) { return S_OK; }
	STDMETHOD(CustomStylusDataAdded)(IRealTimeStylus*, const GUID*, ULONG, const BYTE*) { return S_OK; }
	STDMETHOD(Error)(IRealTimeStylus*, IStylusPlugin*, RealTimeStylusDataInterest, HRESULT, LONG_PTR*) { return S_OK; }
	STDMETHOD(UpdateMapping)(IRealTimeStylus*) { return S_OK; }

	// IUnknown methods
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);

private:
	LONG m_cRefCount;                   // COM object reference count
	IUnknown* m_punkFTMarshaller;       // free-threaded marshaller
	int m_nContacts;                    // number of fingers currently in the contact with the touch digitizer
};
HRESULT CSyncEventHandlerRTS::StylusDown(IRealTimeStylus* piRtsSrc, const StylusInfo* pStylusInfo, ULONG /*cPktCount*/, LONG* pPacket, LONG** /*ppInOutPkts*/)
{
	uRealTimeStylus = 2;

	// 这是一个按下状态

	TABLET_CONTEXT_ID* pTcids;
	ULONG ulTcidCount;
	TABLET_CONTEXT_ID tcid;
	FLOAT fInkToDeviceScaleX;
	FLOAT fInkToDeviceScaleY;
	ULONG ulPacketProperties;
	PACKET_PROPERTY* pPacketProperties;

	piRtsSrc->GetAllTabletContextIds(&ulTcidCount, &pTcids);
	tcid = *pTcids;
	piRtsSrc->GetPacketDescriptionData(tcid, &fInkToDeviceScaleX, &fInkToDeviceScaleY, &ulPacketProperties, &pPacketProperties);

	TouchCnt++;
	TouchPointer[pStylusInfo->cid] = TouchCnt;

	TouchMode mode{};
	mode.pt.x = LONG(pPacket[0] * fInkToDeviceScaleX + 0.5);
	mode.pt.y = LONG(pPacket[1] * fInkToDeviceScaleY + 0.5);

	std::unique_lock<std::shared_mutex> lock1(PointPosSm);
	TouchPos[TouchCnt] = mode;
	lock1.unlock();

	std::unique_lock<std::shared_mutex> lock2(TouchSpeedSm);
	TouchSpeed[TouchCnt] = 1;
	lock2.unlock();

	std::unique_lock<std::shared_mutex> lock3(PointListSm);
	TouchList.push_back(TouchCnt);
	lock3.unlock();

	TouchInfo info{};
	info.pid = TouchCnt;
	info.pt = mode.pt;

	std::unique_lock<std::shared_mutex> lock4(PointTempSm);
	TouchTemp.push_back(info);
	lock4.unlock();

	TouchCnt %= 100000;

	touchNum++;
	touchDown = true;

	return S_OK;
}
HRESULT CSyncEventHandlerRTS::StylusUp(IRealTimeStylus*, const StylusInfo* pStylusInfo, ULONG /*cPktCount*/, LONG* pPacket, LONG** /*ppInOutPkts*/)
{
	uRealTimeStylus = 3;

	// 这是一个抬起状态

	touchNum = max(0, touchNum - 1);
	if (touchNum == 0) touchDown = false;

	auto it = std::find(TouchList.begin(), TouchList.end(), TouchPointer[pStylusInfo->cid]);
	if (it != TouchList.end())
	{
		std::unique_lock<std::shared_mutex> lock1(PointListSm);
		TouchList.erase(it);
		TouchPointer.erase(pStylusInfo->cid);
		lock1.unlock();
	}

	if (touchNum == 0)
	{
		std::unique_lock<std::shared_mutex> lock(PointPosSm);
		TouchPos.clear();
		lock.unlock();
	}

	return S_OK;
}
HRESULT CSyncEventHandlerRTS::Packets(IRealTimeStylus* piRtsSrc, const StylusInfo* pStylusInfo, ULONG /*cPktCount*/, ULONG /*cPktBuffLength*/, LONG* pPacket, ULONG* /*pcInOutPkts*/, LONG** /*ppInOutPkts*/)
{
	uRealTimeStylus = 4;

	// 这是一个移动状态

	TABLET_CONTEXT_ID* pTcids;
	ULONG ulTcidCount;
	TABLET_CONTEXT_ID tcid;
	FLOAT fInkToDeviceScaleX;
	FLOAT fInkToDeviceScaleY;
	ULONG ulPacketProperties;
	PACKET_PROPERTY* pPacketProperties;

	piRtsSrc->GetAllTabletContextIds(&ulTcidCount, &pTcids);
	tcid = *pTcids;
	piRtsSrc->GetPacketDescriptionData(tcid, &fInkToDeviceScaleX, &fInkToDeviceScaleY, &ulPacketProperties, &pPacketProperties);

	TouchMode mode{};
	mode.pt.x = LONG(pPacket[0] * fInkToDeviceScaleX + 0.5);
	mode.pt.y = LONG(pPacket[1] * fInkToDeviceScaleY + 0.5);

	std::unique_lock<std::shared_mutex> lock2(PointPosSm);
	TouchPos[TouchPointer[pStylusInfo->cid]] = mode;
	lock2.unlock();

	return S_OK;
}
HRESULT CSyncEventHandlerRTS::DataInterest(RealTimeStylusDataInterest* pDataInterest)
{
	*pDataInterest = (RealTimeStylusDataInterest)(RTSDI_StylusDown | RTSDI_Packets | RTSDI_StylusUp);

	return S_OK;
}

// CSyncEventHandlerRTS constructor.
CSyncEventHandlerRTS::CSyncEventHandlerRTS()
	: m_cRefCount(1),
	m_punkFTMarshaller(NULL),
	m_nContacts(0)
{
}
// CSyncEventHandlerRTS destructor.
CSyncEventHandlerRTS::~CSyncEventHandlerRTS()
{
	if (m_punkFTMarshaller != NULL)
	{
		m_punkFTMarshaller->Release();
	}
}

ULONG CSyncEventHandlerRTS::AddRef()
{
	return InterlockedIncrement(&m_cRefCount);
}
ULONG CSyncEventHandlerRTS::Release()
{
	ULONG cNewRefCount = InterlockedDecrement(&m_cRefCount);
	if (cNewRefCount == 0)
	{
		delete this;
	}
	return cNewRefCount;
}
HRESULT CSyncEventHandlerRTS::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
	if ((riid == IID_IStylusSyncPlugin) || (riid == IID_IUnknown))
	{
		*ppvObj = this;
		AddRef();
		return S_OK;
	}
	else if ((riid == IID_IMarshal) && (m_punkFTMarshaller != NULL))
	{
		return m_punkFTMarshaller->QueryInterface(riid, ppvObj);
	}

	*ppvObj = NULL;
	return E_NOINTERFACE;
}

IStylusSyncPlugin* CSyncEventHandlerRTS::Create(IRealTimeStylus* pRealTimeStylus)
{
	// Check input argument
	if (pRealTimeStylus == NULL)
	{
		//ASSERT(pRealTimeStylus != NULL && L"CSyncEventHandlerRTS::Create: invalid argument RealTimeStylus");
		return NULL;
	}

	// Instantiate CSyncEventHandlerRTS object
	CSyncEventHandlerRTS* pSyncEventHandlerRTS = new CSyncEventHandlerRTS();
	if (pSyncEventHandlerRTS == NULL)
	{
		//ASSERT(pSyncEventHandlerRTS != NULL && L"CSyncEventHandlerRTS::Create: cannot create instance of CSyncEventHandlerRTS");
		return NULL;
	}

	// Create free-threaded marshaller for this object and aggregate it.
	HRESULT hr = CoCreateFreeThreadedMarshaler(pSyncEventHandlerRTS, &pSyncEventHandlerRTS->m_punkFTMarshaller);
	if (FAILED(hr))
	{
		//ASSERT(SUCCEEDED(hr) && L"CSyncEventHandlerRTS::Create: cannot create free-threaded marshaller");
		pSyncEventHandlerRTS->Release();
		return NULL;
	}

	// Add CSyncEventHandlerRTS object to the list of synchronous plugins in the RTS object.
	hr = pRealTimeStylus->AddStylusSyncPlugin(
		0,                      // insert plugin at position 0 in the sync plugin list
		pSyncEventHandlerRTS);  // plugin to be inserted - event handler CSyncEventHandlerRTS
	if (FAILED(hr))
	{
		//ASSERT(SUCCEEDED(hr) && L"CEventHandlerRTS::Create: failed to add CSyncEventHandlerRTS to the RealTimeStylus plugins");
		pSyncEventHandlerRTS->Release();
		return NULL;
	}

	return pSyncEventHandlerRTS;
}
void RTSSpeed()
{
	int x, y;
	int lastx = -1, lasty = -1;

	while (!off_signal)
	{
		for (int i = 0; i < touchNum; i++)
		{
			std::shared_lock<std::shared_mutex> lock1(PointPosSm);
			x = TouchPos[TouchList[i]].pt.x;
			y = TouchPos[TouchList[i]].pt.y;
			lock1.unlock();

			if (lastx == -1 && lasty == -1) lastx = x, lasty = y;

			double speed = (TouchSpeed[TouchList[i]] + sqrt(pow(x - lastx, 2) + pow(y - lasty, 2))) / 2;
			std::unique_lock<std::shared_mutex> lock2(TouchSpeedSm);
			TouchSpeed[TouchList[i]] = speed;
			lock2.unlock();

			lastx = x, lasty = y;
		}

		hiex::DelayFPS(20);
	}
}

///图像 =====

//drawpad画笔
IMAGE drawpad(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)); //主画板
IMAGE alpha_drawpad(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)); //临时画板
IMAGE last_drawpad; //上一次绘制内容，后续将弃用
IMAGE putout; //主画板上叠加的控件内容
IMAGE tester(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)); //图形绘制画板
IMAGE pptdrawpad(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)); //PPT控件画板

IMAGE test_sign[5];

struct RecallStruct
{
	IMAGE img;
	map<pair<int, int>, bool> extreme_point;
};
deque<RecallStruct> RecallImage;//撤回栈

//悬浮窗
IMAGE background(576, 386);
Graphics graphics(GetImageHDC(&background));

///文字 =====

PrivateFontCollection fontCollection;
FontFamily HarmonyOS_fontFamily;
StringFormat stringFormat;
StringFormat stringFormat_left;
RECT words_rect, dwords_rect, pptwords_rect;

//string to wstring
wstring string_to_wstring(const string& s)
{
	if (s.empty()) return L"";

	int sizeRequired = MultiByteToWideChar(936, 0, s.c_str(), -1, NULL, 0);
	if (sizeRequired == 0) return L"";

	wstring ws(sizeRequired - 1, L'\0');
	MultiByteToWideChar(936, 0, s.c_str(), -1, &ws[0], sizeRequired);

	return ws;
}
//wstring to string
string wstring_to_string(const wstring& ws)
{
	if (ws.empty()) return "";

	int sizeRequired = WideCharToMultiByte(936, 0, ws.c_str(), -1, NULL, 0, NULL, NULL);
	if (sizeRequired == 0) return "";

	string s(sizeRequired - 1, '\0');
	WideCharToMultiByte(936, 0, ws.c_str(), -1, &s[0], sizeRequired, NULL, NULL);

	return s;
}

//string 转 wstring
wstring convert_to_wstring(const string s)
{
	LPCSTR pszSrc = s.c_str();
	int nLen = s.size();

	int nSize = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszSrc, nLen, 0, 0);
	WCHAR* pwszDst = new WCHAR[nSize + 1];
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)pszSrc, nLen, pwszDst, nSize);
	pwszDst[nSize] = 0;
	if (pwszDst[0] == 0xFEFF) // skip Oxfeff
		for (int i = 0; i < nSize; i++)
			pwszDst[i] = pwszDst[i + 1];
	wstring wcharString(pwszDst);
	delete pwszDst;
	return wcharString;
}
//wstring 转 string
string convert_to_string(const wstring str)
{
	int size = WideCharToMultiByte(CP_ACP, 0, str.c_str(), str.size(), nullptr, 0, nullptr, nullptr);
	auto p_str(std::make_unique<char[]>(size + 1));
	WideCharToMultiByte(CP_ACP, 0, str.c_str(), str.size(), p_str.get(), size, nullptr, nullptr);
	return std::string(p_str.get());
}
//c# string 转 wstring
wstring bstr_to_wstring(const _bstr_t& bstr)
{
	return static_cast<wchar_t*>(bstr);
}
//string 转 LPCWSTR
LPCWSTR stringtoLPCWSTR(string str)
{
	size_t size = str.length();
	int wLen = ::MultiByteToWideChar(CP_UTF8,
		0,
		str.c_str(),
		-1,
		NULL,
		0);
	wchar_t* buffer = new wchar_t[wLen + 1];
	memset(buffer, 0, (wLen + 1) * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), size, (LPWSTR)buffer, wLen);
	return buffer;
}
//string 转 urlencode
string convert_to_urlencode(string str)
{
	int size(str.size() * 3 + 1);
	auto pstr(std::make_unique<char[]>(size + 1));
	int i(0);

	for (const auto& x : str)
	{
		if (x >= 0 && x <= 0x80)
		{
			if (x >= 0x41 && x <= 0x5A || x >= 0x61 && x <= 0x7A)
			{
				pstr.get()[i] = x;
				++i;
				continue;
			}
		}
		sprintf(pstr.get() + i, "%%%02X", static_cast<unsigned char>(x));
		i += 3;
	}
	str = pstr.get();
	return str;
}
//utf-8 转 GBK
string convert_to_gbk(string strUTF8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, NULL, 0);
	TCHAR* wszGBK = new TCHAR[len + 1];
	memset(wszGBK, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, strUTF8.c_str(), -1, wszGBK, len);

	len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
	char* szGBK = new char[len + 1];
	memset(szGBK, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
	std::string strTemp(szGBK);
	delete[]szGBK;
	delete[]wszGBK;
	return strTemp;
}
//GBK 转 utf-8
string convert_to_utf8(string str)
{
	wstring x = string_to_wstring(str);

	int size = WideCharToMultiByte(CP_UTF8, 0, x.c_str(), x.size(), nullptr, 0, nullptr, nullptr);
	auto p_str(std::make_unique<char[]>(size + 1));
	WideCharToMultiByte(CP_UTF8, 0, x.c_str(), x.size(), p_str.get(), size, nullptr, nullptr);
	str = p_str.get();
	return str;
}

///窗口 =====

HWND floating_window = NULL; //悬浮窗窗口
HWND drawpad_window = NULL; //画板窗口
HWND ppt_window = NULL; //PPT控件窗口
HWND test_window = NULL; //程序调测窗口
HWND freeze_window = NULL; //定格背景窗口

bool FreezePPT;
HWND ppt_show;

//窗口强制置顶
BOOL OnForceShow(HWND hWnd)
{
	HWND hForeWnd = NULL;
	DWORD dwForeID = 0;
	DWORD dwCurID = 0;

	hForeWnd = ::GetForegroundWindow();
	dwCurID = ::GetCurrentThreadId();
	dwForeID = ::GetWindowThreadProcessId(hForeWnd, NULL);
	::AttachThreadInput(dwCurID, dwForeID, TRUE);
	//::ShowWindow(hWnd, SW_SHOWNORMAL);
	::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	::SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	::SetForegroundWindow(hWnd);
	// 将前台窗口线程贴附到当前线程（也就是程序A中的调用线程）
	::AttachThreadInput(dwCurID, dwForeID, FALSE);

	return TRUE;
}
//窗口是否置顶
bool IsWindowFocused(HWND hWnd)
{
	return GetForegroundWindow() == hWnd;
}
//程序进程状态获取
bool isProcessRunning(const std::wstring& processPath)
{
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

	if (Process32First(snapshot, &entry)) {
		do {
			// 打开进程句柄
			HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
			if (process == NULL) {
				continue;
			}

			// 获取进程完整路径
			wchar_t path[MAX_PATH];
			DWORD size = MAX_PATH;
			if (QueryFullProcessImageName(process, 0, path, &size)) {
				if (processPath == path) {
					CloseHandle(process);
					CloseHandle(snapshot);
					return true;
				}
			}

			CloseHandle(process);
		} while (Process32Next(snapshot, &entry));
	}

	CloseHandle(snapshot);
	return false;
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

//Magnification API
IMAGE MagnificationBackground = CreateImageColor(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), RGBA(255, 255, 255, 255), false);
HWND hwndHost, hwndMag;
int magnificationWindowReady;

shared_mutex MagnificationBackgroundSm;
RECT magWindowRect;
RECT hostWindowRect;

BOOL MagImageScaling(HWND hwnd, void* srcdata, MAGIMAGEHEADER srcheader, void* destdata, MAGIMAGEHEADER destheader, RECT unclipped, RECT clipped, HRGN dirty)
{
	BITMAPINFOHEADER bmif;
	HBITMAP hBmp = NULL;
	BYTE* pBits = nullptr;

	bmif.biSize = sizeof(BITMAPINFOHEADER);
	bmif.biHeight = srcheader.height;
	bmif.biWidth = srcheader.width;
	bmif.biSizeImage = bmif.biWidth * bmif.biHeight * 4;
	bmif.biPlanes = 1;
	bmif.biBitCount = (WORD)(bmif.biSizeImage / bmif.biHeight / bmif.biWidth * 8);
	bmif.biCompression = BI_RGB;

	LPBYTE pData = (BYTE*)new BYTE[bmif.biSizeImage];
	memcpy(pData, (LPBYTE)srcdata + srcheader.offset, bmif.biSizeImage);
	LONG nLineSize = bmif.biWidth * bmif.biBitCount / 8;
	BYTE* pLineData = new BYTE[nLineSize];
	LONG nLineStartIndex = 0;
	LONG nLineEndIndex = bmif.biHeight - 1;
	while (nLineStartIndex < nLineEndIndex)
	{
		BYTE* pStart = pData + (nLineStartIndex * nLineSize);
		BYTE* pEnd = pData + (nLineEndIndex * nLineSize);
		memcpy(pLineData, pStart, nLineSize);
		memcpy(pStart, pEnd, nLineSize);
		memcpy(pEnd, pLineData, nLineSize);
		nLineStartIndex++;
		nLineEndIndex--;
	}

	// 使用CreateDIBSection来创建HBITMAP
	HDC hDC = GetDC(NULL);
	hBmp = CreateDIBSection(hDC, (BITMAPINFO*)&bmif, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
	ReleaseDC(NULL, hDC);

	if (hBmp && pBits)
	{
		memcpy(pBits, pData, bmif.biSizeImage);
		MagnificationBackground = Bitmap2Image(&hBmp, false);
	}

	delete[] pLineData;
	delete[] pData;
	DeleteObject(hBmp);

	return 1;
}
void UpdateMagWindow()
{
	POINT mousePoint;
	GetCursorPos(&mousePoint);

	RECT sourceRect = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };

	// Set the source rectangle for the magnifier control.
	std::unique_lock<std::shared_mutex> lock1(MagnificationBackgroundSm);
	MagSetWindowSource(hwndMag, sourceRect);
	lock1.unlock();

	// Reclaim topmost status, to prevent unmagnified menus from remaining in view.
	SetWindowPos(hwndHost, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);

	// Force redraw.
	InvalidateRect(hwndMag, NULL, TRUE);
}

#define RESTOREDWINDOWSTYLES WS_SIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN | WS_CAPTION | WS_MAXIMIZEBOX
HINSTANCE hInst;

LRESULT CALLBACK MagnifierWindowWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return DefWindowProc(hWnd, message, wParam, lParam);
}
ATOM RegisterHostWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = {};

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = MagnifierWindowWndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(1 + COLOR_BTNFACE);
	wcex.lpszClassName = TEXT("MagnifierWindow");

	return RegisterClassEx(&wcex);
}
BOOL SetupMagnifier(HINSTANCE hinst)
{
	hostWindowRect.top = 0;
	hostWindowRect.bottom = GetSystemMetrics(SM_CYSCREEN);
	hostWindowRect.left = 0;
	hostWindowRect.right = GetSystemMetrics(SM_CXSCREEN);

	RegisterHostWindowClass(hinst);
	hwndHost = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED, TEXT("MagnifierWindow"), TEXT("Screen Magnifier Sample"), RESTOREDWINDOWSTYLES, 0, 0, hostWindowRect.right, hostWindowRect.bottom, NULL, NULL, hInst, NULL);
	if (!hwndHost) return FALSE;

	SetLayeredWindowAttributes(hwndHost, 0, 255, LWA_ALPHA);

	GetClientRect(hwndHost, &magWindowRect);
	hwndMag = CreateWindow(WC_MAGNIFIER, TEXT("MagnifierWindow"), WS_CHILD | MS_SHOWMAGNIFIEDCURSOR | WS_VISIBLE, magWindowRect.left, magWindowRect.top, magWindowRect.right, magWindowRect.bottom, hwndHost, NULL, hInst, NULL);
	if (!hwndMag) return FALSE;

	SetWindowLong(hwndMag, GWL_STYLE, GetWindowLong(hwndMag, GWL_STYLE) & ~WS_CAPTION);//隐藏标题栏
	SetWindowPos(hwndMag, NULL, hostWindowRect.left, hostWindowRect.top, hostWindowRect.right - hostWindowRect.left, hostWindowRect.bottom - hostWindowRect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_DRAWFRAME);
	SetWindowLong(hwndMag, GWL_EXSTYLE, WS_EX_TOOLWINDOW);//隐藏任务栏

	// Set the magnification factor.
	MAGTRANSFORM matrix;
	memset(&matrix, 0, sizeof(matrix));
	matrix.v[0][0] = 1.0f;
	matrix.v[1][1] = 1.0f;
	matrix.v[2][2] = 1.0f;

	BOOL ret = MagSetWindowTransform(hwndMag, &matrix);
	if (ret)
	{
		MAGCOLOREFFECT magEffectInvert =
		{ { // MagEffectInvert
			{  1.0f,  0.0f,  0.0f,  0.0f,  0.0f },
			{  0.0f,  1.0f,  0.0f,  0.0f,  0.0f },
			{  0.0f,  0.0f,  1.0f,  0.0f,  0.0f },
			{  0.0f,  0.0f,  0.0f,  1.0f,  0.0f },
			{  1.0f,  1.0f,  1.0f,  0.0f,  1.0f }
		} };

		ret = MagSetColorEffect(hwndMag, NULL);
	}

	MagSetImageScalingCallback(hwndMag, (MagImageScalingCallback)MagImageScaling);

	return ret;
}

void MagnifierThread()
{
	MagInitialize();

	SetupMagnifier(GetModuleHandle(0));
	ShowWindow(hwndHost, SW_HIDE);
	UpdateWindow(hwndHost);

	while (!off_signal)
	{
		UpdateMagWindow();
		hiex::DelayFPS(25);

		if (magnificationWindowReady >= 4)
		{
			std::vector<HWND> hwndList;
			hwndList.emplace_back(floating_window);
			hwndList.emplace_back(drawpad_window);
			hwndList.emplace_back(ppt_window);
			hwndList.emplace_back(test_window);
			MagSetWindowFilterList(hwndMag, MW_FILTERMODE_EXCLUDE, hwndList.size(), hwndList.data());

			magnificationWindowReady = -1;
		}
	}

	MagUninitialize();
}

///绘制 =====

struct
{
	int x, y;
	int last_x = -1, last_y = -1;
}BrushColorChoose;
IMAGE ColorPaletteImg;
shared_mutex ColorPaletteSm;

// 将标准的 sRGB 值转换为线性 RGB 值
double sRGBToLinear(double s) {
	if (s <= 0.04045) return s / 12.92;
	return pow((s + 0.055) / 1.055, 2.4);
}
// 计算相对亮度
double computeLuminance(COLORREF color) {
	double R = sRGBToLinear(GetRValue(color) / 255.0);
	double G = sRGBToLinear(GetGValue(color) / 255.0);
	double B = sRGBToLinear(GetBValue(color) / 255.0);
	return 0.2126 * R + 0.7152 * G + 0.0722 * B;
}
// 计算两种颜色的对比度
double computeContrast(COLORREF color1, COLORREF color2) {
	double L1 = computeLuminance(color1);
	double L2 = computeLuminance(color2);

	if (L1 > L2) return (L1 + 0.05) / (L2 + 0.05);
	return (L2 + 0.05) / (L1 + 0.05);
}

//像素颜色调整（将所有透明度不为0的像素点，改为指定颜色）
void ChangeColor(IMAGE& img, COLORREF color)
{
	// 获取图像的宽度和高度
	int width = img.getwidth();
	int height = img.getheight();

	// 获取图像的缓冲区指针
	DWORD* pBuf = GetImageBuffer(&img);

	// 遍历每个像素点
	for (int i = 0; i < width * height; i++)
	{
		// 获取当前像素点的颜色值
		DWORD pixel = pBuf[i];

		// 获取当前像素点的透明度（alpha 值）
		DWORD alpha = pixel >> 24;

		// 如果源图像颜色与修改颜色相同则不修改（直接跳过）
		if (alpha != 0)
		{
			if (pixel == BGR(color)) return;
		}
		else continue;

		// 将COLORREF转换为RGB
		DWORD rgb = ((color & 0xFF) << 16) | (color & 0xFF00) | ((color & 0xFF0000) >> 16);

		// 根据透明度调整颜色的亮度
		DWORD r = (rgb & 0xFF0000) >> 16;
		DWORD g = (rgb & 0x00FF00) >> 8;
		DWORD b = (rgb & 0x0000FF);
		r = r * alpha / 255;
		g = g * alpha / 255;
		b = b * alpha / 255;
		rgb = (r << 16) | (g << 8) | b;

		// 将传入的颜色值与透明度合并
		DWORD newPixel = (alpha << 24) | (rgb & 0x00FFFFFF);

		// 将新的颜色值写入缓冲区
		pBuf[i] = newPixel;
	}
}
// 计算两个COLORREF颜色之间的加权距离
double color_distance(COLORREF c1, COLORREF c2) {
	// 提取各个颜色分量
	int r1 = GetRValue(c1);
	int g1 = GetGValue(c1);
	int b1 = GetBValue(c1);
	int r2 = GetRValue(c2);
	int g2 = GetGValue(c2);
	int b2 = GetBValue(c2);

	// 设置各个分量的权重
	double wr = 0.3;
	double wg = 0.59;
	double wb = 0.11;

	// 计算加权平方和
	double sum = wr * (r1 - r2) * (r1 - r2) +
		wg * (g1 - g2) * (g1 - g2) +
		wb * (b1 - b2) * (b1 - b2);

	// 开平方并返回
	return sqrt(sum);
}
// 定义反色函数
COLORREF InvertColor(COLORREF color, bool alpha_enable = false)
{
	// 提取颜色分量
	BYTE red = GetRValue(color);
	BYTE green = GetGValue(color);
	BYTE blue = GetBValue(color);
	BYTE alpha;
	if (alpha_enable) alpha = (color >> 24) & 0xff;
	else alpha = 255;

	// 反色分量
	red = 255 - red;
	green = 255 - green;
	blue = 255 - blue;

	// 合并颜色分量和透明度
	COLORREF inverted = red | (green << 8) | (blue << 16) | (alpha << 24);

	// 返回反色
	return inverted;
}
//保存图像到本地
void saveImageToPNG(IMAGE img, const char* filename, bool alpha = true, int compression_level = 9)
{
	int width = img.getwidth();
	int height = img.getheight();

	// Get the image buffer of the IMAGE object
	DWORD* pMem = GetImageBuffer(&img);

	if (alpha)
	{
		unsigned char* data = new unsigned char[width * height * 4];
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				COLORREF color = pMem[y * width + x];
				data[(y * width + x) * 4 + 0] = GetBValue(color); // Swap the red and blue components
				data[(y * width + x) * 4 + 1] = GetGValue(color);
				data[(y * width + x) * 4 + 2] = GetRValue(color); // Swap the red and blue components
				data[(y * width + x) * 4 + 3] = color >> 24;
			}
		}

		stbi_write_png_compression_level = compression_level;
		stbi_write_png(filename, width, height, 4, data, width * 4);
		delete[] data;
	}
	else
	{
		unsigned char* data = new unsigned char[width * height * 3];
		for (int y = 0; y < height; ++y) {
			for (int x = 0; x < width; ++x) {
				COLORREF color = pMem[y * width + x];
				data[(y * width + x) * 3 + 0] = GetBValue(color); // Swap the red and blue components
				data[(y * width + x) * 3 + 1] = GetGValue(color);
				data[(y * width + x) * 3 + 2] = GetRValue(color); // Swap the red and blue components
			}
		}

		stbi_write_png_compression_level = compression_level;
		stbi_write_png(filename, width, height, 3, data, width * 3);
		delete[] data;
	}
}
void SaveHBITMAPToPNG(HBITMAP hBitmap, const char* filename)
{
	BITMAP bmp;
	GetObject(hBitmap, sizeof(bmp), &bmp);

	int width = bmp.bmWidth;
	int height = bmp.bmHeight;

	HDC hdc = GetDC(NULL);
	HDC memDC = CreateCompatibleDC(hdc);
	HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, hBitmap);

	unsigned char* pixels = new unsigned char[width * height * 4];
	BITMAPINFOHEADER bih;
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = width;
	bih.biHeight = -height;
	bih.biPlanes = 1;
	bih.biBitCount = 32;
	bih.biCompression = BI_RGB;
	bih.biSizeImage = 0;
	bih.biXPelsPerMeter = 0;
	bih.biYPelsPerMeter = 0;
	bih.biClrUsed = 0;
	bih.biClrImportant = 0;

	GetDIBits(memDC, hBitmap, 0, height, pixels, (BITMAPINFO*)&bih, DIB_RGB_COLORS);

	stbi_flip_vertically_on_write(1);
	stbi_write_png(filename, width, height, 4, pixels, width * 4);

	delete[] pixels;

	SelectObject(memDC, oldBmp);
	DeleteDC(memDC);
}
void saveImageToJPG(IMAGE img, const char* filename, int quality = 100)
{
	int width = img.getwidth();
	int height = img.getheight();
	DWORD* pMem = GetImageBuffer(&img);

	unsigned char* data = new unsigned char[width * height * 3];

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			COLORREF color = pMem[y * width + x];
			data[(y * width + x) * 3 + 0] = GetBValue(color);
			data[(y * width + x) * 3 + 1] = GetGValue(color);
			data[(y * width + x) * 3 + 2] = GetRValue(color);
		}
	}
	stbi_write_jpg(filename, width, height, 3, data, quality);
	delete[] data;
}
//比较图像
bool CompareImagesWithBuffer(IMAGE* img1, IMAGE* img2)
{
	// 检查宽度和高度
	if (img1->getwidth() != img2->getwidth() || img1->getheight() != img2->getheight()) return false;

	DWORD* pBuf1 = GetImageBuffer(img1);
	DWORD* pBuf2 = GetImageBuffer(img2);

	int dataSize = img1->getwidth() * img1->getheight() * sizeof(DWORD);

	return memcmp(pBuf1, pBuf2, dataSize) == 0;
}
//设置图像必须不拥有全透明像素（将所有全透明像素点透明度设置为1）
void SetAlphaToOne(IMAGE* pImg) // pImg是绘图设备指针
{
	// 获取图像缓冲区指针
	DWORD* pBuffer = GetImageBuffer(pImg);
	// 获取图像宽度和高度
	int width = pImg->getwidth();
	int height = pImg->getheight();
	// 遍历每个点
	for (int i = 0; i < width * height; i++)
	{
		// 获取当前点的颜色值（ARGB格式）
		DWORD color = pBuffer[i];
		// 如果透明度为0，则将其设为1
		if ((color >> 24) == 0)
		{
			// 将最高8位设为1，并保持其他位不变
			color = (color & 0x00FFFFFF) | 0x01000000;
			// 将修改后的颜色值写回到图像缓冲区
			pBuffer[i] = color;
		}
	}
}

struct
{
	bool select;
}penetrate; //窗口穿透
struct
{
	bool select;
	bool mode;
}choose; //选择
struct
{
	bool select;
	int width = 4, mode = 1;
	COLORREF color, primary_colour;

	int PenWidthHistory = 4, HighlighterWidthHistory = 40;
}brush; //画笔
struct
{
	bool select;
	int mode;
}rubber; //橡皮
struct
{
	bool select;
}test; //调测

struct
{
	int select;
}plug_in_RandomRollCall;
struct
{
	bool select;
	int mode;

	bool update;
}FreezeFrame;

///时间（主要用于联动部分） =====

SYSTEMTIME sys_time;
//时间戳
wstring getTimestamp()
{
	auto now = std::chrono::system_clock::now();
	auto duration = now.time_since_epoch();
	auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	std::wstringstream wss;
	wss << millis;
	return wss.str();
}
wstring getCurrentDate()
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::wstringstream wss;
	wss << std::put_time(&tm, L"%Y%m%d");
	return wss.str();
}
//获取日期
wstring CurrentDate()
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::wostringstream woss;
	woss << std::put_time(&tm, L"%Y-%m-%d");
	return woss.str();
}
//获取时间
wstring CurrentTime()
{
	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	std::wostringstream woss;
	woss << std::put_time(&tm, L"%H-%M-%S");
	return woss.str();
}

void GetTime()
{
	thread_status[L"GetTime"] = true;
	while (!off_signal)
	{
		GetLocalTime(&sys_time);
		Sleep(1000);
	}
	thread_status[L"GetTime"] = false;
}
string GetCurrentTimeAll()
{
	auto now = std::chrono::system_clock::now(); // 获取当前时间点
	auto in_time_t = std::chrono::system_clock::to_time_t(now); // 转换为time_t

	std::tm buf; // 定义时间结构体
	localtime_s(&buf, &in_time_t); // 转换为本地时间

	std::stringstream ss;
	ss << std::put_time(&buf, "%Y/%m/%d %H:%M:%S"); // 格式化时间
	return ss.str();
}

///联动部分 =====

//PPT联动

IMAGE ppt_icon[5];

struct
{
	int currentSlides = -1;
	int totalSlides = -1;
}ppt_info;
struct
{
	bool is_save = false;
	map<int, bool> is_saved;
	map<int, IMAGE> image;
}ppt_img;
wstring LinkTest()
{
	wstring ret = L"COM库(.dll) 不存在，且发生严重错误，返回值被忽略";

	if (_waccess((string_to_wstring(global_path) + L"PptCOM.dll").c_str(), 4) == 0)
	{
		IPptCOMServerPtr pto;
		try
		{
			_com_util::CheckError(pto.CreateInstance(_uuidof(PptCOMServer)));
			ret = bstr_to_wstring(pto->LinkTest());
		}
		catch (_com_error& err)
		{
			ret = L"COM库(.dll) 存在，COM成功初始化，但C++端COM接口异常：" + wstring(err.ErrorMessage());
		}
	}
	else
	{
		wchar_t absolutePath[_MAX_PATH];

		if (_wfullpath(absolutePath, L"PptCOM.dll", _MAX_PATH) != NULL)
		{
			ret = L"COM库(.dll) 不存在，预期调用目录为：\"" + string_to_wstring(global_path) + L"PptCOM.dll\"";
		}
		else ret = L"COM库(.dll) 不存在，预期调用目录测算失败";
	}

	return ret;
}
wstring IsPptDependencyLoaded()
{
	wstring ret = L"PPT 联动组件异常，且发生严重错误，返回值被忽略";

	IPptCOMServerPtr pto;
	try
	{
		_com_util::CheckError(pto.CreateInstance(_uuidof(PptCOMServer)));
		ret = L"COM接口正常，C#类库反馈信息：" + bstr_to_wstring(pto->IsPptDependencyLoaded());
	}
	catch (_com_error& err)
	{
		ret = L"COM接口异常：" + wstring(err.ErrorMessage());
	}

	return ret;
}
struct
{
	int CurrentPage = -1, TotalPage = -1;
}ppt_info_stay;
HWND GetPptShow()
{
	HWND hWnd = NULL;

	IPptCOMServerPtr pto;
	try
	{
		_com_util::CheckError(pto.CreateInstance(_uuidof(PptCOMServer)));

		_variant_t result = pto->GetPptHwnd();
		hWnd = (HWND)result.llVal;

		return hWnd;
	}
	catch (_com_error)
	{
	}

	return NULL;
}
bool EndPptShow()
{
	IPptCOMServerPtr pto;
	try
	{
		_com_util::CheckError(pto.CreateInstance(_uuidof(PptCOMServer)));
		pto->EndSlideShow();

		return true;
	}
	catch (_com_error)
	{
	}

	return false;
}
void DrawControlWindow();
void ControlManipulation();

//获取当前页编号
int GetCurrentPage()
{
	int currentSlides = -1;

	IPptCOMServerPtr pto;
	try
	{
		_com_util::CheckError(pto.CreateInstance(_uuidof(PptCOMServer)));
		currentSlides = pto->currentSlideIndex();
	}
	catch (_com_error)
	{
	}

	return currentSlides;
}
//获取总页数
int GetTotalPage()
{
	int totalSlides = -1;

	IPptCOMServerPtr pto;
	try
	{
		_com_util::CheckError(pto.CreateInstance(_uuidof(PptCOMServer)));
		totalSlides = pto->totalSlideIndex();
	}
	catch (_com_error)
	{
	}

	return totalSlides;
}
void ppt_state()
{
	thread_status[L"ppt_state"] = true;
	while (!off_signal)
	{
		ppt_info_stay.CurrentPage = GetCurrentPage();
		ppt_info_stay.TotalPage = GetTotalPage();
		if (ppt_info_stay.TotalPage == -1 && !off_signal) Sleep(3000);
		else if (!off_signal) Sleep(10);
	}
	thread_status[L"ppt_state"] = false;
}

//希沃视频展台是否开启
bool SeewoCamera;

//弹窗拦截
//关闭AIClass和希沃白板5窗口
HWND FindWindowByStrings(const std::wstring& className, const std::wstring& windowTitle, const std::wstring& style, int width = 0, int height = 0)
{
	HWND hwnd = NULL;
	while ((hwnd = FindWindowEx(NULL, hwnd, NULL, NULL)) != NULL)
	{
		TCHAR classNameBuffer[1024];
		GetClassName(hwnd, classNameBuffer, 1024);
		if (_tcsstr(classNameBuffer, className.c_str()) == NULL) continue;

		TCHAR title[1024];
		GetWindowText(hwnd, title, 1024);
		if (_tcsstr(title, windowTitle.c_str()) == NULL) continue;

		if (windowTitle.length() == 0)
		{
			if (wstring(title) == windowTitle && to_wstring(GetWindowLong(hwnd, GWL_STYLE)) == style)
			{
				if (width && height)
				{
					RECT rect{};
					DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof(RECT));
					int twidth = rect.right - rect.left, thwight = rect.bottom - rect.top;

					HDC hdc = GetDC(NULL);
					int horizontalDPI = GetDeviceCaps(hdc, LOGPIXELSX);
					int verticalDPI = GetDeviceCaps(hdc, LOGPIXELSY);
					ReleaseDC(NULL, hdc);
					float scale = (horizontalDPI + verticalDPI) / 2.0f / 96.0f;

					if (abs(width * scale - twidth) <= 1 && abs(height * scale - thwight) <= 1) return hwnd;
				}
				else return hwnd;
			}
		}
		else if (to_wstring(GetWindowLong(hwnd, GWL_STYLE)) == style) return hwnd;
	}
	return NULL;
}
void black_block()
{
	thread_status[L"black_block"] = true;
	while (!off_signal)
	{
		HWND ai_class = FindWindowByStrings(L"UIIrregularWindow", L"UIIrregularWindow", L"-1811939328");
		if (ai_class != NULL) PostMessage(ai_class, WM_CLOSE, 0, 0);

		HWND Seewo_Whiteboard = FindWindowByStrings(L"HwndWrapper[EasiNote;;", L"", L"369623040", 550, 200);
		if (Seewo_Whiteboard != NULL) PostMessage(Seewo_Whiteboard, WM_CLOSE, 0, 0);

		HWND Seewo_Camera = FindWindowByStrings(L"HwndWrapper[EasiCamera.exe;;", L"希沃视频展台", L"386400256");
		if (Seewo_Camera != NULL) SeewoCamera = true;
		else SeewoCamera = false;

		for (int i = 1; i <= 5 && !off_signal; i++) Sleep(1000);
	}
	thread_status[L"black_block"] = false;
}

//智能绘图部分
map<pair<int, int>, bool> extreme_point;
//map<pair<Point, Point >, bool> extreme_line;
double pointToLineDistance(Point lineStart, Point lineEnd, Point p) {
	if (lineStart.X == lineEnd.X) {
		return abs(p.X - lineStart.X);
	}

	double a = double(lineEnd.Y - lineStart.Y) / (lineEnd.X - lineStart.X);
	double b = lineStart.Y - a * lineStart.X;

	return abs(a * p.X - p.Y + b) / sqrt(a * a + 1);
}
double pointToLineSegmentDistance(Point lineStart, Point lineEnd, Point p)
{
	double x1 = lineStart.X;
	double y1 = lineStart.Y;
	double x2 = lineEnd.X;
	double y2 = lineEnd.Y;
	double x3 = p.X;
	double y3 = p.Y;

	if (x1 == x2) {
		if (y3 >= min(y1, y2) && y3 <= max(y1, y2)) {
			return abs(x3 - x1);
		}
		else {
			return min(sqrt(pow(x3 - x1, 2) + pow(y3 - y1, 2)), sqrt(pow(x3 - x2, 2) + pow(y3 - y2, 2)));
		}
	}

	double a = (y2 - y1) / (x2 - x1);
	double b = y1 - a * x1;
	double x4 = (a * (y3 - b) + x3) / (a * a + 1);
	if (x4 >= min(x1, x2) && x4 <= max(x1, x2)) {
		return abs(a * x3 - y3 + b) / sqrt(a * a + 1);
	}
	else {
		return min(sqrt(pow(x3 - x1, 2) + pow(y3 - y1, 2)), sqrt(pow(x3 - x2, 2) + pow(y3 - y2, 2)));
	}
}
bool isLine(vector<Point> points, int tolerance)
{
	int n = points.size();
	if (n < 2) return false;

	double last_distance = 0;
	int trend = 0; //1远离 2靠近
	int fluctuate = 0;

	for (int i = 1; i < n - 1; i++)
	{
		double distance = pointToLineSegmentDistance(points[0], points[n - 1], points[i]);
		if (distance > tolerance) return false;

		if (distance > last_distance)
		{
			if (trend != 1 && abs(distance - last_distance) >= double(tolerance) / 6.0)
			{
				fluctuate++;
				trend = 1;
				last_distance = distance;
			}
		}
		else if (distance < last_distance)
		{
			if (trend != 2 && abs(distance - last_distance) >= double(tolerance) / 6.0)
			{
				fluctuate++;
				trend = 2;
				last_distance = distance;
			}
		}
	}

	if (fluctuate <= 5) return true;
	else return false;
}

/// 杂项 =====

wstring GetCurrentExeDirectory()
{
	wchar_t buffer[MAX_PATH];
	DWORD length = GetModuleFileNameW(NULL, buffer, sizeof(buffer) / sizeof(wchar_t));
	if (length == 0 || length == sizeof(buffer) / sizeof(wchar_t)) return L"";

	filesystem::path fullPath(buffer);
	return fullPath.parent_path().wstring();
}
//开机启动项设置
bool ModifyRegedit(bool bAutoRun)
{
	wchar_t pFileName[MAX_PATH] = { 0 };
	wcscpy(pFileName, string_to_wstring(_pgmptr).c_str());

	HKEY hKey;
	LPCTSTR lpRun = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
	long lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRun, 0, KEY_WRITE, &hKey);
	if (lRet != ERROR_SUCCESS)
		return false;

	if (bAutoRun)
		RegSetValueEx(hKey, L"xmg_drawpad_startup", 0, REG_SZ, (const BYTE*)(LPCSTR)pFileName, MAX_PATH);
	else
		RegDeleteValueA(hKey, "xmg_drawpad_startup");
	RegCloseKey(hKey);
	return true;
}
//网络状态获取
static bool checkIsNetwork()
{
	CoInitialize(NULL);
	//  通过NLA接口获取网络状态
	IUnknown* pUnknown = NULL;
	BOOL   bOnline = TRUE;//是否在线
	HRESULT Result = CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL,
		IID_IUnknown, (void**)&pUnknown);
	if (SUCCEEDED(Result))
	{
		INetworkListManager* pNetworkListManager = NULL;
		if (pUnknown)
			Result = pUnknown->QueryInterface(IID_INetworkListManager, (void
				**)&pNetworkListManager);
		if (SUCCEEDED(Result))
		{
			VARIANT_BOOL IsConnect = VARIANT_FALSE;
			if (pNetworkListManager)
				Result = pNetworkListManager->get_IsConnectedToInternet(&IsConnect);
			if (SUCCEEDED(Result))
			{
				bOnline = (IsConnect == VARIANT_TRUE) ? true : false;
			}
		}
		if (pNetworkListManager)
			pNetworkListManager->Release();
	}
	if (pUnknown)
		pUnknown->Release();
	CoUninitialize();
	return bOnline;
}
// 提取指定模块中的资源文件
bool ExtractResource(LPCTSTR strDstFile, LPCTSTR strResType, LPCTSTR strResName)
{
	// 创建文件
	HANDLE hFile = ::CreateFile(strDstFile, GENERIC_WRITE, NULL, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	// 查找资源文件、加载资源到内存、得到资源大小
	HRSRC	hRes = ::FindResource(NULL, strResName, strResType);
	HGLOBAL	hMem = ::LoadResource(NULL, hRes);
	DWORD	dwSize = ::SizeofResource(NULL, hRes);

	// 写入文件
	DWORD dwWrite = 0;		// 返回写入字节
	::WriteFile(hFile, hMem, dwSize, &dwWrite, NULL);
	::CloseHandle(hFile);

	return true;
}

wstring GetCPUID()
{
	wstring strCPUID;
	int CPUInfo[4] = { -1 };
	unsigned long s1, s2;
	wchar_t buffer[17];

	__cpuid(CPUInfo, 1);
	s1 = CPUInfo[3];
	s2 = CPUInfo[0];

	swprintf(buffer, 17, L"%08X%08X", s1, s2);
	strCPUID = buffer;
	return strCPUID;
}
wstring GetMotherboardUUID()
{
	std::wstring motherboardUUID;

	// 初始化 COM
	HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (SUCCEEDED(hr))
	{
		// 初始化 WMI
		hr = CoInitializeSecurity(
			NULL,
			-1,
			NULL,
			NULL,
			RPC_C_AUTHN_LEVEL_DEFAULT,
			RPC_C_IMP_LEVEL_IMPERSONATE,
			NULL,
			EOAC_NONE,
			NULL
		);
		if (SUCCEEDED(hr))
		{
			IWbemLocator* pWbemLocator = NULL;
			hr = CoCreateInstance(
				CLSID_WbemLocator,
				0,
				CLSCTX_INPROC_SERVER,
				IID_IWbemLocator,
				(LPVOID*)&pWbemLocator
			);

			if (SUCCEEDED(hr))
			{
				IWbemServices* pWbemServices = NULL;
				hr = pWbemLocator->ConnectServer(
					_bstr_t(L"ROOT\\CIMV2"),
					NULL,
					NULL,
					0,
					NULL,
					0,
					0,
					&pWbemServices
				);

				if (SUCCEEDED(hr))
				{
					hr = CoSetProxyBlanket(
						pWbemServices,
						RPC_C_AUTHN_WINNT,
						RPC_C_AUTHZ_NONE,
						NULL,
						RPC_C_AUTHN_LEVEL_CALL,
						RPC_C_IMP_LEVEL_IMPERSONATE,
						NULL,
						EOAC_NONE
					);

					if (SUCCEEDED(hr))
					{
						// 执行 WMI 查询
						IEnumWbemClassObject* pEnumObject = NULL;
						hr = pWbemServices->ExecQuery(
							_bstr_t("WQL"),
							_bstr_t("SELECT UUID FROM Win32_ComputerSystemProduct"),
							WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
							NULL,
							&pEnumObject
						);

						if (SUCCEEDED(hr))
						{
							IWbemClassObject* pClassObject = NULL;
							ULONG uReturned = 0;

							// 获取查询结果
							hr = pEnumObject->Next(
								WBEM_INFINITE,
								1,
								&pClassObject,
								&uReturned
							);

							if (SUCCEEDED(hr) && uReturned == 1)
							{
								VARIANT vtProp;
								VariantInit(&vtProp);

								// 获取 UUID 属性
								hr = pClassObject->Get(
									_bstr_t(L"UUID"),
									0,
									&vtProp,
									0,
									0
								);

								if (SUCCEEDED(hr))
								{
									// 将 UUID 转换为 wstring
									motherboardUUID = vtProp.bstrVal;

									VariantClear(&vtProp);
								}
								pClassObject->Release();
							}
							pEnumObject->Release();
						}
					}
					pWbemServices->Release();
				}
				pWbemLocator->Release();
			}
		}
		CoUninitialize();
	}

	return motherboardUUID;
}
wstring GetMainHardDiskSerialNumber()
{
	std::wstring serialNumber;

	// 初始化 COM
	HRESULT hr = CoInitializeSecurity(
		NULL,
		-1,
		NULL,
		NULL,
		RPC_C_AUTHN_LEVEL_DEFAULT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE,
		NULL
	);
	if (SUCCEEDED(hr))
	{
		IWbemLocator* pWbemLocator = NULL;
		hr = CoCreateInstance(
			CLSID_WbemLocator,
			0,
			CLSCTX_INPROC_SERVER,
			IID_IWbemLocator,
			(LPVOID*)&pWbemLocator
		);

		if (SUCCEEDED(hr))
		{
			IWbemServices* pWbemServices = NULL;
			hr = pWbemLocator->ConnectServer(
				_bstr_t(L"ROOT\\CIMV2"),
				NULL,
				NULL,
				0,
				NULL,
				0,
				0,
				&pWbemServices
			);

			if (SUCCEEDED(hr))
			{
				hr = CoSetProxyBlanket(
					pWbemServices,
					RPC_C_AUTHN_WINNT,
					RPC_C_AUTHZ_NONE,
					NULL,
					RPC_C_AUTHN_LEVEL_CALL,
					RPC_C_IMP_LEVEL_IMPERSONATE,
					NULL,
					EOAC_NONE
				);

				if (SUCCEEDED(hr))
				{
					// 执行 WMI 查询
					IEnumWbemClassObject* pEnumObject = NULL;
					hr = pWbemServices->ExecQuery(
						_bstr_t("WQL"),
						_bstr_t("SELECT SerialNumber FROM Win32_DiskDrive WHERE Index = 0"),
						WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
						NULL,
						&pEnumObject
					);

					if (SUCCEEDED(hr))
					{
						IWbemClassObject* pClassObject = NULL;
						ULONG uReturned = 0;

						// 获取查询结果
						hr = pEnumObject->Next(
							WBEM_INFINITE,
							1,
							&pClassObject,
							&uReturned
						);

						if (SUCCEEDED(hr) && uReturned == 1)
						{
							VARIANT vtProp;
							VariantInit(&vtProp);

							// 获取硬盘序列号属性
							hr = pClassObject->Get(
								_bstr_t(L"SerialNumber"),
								0,
								&vtProp,
								0,
								0
							);

							if (SUCCEEDED(hr))
							{
								// 将序列号转换为 wstring
								serialNumber = vtProp.bstrVal;

								VariantClear(&vtProp);
							}
							pClassObject->Release();
						}
						pEnumObject->Release();
					}
				}
				pWbemServices->Release();
			}
			pWbemLocator->Release();
		}
	}

	return serialNumber;
}
//判断硬盘序列号是否错乱
bool isValidString(const wstring& str)
{
	for (wchar_t ch : str)
	{
		// 如果字符不是可打印的，并且不是空格，则认为是乱码
		if (!iswprint(ch) && !iswspace(ch))  return false;
	}
	return true;
}

//程序崩溃保护
void CrashedHandler()
{
	thread_status[L"CrashedHandler"] = true;

	//避免重复打开
	/*
	if (_waccess((string_to_wstring(global_path) + L"api\\open.txt").c_str(), 6) == 0)
	{
		off_signal = true;
		filesystem::remove(string_to_wstring(global_path) + L"api\\open.txt");
		thread_status[L"CrashedHandler"] = false;
		return;
	}
	*/

	ofstream write;
	write.imbue(locale("zh_CN.UTF8"));

	write.open(wstring_to_string(string_to_wstring(global_path) + L"api\\open.txt").c_str());
	write << 1;
	write.close();

	if (!isProcessRunning((string_to_wstring(global_path) + L"api\\智绘教CrashedHandler.exe").c_str()))
	{
		STARTUPINFOA si = { 0 };
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi = { 0 };
		CreateProcessA(NULL, (global_path + "api\\智绘教CrashedHandler.exe").data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);

		Sleep(100);
		if (!isProcessRunning((string_to_wstring(global_path) + L"api\\智绘教CrashedHandler.exe").c_str()))
		{
			WinExec((global_path + "api\\智绘教CrashedHandler.exe").c_str(), SW_NORMAL);

			Sleep(100);
			if (!isProcessRunning((string_to_wstring(global_path) + L"api\\智绘教CrashedHandler.exe").c_str())) ShellExecute(NULL, L"runas", (string_to_wstring(global_path) + L"api\\智绘教CrashedHandler.exe").c_str(), NULL, NULL, SW_SHOWNORMAL);
		}
	}

	int value = 2;

	while (!off_signal)
	{
		write.open(wstring_to_string(string_to_wstring(global_path) + L"api\\open.txt").c_str());
		write << value;
		write.close();

		value++;
		if (value > 1000) value = 1;

		Sleep(1000);
	}

	filesystem::remove(string_to_wstring(global_path) + L"api\\open.txt");
	thread_status[L"CrashedHandler"] = false;
}
//程序自动更新
wstring get_domain_name(wstring url) {
	wregex pattern(L"([a-zA-z]+://[^/]+)");
	wsmatch match;
	if (regex_search(url, match, pattern)) return match[0].str();
	else return L"";
}
wstring convertToHttp(const wstring& url)
{
	//更新保险
	if (getCurrentDate() >= L"20231020") return url;

	wstring httpPrefix = L"http://";
	if (url.length() >= 7 && url.compare(0, 7, httpPrefix) == 0) return url;
	else if (url.length() >= 8 && url.compare(0, 8, L"https://") == 0) return httpPrefix + url.substr(8);
	else return httpPrefix + url;
}

int AutomaticUpdateStep = 0;
void AutomaticUpdate()
{
	this_thread::sleep_for(chrono::seconds(3));
	while (!already) this_thread::sleep_for(chrono::milliseconds(50));

	struct
	{
		wstring edition_date;
		wstring edition_code;
		wstring explain;
		wstring representation;
		wstring path[10];
		int path_size;

		string hash_md5, hash_sha256;
	}info;
	bool state = true;
	bool update = true;

	bool against = false;

	while (1)
	{
		state = true;
		against = false;

		//获取最新版本信息
		if (state && checkIsNetwork())
		{
			filesystem::create_directory(string_to_wstring(global_path) + L"installer"); //创建路径
			filesystem::remove(string_to_wstring(global_path) + L"installer\\new_download.json");
			info.edition_date = L"";
			info.edition_code = L"";
			info.explain = L"";
			info.representation = L"";
			info.hash_md5 = "";
			info.hash_sha256 = "";
			info.path_size = 0;

			HRESULT download1;
			{
				DeleteUrlCacheEntry(L"http://home.alan-crl.top");
				download1 = URLDownloadToFileW( // 从网络上下载数据到本地文件
					nullptr,                  // 在这里，写 nullptr 就行
					(L"http://home.alan-crl.top/version_identification/official_version.json?timestamp=" + getTimestamp()).c_str(), // 在这里写上网址
					stringtoLPCWSTR(global_path + "installer\\new_download.json"),            // 文件名写在这
					0,                        // 写 0 就对了
					nullptr                   // 也是，在这里写 nullptr 就行
				);
			}

			if (download1 == S_OK)
			{
				procedure_updata_error = 1;

				Json::Reader reader;
				Json::Value root;

				ifstream injson;
				injson.imbue(locale("zh_CN.UTF8"));
				injson.open(wstring_to_string(string_to_wstring(global_path) + L"installer\\new_download.json").c_str());

				if (reader.parse(injson, root))
				{
					if (root.isMember("LTS"))
					{
						info.edition_date = string_to_wstring(convert_to_gbk(root["LTS"]["edition_date"].asString()));
						info.edition_code = string_to_wstring(convert_to_gbk(root["LTS"]["edition_code"].asString()));
						info.explain = string_to_wstring(convert_to_gbk(root["LTS"]["explain"].asString()));

						info.hash_md5 = convert_to_gbk(root["LTS"]["hash"]["md5"].asString());
						info.hash_sha256 = convert_to_gbk(root["LTS"]["hash"]["sha256"].asString());

						info.path_size = root["LTS"]["path"].size();
						for (int i = 0; i < min(info.path_size, 10); i++)
						{
							info.path[i] = string_to_wstring(convert_to_gbk(root["LTS"]["path"][i].asString()));
						}
						info.representation = string_to_wstring(convert_to_gbk(root["LTS"]["representation"].asString()));
					}
				}

				injson.close();
			}
			else state = false;

			filesystem::remove(string_to_wstring(global_path) + L"installer\\new_download.json");
		}
		else procedure_updata_error = 2;

		//下载最新版本
		if (state && checkIsNetwork() && info.edition_date != L"" && info.edition_date > string_to_wstring(edition_date))
		{
			update = true, AutomaticUpdateStep = 2;
			if (_waccess((string_to_wstring(global_path) + L"installer\\update.json").c_str(), 4) == 0)
			{
				wstring tedition, tpath;
				string thash_md5, thash_sha256;

				Json::Reader reader;
				Json::Value root;

				ifstream readjson;
				readjson.imbue(locale("zh_CN.UTF8"));
				readjson.open(wstring_to_string(string_to_wstring(global_path) + L"installer\\update.json").c_str());

				if (reader.parse(readjson, root))
				{
					tedition = string_to_wstring(convert_to_gbk(root["edition"].asString()));
					tpath = string_to_wstring(convert_to_gbk(root["path"].asString()));

					thash_md5 = convert_to_gbk(root["hash"]["md5"].asString());
					thash_sha256 = convert_to_gbk(root["hash"]["sha256"].asString());
				}

				readjson.close();

				string hash_md5, hash_sha256;
				{
					hashwrapper* myWrapper = new md5wrapper();
					hash_md5 = myWrapper->getHashFromFile(global_path + wstring_to_string(tpath));
					delete myWrapper;
				}
				{
					hashwrapper* myWrapper = new sha256wrapper();
					hash_sha256 = myWrapper->getHashFromFile(global_path + wstring_to_string(tpath));
					delete myWrapper;
				}

				if (tedition >= info.edition_date && _waccess((string_to_wstring(global_path) + tpath).c_str(), 0) == 0 && hash_md5 == thash_md5 && hash_sha256 == thash_sha256)
				{
					update = false;
				}
			}

			if (update)
			{
				filesystem::create_directory(string_to_wstring(global_path) + L"installer"); //创建路径
				filesystem::remove(string_to_wstring(global_path) + L"installer\\new_procedure.tmp");

				against = true;
				for (int i = 0; i < info.path_size; i++)
				{
					DeleteUrlCacheEntry(convertToHttp(get_domain_name(info.path[i])).c_str());

					HRESULT download2;
					{
						download2 = URLDownloadToFileW( // 从网络上下载数据到本地文件
							nullptr,                  // 在这里，写 nullptr 就行
							(convertToHttp(info.path[i]) + L"?timestamp=" + getTimestamp()).c_str(), // 在这里写上网址
							stringtoLPCWSTR(global_path + "installer\\new_procedure.tmp"),            // 文件名写在这
							0,                        // 写 0 就对了
							nullptr                   // 也是，在这里写 nullptr 就行
						);
					}

					if (download2 == S_OK)
					{
						wstring timestamp = getTimestamp();

						filesystem::remove(string_to_wstring(global_path) + L"installer\\new_procedure" + timestamp + L".exe");
						filesystem::remove(string_to_wstring(global_path) + L"installer\\" + info.representation + L".exe");

						std::error_code ec;
						filesystem::rename(string_to_wstring(global_path) + L"installer\\new_procedure.tmp", string_to_wstring(global_path) + L"installer\\new_procedure" + timestamp + L".zip", ec);
						if (ec) continue;

						{
							HZIP hz = OpenZip((string_to_wstring(global_path) + L"installer\\new_procedure" + timestamp + L".zip").c_str(), 0);
							SetUnzipBaseDir(hz, (string_to_wstring(global_path) + L"installer\\").c_str());
							ZIPENTRY ze;
							GetZipItem(hz, -1, &ze);
							int numitems = ze.index;
							for (int i = 0; i < numitems; i++)
							{
								GetZipItem(hz, i, &ze);
								UnzipItem(hz, i, ze.name);
							}
							CloseZip(hz);
						}

						filesystem::remove(string_to_wstring(global_path) + L"installer\\new_procedure" + timestamp + L".zip");
						filesystem::rename(string_to_wstring(global_path) + L"installer\\" + info.representation, string_to_wstring(global_path) + L"installer\\new_procedure" + timestamp + L".exe", ec);
						if (ec) continue;

						string hash_md5, hash_sha256;
						{
							hashwrapper* myWrapper = new md5wrapper();
							hash_md5 = myWrapper->getHashFromFile(global_path + "installer\\new_procedure" + wstring_to_string(timestamp) + ".exe");
							delete myWrapper;
						}
						{
							hashwrapper* myWrapper = new sha256wrapper();
							hash_sha256 = myWrapper->getHashFromFile(global_path + "installer\\new_procedure" + wstring_to_string(timestamp) + ".exe");
							delete myWrapper;
						}

						//创建 update.json 文件，指示更新
						if (info.hash_md5 == hash_md5 && info.hash_sha256 == hash_sha256)
						{
							Json::Value root;

							root["edition"] = Json::Value(wstring_to_string(info.edition_date));
							root["path"] = Json::Value("installer\\new_procedure" + wstring_to_string(timestamp) + ".exe");
							root["representation"] = Json::Value("new_procedure" + wstring_to_string(timestamp) + ".exe");

							root["hash"]["md5"] = Json::Value(info.hash_md5);
							root["hash"]["sha256"] = Json::Value(info.hash_sha256);

							Json::StreamWriterBuilder outjson;
							outjson.settings_["emitUTF8"] = true;
							std::unique_ptr<Json::StreamWriter> writer(outjson.newStreamWriter());
							ofstream writejson;
							writejson.imbue(locale("zh_CN.UTF8"));
							writejson.open(wstring_to_string(string_to_wstring(global_path) + L"installer\\update.json").c_str());
							writer->write(root, &writejson);
							writejson.close();

							against = false;
							AutomaticUpdateStep = 3;

							break;
						}
						else
						{
							AutomaticUpdateStep = 0;
							filesystem::remove(string_to_wstring(global_path) + L"installer\\new_procedure" + timestamp + L".exe");
						}
					}
				}
			}
		}
		else AutomaticUpdateStep = 1;

		if (against) this_thread::sleep_for(chrono::seconds(30));
		else this_thread::sleep_for(chrono::minutes(30));
	}
}

//程序注册 + 网络登记
void NetUpdate()
{
	this_thread::sleep_for(chrono::seconds(3));
	while (!already) this_thread::sleep_for(chrono::milliseconds(50));
	if (userid.empty() || userid == L"无法正确识别") return;

	thread_status[L"NetUpdate"] = true;

	//server_updata_error
	//1 网络错误
	//2 info 信息获取错误

	error_code ec;
	if (_waccess((string_to_wstring(global_path) + L"opt\\info.ini").c_str(), 4) == 0)
	{
		Json::Reader reader;
		Json::Value root;

		ifstream readjson;
		readjson.imbue(locale("zh_CN.UTF8"));
		readjson.open(wstring_to_string(string_to_wstring(global_path) + L"opt\\info.ini").c_str());

		if (reader.parse(readjson, root))
		{
			if (root.isMember("server_feedback")) server_feedback = convert_to_gbk(root["server_feedback"].asString());
			if (root.isMember("server_code")) server_code = convert_to_gbk(root["server_code"].asString());
		}
		readjson.close();
	}

	for (int i = 1; !off_signal; i++)
	{
		if (checkIsNetwork())
		{
			wstring ip, adm1, adm2;
			wstring announcement_address;

			while (1)
			{
				//获取 info 信息

				filesystem::create_directory(string_to_wstring(global_path) + L"tmp", ec); //创建路径

				DeleteUrlCacheEntry(L"http://api.vore.top");

				wstring Download1Request = L"http://api.vore.top/api/IPdata?timestamp=" + getTimestamp();
				string Download1Path = global_path + "tmp\\info.json";
				HRESULT	download1 = URLDownloadToFileW( // 从网络上下载数据到本地文件
					nullptr,
					Download1Request.c_str(),
					stringtoLPCWSTR(Download1Path),
					0,
					nullptr
				);

				if (download1 != S_OK)
				{
					server_updata_error = 2;

					server_updata_error_reason = L"发生下载错误，位于 download1 处\n目标路径 " + string_to_wstring(Download1Path) + L"\n调用报错信息 ";

					if (download1 == S_OK) server_updata_error_reason += L"0";
					else if (download1 == E_ABORT) server_updata_error_reason += L"1";
					else if (download1 == E_ACCESSDENIED) server_updata_error_reason += L"2";
					else if (download1 == E_FAIL) server_updata_error_reason += L"3";
					else if (download1 == E_HANDLE) server_updata_error_reason += L"4";
					else if (download1 == E_INVALIDARG) server_updata_error_reason += L"5";
					else if (download1 == E_NOINTERFACE) server_updata_error_reason += L"6";
					else if (download1 == E_NOTIMPL) server_updata_error_reason += L"7";
					else if (download1 == E_OUTOFMEMORY) server_updata_error_reason += L"8";
					else if (download1 == E_POINTER) server_updata_error_reason += L"9";
					else if (download1 == E_UNEXPECTED) server_updata_error_reason += L"10";
					else server_updata_error_reason += L"-1";

					break;
				}

				{
					Json::Reader reader;
					Json::Value root;

					ifstream readjson;
					readjson.imbue(locale("zh_CN.UTF8"));
					readjson.open(wstring_to_string(string_to_wstring(global_path) + L"tmp\\info.json").c_str());

					if (!(reader.parse(readjson, root) && root.isMember("code") && root["code"].asInt() == 200))
					{
						server_updata_error = 3.1;
						break;
					}
					readjson.close();
					filesystem::remove_all(string_to_wstring(global_path) + L"tmp", ec); //删除路径

					if (!(root.isMember("ipinfo") && root["ipinfo"].isObject() && root["ipinfo"].isMember("text")))
					{
						server_updata_error = 3.2;
						break;
					}
					if (!(root.isMember("ipdata") && root["ipdata"].isObject() && root["ipdata"].isMember("info1") && root["ipdata"].isMember("info2")))
					{
						server_updata_error = 3.3;
						break;
					}

					ip = string_to_wstring(convert_to_gbk(root["ipinfo"]["text"].asString()));
					adm1 = string_to_wstring(convert_to_gbk(root["ipdata"]["info1"].asString()));
					adm2 = string_to_wstring(convert_to_gbk(root["ipdata"]["info2"].asString()));
				}

				if (off_signal) break;
				//获取公告板内容

				{
					filesystem::create_directory(string_to_wstring(global_path) + L"tmp", ec); //创建路径

					DeleteUrlCacheEntry(L"http://api.a20safe.com");

					wstring Download2Request = L"http://api.a20safe.com/api.php?api=14&key=" + api_key + L"&timestamp=" + getTimestamp();
					string Download2Path = global_path + "tmp\\info.json";
					HRESULT	download2 = URLDownloadToFileW(
						nullptr,
						Download2Request.c_str(),
						stringtoLPCWSTR(Download2Path),
						0,
						nullptr
					);

					if (download2 != S_OK)
					{
						server_updata_error = 4;

						server_updata_error_reason = L"发生下载错误，位于 download2 处\n目标路径 " + string_to_wstring(Download2Path) + L"\n调用报错信息 ";

						if (download2 == S_OK) server_updata_error_reason += L"0";
						else if (download2 == E_ABORT) server_updata_error_reason += L"1";
						else if (download2 == E_ACCESSDENIED) server_updata_error_reason += L"2";
						else if (download2 == E_FAIL) server_updata_error_reason += L"3";
						else if (download2 == E_HANDLE) server_updata_error_reason += L"4";
						else if (download2 == E_INVALIDARG) server_updata_error_reason += L"5";
						else if (download2 == E_NOINTERFACE) server_updata_error_reason += L"6";
						else if (download2 == E_NOTIMPL) server_updata_error_reason += L"7";
						else if (download2 == E_OUTOFMEMORY) server_updata_error_reason += L"8";
						else if (download2 == E_POINTER) server_updata_error_reason += L"9";
						else if (download2 == E_UNEXPECTED) server_updata_error_reason += L"10";
						else server_updata_error_reason += L"-1";

						break;
					}

					{
						Json::Reader reader;
						Json::Value root;

						ifstream readjson;
						readjson.imbue(locale("zh_CN.UTF8"));
						readjson.open(wstring_to_string(string_to_wstring(global_path) + L"tmp\\info.json").c_str());

						if (!(reader.parse(readjson, root) && root.isMember("code") && root["code"].asInt() == 0))
						{
							server_updata_error = 5.1;
							break;
						}
						readjson.close();
						filesystem::remove_all(string_to_wstring(global_path) + L"tmp", ec); //删除路径

						announcement_address = string_to_wstring(convert_to_gbk(root["data"][0]["url"].asString()));
						if (announcement_address.empty())
						{
							server_updata_error = 5.2;
							break;
						}
					}
				}

				int update = 1;
				{
					filesystem::create_directory(string_to_wstring(global_path) + L"tmp", ec); //创建路径

					DeleteUrlCacheEntry(L"http://board.a20safe.com");

					wstring Download3Request = convertToHttp(announcement_address) + L"?timestamp=" + getTimestamp();
					string Download3Path = global_path + "tmp\\info.txt";
					HRESULT	download3 = URLDownloadToFileW(
						nullptr,
						Download3Request.c_str(),
						stringtoLPCWSTR(Download3Path),
						0,
						nullptr
					);

					if (download3 != S_OK)
					{
						server_updata_error = 6;

						server_updata_error_reason = L"发生下载错误，位于 download3 处\n目标路径 " + string_to_wstring(Download3Path) + L"\n调用报错信息 ";

						if (download3 == S_OK) server_updata_error_reason += L"0";
						else if (download3 == E_ABORT) server_updata_error_reason += L"1";
						else if (download3 == E_ACCESSDENIED) server_updata_error_reason += L"2";
						else if (download3 == E_FAIL) server_updata_error_reason += L"3";
						else if (download3 == E_HANDLE) server_updata_error_reason += L"4";
						else if (download3 == E_INVALIDARG) server_updata_error_reason += L"5";
						else if (download3 == E_NOINTERFACE) server_updata_error_reason += L"6";
						else if (download3 == E_NOTIMPL) server_updata_error_reason += L"7";
						else if (download3 == E_OUTOFMEMORY) server_updata_error_reason += L"8";
						else if (download3 == E_POINTER) server_updata_error_reason += L"9";
						else if (download3 == E_UNEXPECTED) server_updata_error_reason += L"10";
						else server_updata_error_reason += L"-1";

						break;
					}

					if (off_signal) break;
					{
						//Test();

						string whole;
						string tuserid, tip, tadm1, tadm2, tversion, prat;

						ifstream readjson;
						readjson.imbue(locale("zh_CN.UTF8"));
						readjson.open(wstring_to_string(string_to_wstring(global_path) + L"tmp\\info.txt").c_str());

						stringstream buffer;
						buffer << readjson.rdbuf();
						whole = buffer.str();

						readjson.close();
						filesystem::remove_all(string_to_wstring(global_path) + L"tmp", ec); //删除路径

						int l, r;
						for (int i = 0; i < (int)whole.length(); i++)
						{
						whole_start:

							l = r = -1;
							for (; i < (int)whole.length(); i++)
							{
								if (whole[i] == '[')
								{
									l = i;
									break;
								}
							}
							if (l == -1) break;

							i++, r = -1;
							for (int tmp = i; i < (int)whole.length(); i++)
							{
								if (whole[i] == ';')
								{
									r = i;
									tuserid = whole.substr(tmp, i - tmp);

									break;
								}
								else if (whole[i] == ']' || whole[i] == '[') goto whole_start;
							}
							if (r == -1) break;

							i++, r = -1;
							for (int tmp = i; i < (int)whole.length(); i++)
							{
								if (whole[i] == ';')
								{
									r = i;
									tip = whole.substr(tmp, i - tmp);

									break;
								}
								else if (whole[i] == ']' || whole[i] == '[') goto whole_start;
							}
							if (r == -1) break;

							i++, r = -1;
							for (int tmp = i; i < (int)whole.length(); i++)
							{
								if (whole[i] == ';')
								{
									r = i;
									tadm1 = whole.substr(tmp, i - tmp);

									break;
								}
								else if (whole[i] == ']' || whole[i] == '[') goto whole_start;
							}
							if (r == 0) break;

							i++, r = -1;
							for (int tmp = i; i < (int)whole.length(); i++)
							{
								if (whole[i] == ';')
								{
									r = i;
									tadm2 = whole.substr(tmp, i - tmp);

									break;
								}
								else if (whole[i] == ']' || whole[i] == '[') goto whole_start;
							}
							if (r == -1) break;

							i++, r = -1;
							for (int tmp = i; i < (int)whole.length(); i++)
							{
								if (whole[i] == ';')
								{
									r = i;
									tversion = whole.substr(tmp, i - tmp);

									break;
								}
								else if (whole[i] == ']' || whole[i] == '[') goto whole_start;
							}
							if (r == -1) break;

							i++, r = -1;
							for (int tmp = i; i < (int)whole.length(); i++)
							{
								if (whole[i] == ']')
								{
									r = i;
									prat = whole.substr(tmp, i - tmp);

									break;
								}
								else if (whole[i] == ';') goto whole_start;
							}
							if (r == -1) break;

							if (0 && string_to_wstring(tuserid) == userid && (string_to_wstring(tip) != ip || string_to_wstring(tadm1) != adm1 || string_to_wstring(tadm2) != adm2 || tversion != edition_date)) L"有意空受控语句";
							if (string_to_wstring(tuserid) == userid)
							{
								string tserver_code = "", tserver_feedback = "";
								{
									int lx, rx, inx = 0;
									for (int i = 0; i < (int)prat.length(); i++)
									{
										//起始定位
										lx = rx = -1;
										for (; i < (int)prat.length(); i++)
										{
											if (prat[i] == ',')
											{
												lx = i;
												break;
											}
											else if (i == (int)prat.length() - 1) lx = -1, inx = 1;
										}
										if (lx == -1 || inx == 1) break;

										i++, rx = -1;
										for (; i < (int)prat.length(); i++)
										{
											if (prat[i] == ',' || i == (int)prat.length() - 1)
											{
												if (i == (int)prat.length() - 1) rx = i + 1;
												else rx = i;

												tserver_code = prat.substr(lx + 1, rx - lx - 1);

												if (i == (int)prat.length() - 1) inx = 1;
												else lx = i;

												break;
											}
										}
										if (rx == -1 || inx == 1) break;

										//起始定位
										i++, rx = -1;
										for (; i < (int)prat.length(); i++)
										{
											if (prat[i] == ',' || i == (int)prat.length() - 1)
											{
												if (i == (int)prat.length() - 1) rx = i + 1;
												else rx = i;

												tserver_feedback = prat.substr(lx + 1, rx - lx - 1);

												if (i == (int)prat.length() - 1) inx = 1;
												else lx = i;

												break;
											}
										}
										if (rx == -1 || inx == 1) break;
									}
								}

								server_feedback = tserver_feedback;
								server_code = tserver_code;

								if (server_feedback != "" || server_code != "") whole.replace(l, r - l + 1, ("[" + wstring_to_string(userid) + ";" + wstring_to_string(ip) + ";" + wstring_to_string(adm1) + ";" + wstring_to_string(adm2) + ";" + edition_date + ";" + GetCurrentTimeAll() + "," + server_code + "," + server_feedback + "]"));
								else whole.replace(l, r - l + 1, ("[" + wstring_to_string(userid) + ";" + wstring_to_string(ip) + ";" + wstring_to_string(adm1) + ";" + wstring_to_string(adm2) + ";" + edition_date + ";" + GetCurrentTimeAll() + "]"));

								update = 2;

								//update = 0;
								//为了进一步监测使用情况，每次打开时数据库都会进行统计

								break;
							}

							if (tuserid.empty()) break;
						}

						//更新本地配置
						{
							Json::StyledWriter outjson;
							Json::Value root;
							root["userid"] = Json::Value(wstring_to_string(userid));
							root["ip"] = Json::Value(wstring_to_string(ip));
							root["adm1"] = Json::Value(wstring_to_string(adm1));
							root["adm2"] = Json::Value(wstring_to_string(adm2));
							root["server_feedback"] = Json::Value(convert_to_utf8(server_feedback));
							root["server_code"] = Json::Value(convert_to_utf8(server_code));

							Json::StreamWriterBuilder OutjsonBuilder;
							OutjsonBuilder.settings_["emitUTF8"] = true;
							std::unique_ptr<Json::StreamWriter> writer(OutjsonBuilder.newStreamWriter());
							ofstream writejson;
							writejson.imbue(locale("zh_CN.UTF8"));
							writejson.open(wstring_to_string(string_to_wstring(global_path) + L"opt\\info.ini").c_str());
							writer->write(root, &writejson);
							writejson.close();
						}

						if (off_signal) break;
						//更新
						if (update)
						{
							filesystem::create_directory(string_to_wstring(global_path) + L"tmp", ec); //创建路径

							if (update == 1)
							{
								if (whole.length()) whole += "\n";
								whole += ("[" + wstring_to_string(userid) + ";" + wstring_to_string(ip) + ";" + wstring_to_string(adm1) + ";" + wstring_to_string(adm2) + ";" + edition_date + ";" + GetCurrentTimeAll() + "]");
							}

							DeleteUrlCacheEntry(L"http://api.a20safe.com");
							HRESULT	download4 = URLDownloadToFileW( // 从网络上下载数据到本地文件
								nullptr,                  // 在这里，写 nullptr 就行
								(L"http://api.a20safe.com/api.php?api=14&key=" + api_key + L"&t=" + string_to_wstring(convert_to_urlencode(whole)) + L"&timestamp=" + getTimestamp()).c_str(), // 在这里写上网址
								stringtoLPCWSTR(global_path + "tmp\\info.json"),            // 文件名写在这
								0,                        // 写 0 就对了
								nullptr                   // 也是，在这里写 nullptr 就行
							);

							filesystem::remove_all(string_to_wstring(global_path) + L"tmp", ec); //删除路径
						}
					}
				}

				break;
			}
		}
		else server_updata_error = 1;

		for (int i = 1; !off_signal && i <= 1800; i++)
		{
			this_thread::sleep_for(chrono::seconds(1));
		}
	}

	thread_status[L"NetUpdate"] = false;
}