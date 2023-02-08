#include "stdafx.h"
#include "CKeyboardConfig.h"
#include "CHook/CHookAPI.h"
#include "Config.h"
#include "D:\\SendToMessageTest.h"

CKeyboardConfig::CKeyboardConfig():
m_wParam(0),
m_lParam(0),
m_preKey(0),
m_lastKey(0),
m_currentKey(0)
{
	::memset(m_keyDown, 0, sizeof(m_keyDown));
	::memset(m_keyUp, 0, sizeof(m_keyUp));
	::memset(m_keyHasDown, 0, sizeof(m_keyHasDown));
}

CKeyboardConfig& CKeyboardConfig::instance()
{
	static CKeyboardConfig s_ckeyboardConfig;
	return s_ckeyboardConfig;
}

void CKeyboardConfig::acceptParam(WPARAM wParam, LPARAM lParam)
{
	m_wParam = wParam;
	m_lParam = lParam;

	const DWORD& vkCode = CHook::GetVkCode(lParam);
	if (g_config.m_isPrintVkCode)
	{
		RCSend("vkCode = %d", (int32_t)vkCode);
	}

	::memset(m_keyDown, 0, sizeof(m_keyDown));
	::memset(m_keyUp, 0, sizeof(m_keyUp));

	if (CHook::IsKeyDown(wParam))
	{
		if (vkCode <= 255)
		{
			if (!m_keyHasDown[vkCode])
			{
				m_keyDown[vkCode] = true;
				m_keyHasDown[vkCode] = true;
				m_keyWatch[vkCode].SetWatchTime(0);
				m_preKey = m_lastKey;
				m_lastKey = m_currentKey;
				m_currentKey = (unsigned char)vkCode;
			}
		}
	}
	else if (CHook::IsKeyUp(wParam))
	{
		if (vkCode <= 255)
		{
			if (m_keyHasDown[vkCode])
			{
				m_keyUp[vkCode] = true;
				m_keyHasDown[vkCode] = false;
			}
		}
	}
}