#pragma once
#include <stdint.h>
#include <memory>

class CTask;
/** 线程管理
*/
class ThreadManager
{
protected:
	/** 构造函数
	*/
	ThreadManager();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static ThreadManager& instance();

protected:
	/** 初始化
	*/
	void init();

public:
	/** 释放资源
	*/
	void uninit();

public:
	/** 发送删除钥匙内存任务
	@param [in] spDeleteKeyTask 删除钥匙内存任务
	*/
	void postDeleteKeyTask(const std::shared_ptr<CTask>& spDeleteKeyTask);

	/** 发送删除数据内存任务
	@param [in] spDeleteDataTask 删除数据内存任务
	*/
	void postDeleteDataTask(const std::shared_ptr<CTask>& spDeleteDataTask);

	/** 发送处理任务
	@param [in] spWorkTask 处理任务
	*/
	void postWorkTask(const std::shared_ptr<CTask>& spWorkTask);

	/** 发送接收任务
	@param [in] spReceiveTask 接收任务
	*/
	void postReceiveTask(const std::shared_ptr<CTask>& spReceiveTask);

	/** 发送创建钥匙内存任务
	@param [in] spCreateKeyTask 创建钥匙内存任务
	*/
	void postCreateKeyTask(const std::shared_ptr<CTask>& spCreateKeyTask);

	/** 发送创建数据内存任务
	@param [in] spCreateDataTask 创建数据内存任务
	*/
	void postCreateDataTask(const std::shared_ptr<CTask>& spCreateDataTask);

protected:
	uint32_t m_receiveThreadId;
	uint32_t m_workThreadId;
	uint32_t m_deleteKeyThreadId;
	uint32_t m_deleteDataThreadId;
	uint32_t m_createKeyThreadId;
	uint32_t m_createDataThreadId;
};