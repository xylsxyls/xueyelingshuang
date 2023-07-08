#pragma once
#include <string>
#include <vector>
#include <stdint.h>
#include <windows.h>
#include "CKeyboardMacro.h"

class CKeyboardAPI CKeyboard
{
public:
	enum
	{
		Esc = VK_ESCAPE,
		F1 = VK_F1,
		F2 = VK_F2,
		F3 = VK_F3,
		F4 = VK_F4,
		F5 = VK_F5,
		F6 = VK_F6,
		F7 = VK_F7,
		F8 = VK_F8,
		F9 = VK_F9,
		F10 = VK_F10,
		F11 = VK_F11,
		F12 = VK_F12,
		PrintScreen = VK_PRINT,
		ScrollLock = VK_SCROLL,
		Pause = VK_PAUSE,
		Backspace = VK_BACK,
		Insert = VK_INSERT,
		Home = VK_HOME,
		PageUp = VK_PRIOR,
		Delete = VK_DELETE,
		End = VK_END,
		PageDown = VK_NEXT,
		NumLock = VK_NUMLOCK,
		CapsLock = VK_CAPITAL,
		Shift = VK_SHIFT,
		Up = VK_UP,
		Left = VK_LEFT,
		Down = VK_DOWN,
		Right = VK_RIGHT,
		Ctrl = VK_CONTROL,
		Win = VK_LWIN,
		Alt = VK_MENU,
		SPACE = VK_SPACE,
		MouseRightClick = VK_APPS,
		ENTER = VK_RETURN
	};
public:
	//?按下键盘
	static bool KeyDown(int32_t code);
	//?弹起键盘
	static bool KeyUp(int32_t code);
	//?按下弹起，完成时间，默认100-300毫秒
	static bool KeyPress(int32_t code, int32_t sleepTime = -1);
	//?输入字符串，间隔时间，支持所有单个字符输入，包括!@之类，但是都是英文字符
	static bool InputString(const std::string& str, int32_t sleepTime = -1);
	//?按下组合键，完成时间，Ctrl+Alt+Delete无法模拟，如果想调出任务管理器可以使用Ctrl+Shift+Esc
	static bool KeyManyPress(const std::vector<int32_t>& vecCode, int32_t sleepTime = -1);
	//?输入中文字符串

	//?获取键码，遇到不是英文字符的返回-1
	static std::vector<int32_t> GetCode(char c);
};