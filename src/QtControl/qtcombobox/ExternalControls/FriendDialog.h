#pragma once
#include "../CW3LModalFrame.h"

class COriginalButton;
class FriendDialog : public CW3LModalFrame
{
public:
	FriendDialog();

private:
	COriginalButton* m_exit;
	COriginalButton* m_bottomExit;
};