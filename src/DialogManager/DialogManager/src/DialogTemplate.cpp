#include "DialogTemplate.h"
#include "AccountManagerDialog.h"
#include "AskDialog.h"
#include "AskShowDialog.h"
#include "DownloadOperateDialog.h"
#include "InputDialog.h"
#include "TipDialog.h"
#include "TipShowDialog.h"
#include "WaitDialog.h"
#if defined(QTCONTROLS_ENABLE_WEBKIT) && (QT_VERSION <= QT_VERSION_CHECK(5,5,1))
#include "AdvertAskDialog.h"
#include "AdvertShowDialog.h"
#include "LoginShowDialog.h"
#endif
#include <new>

COriginalDialog* DialogTemplate::createDialog(DialogType dialogType)
{
	try
	{
		switch (dialogType)
		{
		case ASK_DIALOG:
			return new (std::nothrow) AskDialog;
		case INPUT_DIALOG:
			return new (std::nothrow) InputDialog;
		case TIP_DIALOG:
			return new (std::nothrow) TipDialog;
		case WAIT_DIALOG:
			return new (std::nothrow) WaitDialog;
		case DOWNLOAD_OPERATE_DIALOG:
			return new (std::nothrow) DownloadOperateDialog;
		case ASK_SHOW_DIALOG:
			return new (std::nothrow) AskShowDialog;
		case TIP_SHOW_DIALOG:
			return new (std::nothrow) TipShowDialog;
		case ACCOUNT_MANAGER_DIALOG:
			return new (std::nothrow) AccountManagerDialog;
#if defined(QTCONTROLS_ENABLE_WEBKIT) && (QT_VERSION <= QT_VERSION_CHECK(5,5,1))
		case ADVERT_ASK_DIALOG:
			return new (std::nothrow) AdvertAskDialog;
		case LOGIN_SHOW_DIALOG:
			return new (std::nothrow) LoginShowDialog;
		case ADVERT_SHOW_DIALOG:
			return new (std::nothrow) AdvertShowDialog;
#endif
		default:
			break;
		}
	}
	catch (...)
	{
		return nullptr;
	}
	return nullptr;
}

DialogShowMode DialogTemplate::showMode(DialogType dialogType)
{
	switch (dialogType)
	{
	case ASK_DIALOG:
	case INPUT_DIALOG:
	case TIP_DIALOG:
	case WAIT_DIALOG:
	case DOWNLOAD_OPERATE_DIALOG:
		return POP_DIALOG_SHOW_MODE;
	case ASK_SHOW_DIALOG:
	case TIP_SHOW_DIALOG:
		return NOTIFY_DIALOG_SHOW_MODE;
	case ACCOUNT_MANAGER_DIALOG:
		return STATIC_DIALOG_SHOW_MODE;
#if defined(QTCONTROLS_ENABLE_WEBKIT) && (QT_VERSION <= QT_VERSION_CHECK(5,5,1))
	case ADVERT_ASK_DIALOG:
		return POP_DIALOG_SHOW_MODE;
	case LOGIN_SHOW_DIALOG:
	case ADVERT_SHOW_DIALOG:
		return NOTIFY_DIALOG_SHOW_MODE;
#endif
	default:
		break;
	}
	return ERROR_DIALOG_SHOW_MODE;
}

bool DialogTemplate::isBuiltInDialogType(DialogType dialogType)
{
	return showMode(dialogType) != ERROR_DIALOG_SHOW_MODE;
}

DialogTemplate::DialogTemplate()
{

}
