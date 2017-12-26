#include "DialogManager.h"
#include "DialogBase.h"

DialogManager& DialogManager::instance()
{
	static DialogManager dialogHelper;
	return dialogHelper;
}

int32_t DialogManager::setDialog(DialogBase* dialog)
{
	if (dialog == nullptr)
	{
		return -1;
	}
	int32_t id = getId();
	m_mapDialog[id] = dialog;
	return id;
}

void DialogManager::removeDialog(int32_t dialogId)
{
	auto itDialog = m_mapDialog.find(dialogId);
	if (itDialog != m_mapDialog.end())
	{
		itDialog->second->reject();
		m_mapDialog.erase(itDialog);
	}
}

void DialogManager::removeAll()
{
	for (auto itDialog = m_mapDialog.begin(); itDialog != m_mapDialog.end(); ++itDialog)
	{
		itDialog->second->reject();
	}
	m_mapDialog.clear();
}

DialogManager::DialogManager() :
m_id(0)
{

}

int32_t DialogManager::getId()
{
	return ++m_id;
}

