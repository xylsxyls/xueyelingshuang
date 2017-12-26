#pragma once
#include <map>
#include <stdint.h>

class DialogBase;
/** 单一实例，用于统一管理窗口创建关闭，该类的对外接口，销毁所有窗口
DialogManager::instance().removeAll();
*/
class DialogManager
{
public:
	/** 单一实例
	@return 返回单一实例
	*/
	static DialogManager& instance();

	/** 存储窗口指针
	@param [in] dialog 窗口指针
	@return 返回窗口对应ID号
	*/
	int32_t setDialog(DialogBase* dialog);

	/** 根据ID号销毁窗口
	@param [in] dialogId 窗口ID号
	*/
	void removeDialog(int32_t dialogId);

	/** 销毁所有窗口
	*/
	void removeAll();

private:
	DialogManager();

	int32_t getId();

private:
	std::map<int32_t, DialogBase*> m_mapDialog;
	int32_t m_id;
};