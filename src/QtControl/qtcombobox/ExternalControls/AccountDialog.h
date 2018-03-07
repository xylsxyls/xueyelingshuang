#pragma once
#include "BoxDialogBase.h"

class Label;
class AccountDialog : public BoxDialogBase
{
public:
	AccountDialog(QWindow* parent = nullptr);

private:
	Label* m_sep;
	Label* m_registerAlt;
};