#pragma once
#include "CustomDialogFactory.h"
#include "CustomViewFactory.h"
#include "DialogType.h"
#include <QMutex>
#include <map>
#include <memory>

class COriginalDialog;

/** 窗口工厂管理器，集中维护内置窗口创建逻辑和业务自定义窗口注册表
*/
class DialogFactoryManager
{
public:
	/** 单例入口
	@return 返回窗口工厂管理器单例
	*/
	static DialogFactoryManager& instance();

public:
	/** 注册自定义窗口工厂
	@param [in] dialogType 窗口类型ID，必须大于等于CUSTOM_DIALOG_TYPE_BEGIN
	@param [in] factory 工厂指针，注册成功后由DialogFactoryManager托管
	@param [in] destroyFunction 工厂销毁函数，传nullptr时使用delete释放
	@return 返回true表示注册成功
	*/
	bool registerCustomDialogFactory(DialogType dialogType,
									 CustomDialogFactory* factory,
									 CustomDialogFactoryDestroy destroyFunction = nullptr);

	/** 注销自定义窗口工厂
	@param [in] dialogType 窗口类型ID
	*/
	void unregisterCustomDialogFactory(DialogType dialogType);

	/** 注册自定义内容区工厂
	@param [in] dialogType 窗口类型ID，必须大于等于CUSTOM_DIALOG_TYPE_BEGIN
	@param [in] factory 工厂指针，注册成功后由DialogFactoryManager托管
	@param [in] showMode 展示模式，自定义内容区只支持模态和普通非模态两种模式
	@param [in] destroyFunction 工厂销毁函数，传nullptr时使用delete释放
	@return 返回true表示注册成功
	*/
	bool registerCustomViewFactory(DialogType dialogType,
								   CustomViewFactory* factory,
								   DialogShowMode showMode = POP_DIALOG_SHOW_MODE,
								   CustomViewFactoryDestroy destroyFunction = nullptr);

	/** 注销自定义内容区工厂
	@param [in] dialogType 窗口类型ID
	*/
	void unregisterCustomViewFactory(DialogType dialogType);

	/** 创建窗口
	@param [in] param 窗口创建参数
	@return 返回窗口指针，创建失败返回nullptr
	*/
	COriginalDialog* createDialog(const DialogParam& param);

	/** 查询窗口展示模式
	@param [in] dialogType 窗口类型ID
	@return 返回展示模式，未注册的自定义窗口返回错误模式
	*/
	DialogShowMode showMode(DialogType dialogType);

	/** 判断是否是内置窗口类型
	@param [in] dialogType 窗口类型ID
	@return 返回true表示是内置窗口
	*/
	bool isBuiltInDialogType(DialogType dialogType) const;

	/** 判断是否是自定义窗口类型
	@param [in] dialogType 窗口类型ID
	@return 返回true表示是业务自定义窗口ID
	*/
	bool isCustomDialogType(DialogType dialogType) const;

private:
	/** 自定义完整窗口工厂记录
	*/
	struct CustomDialogFactoryRecord
	{
		/** 构造函数
		*/
		CustomDialogFactoryRecord();

		/** 析构函数，在记录真正归管理器托管后释放工厂
		*/
		~CustomDialogFactoryRecord();

		/** 工厂指针，只有记录已经放入注册表后才由管理器托管
		*/
		CustomDialogFactory* m_factory;

		/** 工厂释放函数，用于兼容跨模块分配释放策略
		*/
		CustomDialogFactoryDestroy m_destroyFunction;

		/** 是否由当前记录负责释放工厂，注册失败时保持false
		*/
		bool m_isOwned;
	};

	/** 自定义内容区工厂记录
	*/
	struct CustomViewFactoryRecord
	{
		/** 构造函数
		*/
		CustomViewFactoryRecord();

		/** 析构函数，在记录真正归管理器托管后释放工厂
		*/
		~CustomViewFactoryRecord();

		/** 内容区工厂指针，只有记录已经放入注册表后才由管理器托管
		*/
		CustomViewFactory* m_factory;

		/** 展示模式
		*/
		DialogShowMode m_showMode;

		/** 工厂释放函数，用于兼容跨模块分配释放策略
		*/
		CustomViewFactoryDestroy m_destroyFunction;

		/** 是否由当前记录负责释放工厂，注册失败时保持false
		*/
		bool m_isOwned;
	};

private:
	/** 构造函数
	*/
	DialogFactoryManager();

	/** 析构函数
	*/
	~DialogFactoryManager();

	/** 禁止拷贝构造，避免注册表被复制
	@param [in] other 另一个窗口工厂管理器
	*/
	DialogFactoryManager(const DialogFactoryManager& other);

	/** 禁止赋值，避免注册表被复制
	@param [in] other 另一个窗口工厂管理器
	@return 返回当前对象引用
	*/
	DialogFactoryManager& operator=(const DialogFactoryManager& other);

	/** 创建自定义窗口
	@param [in] param 窗口创建参数
	@return 返回自定义窗口指针，创建失败返回nullptr
	*/
	COriginalDialog* createCustomDialog(const DialogParam& param);

	/** 规范化自定义窗口展示模式
	@param [in] showMode 传入的展示模式
	@return 返回可用于自定义窗口的展示模式
	*/
	DialogShowMode normalizeCustomShowMode(DialogShowMode showMode) const;

private:
	/** 自定义完整窗口工厂注册表
	*/
	std::map<DialogType, std::shared_ptr<CustomDialogFactoryRecord> > m_mapCustomDialogFactory;

	/** 自定义内容区工厂注册表
	*/
	std::map<DialogType, std::shared_ptr<CustomViewFactoryRecord> > m_mapCustomViewFactory;

	/** 保护工厂注册表的互斥锁
	*/
	QMutex m_mutex;
};
