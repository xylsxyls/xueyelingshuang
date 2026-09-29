#pragma once
#include "DialogManager.h"
#include <mutex>
#include <sstream>

/** 组装一条DialogManager日志并在表达式结束时交给宿主回调
*/
class DialogLog
{
public:
	/** 创建指定级别的日志消息
	@param [in] level 日志级别
	*/
	explicit DialogLog(DialogLogLevel level);

	/** 将完整消息同步交给宿主，回调异常不影响弹窗操作
	*/
	~DialogLog();

	/** 追加消息字段
	@param [in] value 需要格式化的字段
	@return 当前日志组装器
	*/
	template<typename ValueType>
	DialogLog& operator<<(const ValueType& value)
	{
		m_message << value;
		return *this;
	}

	/** 追加标准流格式控制符
	@param [in] manipulator 标准流格式控制符
	@return 当前日志组装器
	*/
	DialogLog& operator<<(std::ostream& (*manipulator)(std::ostream&));

	/** 更新宿主日志回调
	@param [in] callback 接收日志的函数指针
	*/
	static void setCallback(DialogLogCallback callback);

private:
	// 本条消息的级别
	DialogLogLevel m_level;
	// 完整消息，在析构时提交
	std::ostringstream m_message;
	// 回调状态由锁保护，调用回调时不持有该锁
	static std::mutex s_callbackMutex;
	static DialogLogCallback s_callback;
};