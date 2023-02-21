#pragma once
#include <stdint.h>
#include <windows.h>
#include "CStopWatch/CStopWatchAPI.h"
#include "CKeyboardConfigMacro.h"

//¿Õ¸ñ
#define SPACE 32
//alt
#define ALT 164
//ctrl
#define CTRL 162
//tab
#define TAB 9
//Ð¡¼üÅÌÆôÊ¼
#define KEY 48
//»Ø³µ
#define ENTER 13
//Ð¡¼üÅÌ+ºÅ
#define PLUS 107
//Ð¡¼üÅÌ-ºÅ
#define MINUS 109
//²¨ÀËºÅ
#define TILDE 192
//delete¼ü
#define DEL 46

#define g_keyboard CKeyboardConfig::instance()

struct CKeyboardConfigAPI CKeyboardConfig
{
	int32_t m_wParam;
	int32_t m_lParam;

	bool m_keyDown[256];
	bool m_keyUp[256];
	bool m_keyHasDown[256];
	CStopWatch m_keyWatch[256];

	unsigned char m_preKey = 0;
	unsigned char m_lastKey = 0;
	unsigned char m_currentKey = 0;

	CKeyboardConfig();

public:
	static CKeyboardConfig& instance();

public:
	void acceptParam(WPARAM wParam, LPARAM lParam);
};