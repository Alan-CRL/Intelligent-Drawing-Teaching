/*
 * @file		IdtPlug-in.h
 * @brief		IDT plugin linkage | 智绘教插件联动
 * @note		PPT linkage components and other plugins | PPT联动组件和其他插件等
 *
 * @author		AlanCRL
 * @qq			2685549821
 * @email		alan-crl@foxmail.com
*/

#pragma once
#include "IdtMain.h"

// All function and variable descriptions should be in the corresponding cpp file.
// 所有的函数和变量说明应该在对应的 cpp 文件中。

extern float PPTUIScale;

struct PptImgStruct
{
	bool IsSave;
	map<int, bool> IsSaved;
	map<int, IMAGE> Image;
};
extern PptImgStruct PptImg;
struct PptInfoStateStruct
{
	long CurrentPage, TotalPage;
};
extern PptInfoStateStruct PptInfoStateBuffer;
extern PptInfoStateStruct PptInfoState;
extern bool PptWindowBackgroundUiChange;

extern wstring pptComVersion;
wstring GetPptComVersion();

void NextPptSlides(int check);
void PreviousPptSlides();
bool EndPptShow();

void GetPptState();
void PPTLinkageMain();

// --------------------------------------------------
// 插件

// DesktopDrawpadBlocker 插件
void StartDesktopDrawpadBlocker();