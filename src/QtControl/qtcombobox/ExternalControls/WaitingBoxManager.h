#pragma once
#include <QObject>
#include <stdint.h>
#include <map>
#include "WaitingBox.h"

class QWindow;
class WaitingBox;
/** 定做类，等待框管理类
*/
class WaitingBoxManager : public QObject
{
	Q_OBJECT
public:
	/** 单一实例
	@return 返回单一实例
	*/
	static WaitingBoxManager& instance();

public:
	/** 显示等待框，不阻塞代码但是阻塞界面
	@param [in] key 用户传入的窗口ID，用户保证唯一性
	@param [in] title 窗口标题
	@param [in] tip 提示
	@param [in] parent 临时父窗口句柄
	@param [in] timeOut 自动消失时间（毫秒）
	@param [in] userExit 用户是否可以主动关闭
	*/
	void showWaitingBox(int32_t key,
						const QString& title,
						const QString& tip,
						QWindow* parent = nullptr,
						int32_t timeOut = 3000,
						bool userExit = false);

	/** 关闭弹窗，支持多线程
	@param [in] key 用户给定key值
	*/
	void closeWaitingBox(int32_t key);

	/** 销毁所有创建的等待框
	*/
	void destroyAll();

Q_SIGNALS:
	/** 当等待框关闭时发送信号
	@param [in] key 用户给定key值
	@param [in] exitEnum 关闭类型
	*/
	void waitingBoxDone(int32_t key, WaitingBox::ExitEnum exitEnum);

private slots:
	void onWaitingBoxDone(WaitingBox* box, WaitingBox::ExitEnum exitEnum);

private:
	std::map<int32_t, WaitingBox*> m_mapWaitingBox;

};