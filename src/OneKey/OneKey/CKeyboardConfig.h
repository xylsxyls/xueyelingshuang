#pragma once
#include <stdint.h>
#include <windows.h>
#include "CStopWatch/CStopWatchAPI.h"
#include "Point/PointAPI.h"

#define SPACE 32
#define ALT 164
#define CTRL 162
#define TAB 9
#define KEY 48
#define ENTER 13
#define PLUS 107
#define MINUS 109
#define TILDE 192

#define g_keyboard CKeyboardConfig::instance()

struct CKeyboardConfig
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