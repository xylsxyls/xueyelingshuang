#pragma once
#include "BoxDialogBase.h"

class AccountDialog;
class COriginalButton;
class AccountManagerDialog : public BoxDialogBase
{
public:
	AccountManagerDialog();

	/** µ¯³ö×¢²áÕËºÅ´°¿Ú
	*/
	void popAccountDialog();

	AccountDialog* accountDialogPtr();

private slots:
	void onHide();

private:
	AccountDialog* m_accountDialog;
	COriginalButton* m_hide;
};