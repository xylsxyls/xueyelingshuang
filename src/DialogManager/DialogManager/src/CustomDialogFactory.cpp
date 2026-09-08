#include "CustomDialogFactory.h"
#include "CustomDialog.h"
#include "CustomViewBase.h"
#include <new>

CustomDialogFactory::~CustomDialogFactory()
{

}

CustomDialog* CustomDialogFactory::createDialog(const DialogParam& param)
{
	CustomDialog* dialog = nullptr;
	CustomViewBase* view = nullptr;
	try
	{
		dialog = new (std::nothrow) CustomDialog;
		if (dialog == nullptr)
		{
			return nullptr;
		}
		view = createView(param);
		if (view == nullptr)
		{
			delete dialog;
			return nullptr;
		}
		if (!dialog->setView(view))
		{
			delete view;
			delete dialog;
			return nullptr;
		}
		dialog->setShowMode(showMode(param));
	}
	catch (...)
	{
		delete view;
		delete dialog;
		return nullptr;
	}
	return dialog;
}

CustomViewBase* CustomDialogFactory::createView(const DialogParam& param)
{
	Q_UNUSED(param);
	return nullptr;
}

DialogShowMode CustomDialogFactory::showMode(const DialogParam& param) const
{
	Q_UNUSED(param);
	return POP_DIALOG_SHOW_MODE;
}