#pragma once
#include <QObject>

/** 逻辑管理者
*/
class StockClientLogicManager : public QObject
{
	Q_OBJECT
protected:
	/** 构造函数
	*/
	StockClientLogicManager();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static StockClientLogicManager& instance();

	/** 获取tonghuashun路径
	@return 返回tonghuashun路径
	*/
	std::string tonghuashunPath();

Q_SIGNALS:
	/** 发送任务完成信号
	@param [in] tip 提示信息
	*/
	void taskTip(const QString tip);

protected:
	/** 初始化
	*/
	void init();
};