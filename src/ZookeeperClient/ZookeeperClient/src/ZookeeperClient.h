#pragma once
#include <string>
#include <vector>
#include <atomic>
#include <stdint.h>
#include "Semaphore/SemaphoreAPI.h"
#include "ZookeeperClientMacro.h"

enum ZookeeperResult
{
	E_ZM_FAIL = -1,
	E_ZM_OK = 0,  //操作成功
	E_ZM_NO_NODE = 1,  //节点不存在
	E_ZM_NODE_EXISTS = 2,  //节点已经存在
	E_ZM_NO_AUTH = 3,  //客户端（client）无权限
	E_ZM_NO_CHILDREN_FOR_EPHEMERALS = 4,  //不能够创建临时（ephemeral）节点的子节点（children）
	E_ZM_BADARGUMENTS = 5,  //存在非法的参数
	E_ZM_INVALIDSTATE = 6,  // 与zookeeper服务器session连接状态有问题
};

enum ZookeeperNodeType
{
	E_PERSISTENT_NODE = 0,  //持久节点
	E_PERSISTENT_SEQUENTIAL_NODE = 1,  //持久顺序节点
	E_EPHEMERAL_NODE = 2,  //临时节点
	E_EPHEMERAL_SEQUENTIAL_NODE = 3,  //临时顺序节点
};

struct _zhandle;
typedef struct _zhandle zhandle_t;

typedef struct ZookeeperList
{
	std::string m_name;
	std::string m_value;
	std::vector<ZookeeperList> m_children;
}ZookeeperList;

class ZookeeperClientAPI ZookeeperClient
{
public:
	/** 构造函数
	*/
	ZookeeperClient();

	/** 析构函数
	*/
	~ZookeeperClient();

public:
	/**
	* \brief create connection used for communicating with zookeeper.
	*
	* \param host comma separated host:port pairs, each corresponding to a zk
	*   server. e.g. "127.0.0.1:3000,127.0.0.1:3001,127.0.0.1:3002"
	* \param time_out millisecond
	* \param clientid the id of a previously established session that this
	*   client will be reconnecting to. Pass 0 if not reconnecting to a previous
	*   session. Clients can access the session id of an established, valid,
	*   connection by calling \ref zoo_client_id. If the session corresponding to
	*   the specified clientid has expired, or if the clientid is invalid for
	*   any reason, the returned zhandle_t will be invalid -- the zhandle_t
	*   state will indicate the reason for failure (typically
	*   ZOO_EXPIRED_SESSION_STATE).
	* \param flags reserved for future use. Should be set to zero.
	* \return 返回值为以下几种之一
	* E_ZM_OK 初始化连接成功
	* E_ZM_FAIL 初始化连接失败.
	*/
	int32_t Connect(const std::string& host, const int32_t time_out_ms = 10000);

	/**
	* \brief close the zookeeper handle and free up any resources.
	*
	* After this call, the client session will no longer be valid. The function
	* will flush any outstanding send requests before return. As a result it may
	* block.
	*
	* This method should only be called only once on a zookeeper handle. Calling
	* twice will cause undefined (and probably undesirable behavior). Calling any other
	* zookeeper method after calling close is undefined behaviour and should be avoided.
	*
	* \return
	* E_ZM_OK operation successed
	* E_ZM_FAIL operation failed
	*/
	int32_t Close();
	
	/**
	* \brief 搜索给定路径下的服务,并返回到用户传入的vector中
	*
	* \param path: 用户给定的搜索路径
	* \param source: 用户传入的参数, 如果给定路径下存在服务, 就存入到source中
	* \return the return code of the function.
	* E_ZM_OK operation completed successfully
	* E_ZM_NO_NODE the node does not exist.
	* E_ZM_NO_AUTH the client does not have permission.
	* E_ZM_BADARGUMENTS invalid input parameters
	* E_ZM_INVALIDSTATE zhandle state is either ZOO_SESSION_EXPIRED_STATE or ZOO_AUTH_FAILED_STATE
	* E_ZM_FAIL operation failed
	**/
	int32_t SearchNode(const std::string& path, ZookeeperList& nodeList);

	int32_t ListenNode(const std::string& path);

	//int32_t SearchBrokersIds(std::string& brokers);
	/**
	* \brief 在用户给定的路径下注册服务
	*
	* \param path:用户给定的路径, 包括节点名
	* \param value: 节点中的内容
	* \param node_type: 枚举值, 创建的节点类型,
	* \param node_name 实际创建的节点名
	* \return  one of the following codes are returned:
	* E_ZM_OK operation completed successfully
	* E_ZM_NO_NODE the parent node does not exist.
	* E_ZM_NODE_EXISTS the node already exists
	* E_ZM_NO_AUTH the client does not have permission.
	* E_ZM_NO_CHILDREN_FOR_EPHEMERALS cannot create children of ephemeral nodes.
	* E_ZM_BADARGUMENTS - invalid input parameters
	* E_ZM_INVALIDSTATE - zhandle state is either ZOO_SESSION_EXPIRED_STATE or ZOO_AUTH_FAILED_STATE
	* E_ZM_FAIL operation failed
	**/
	int32_t CreateNode(const std::string& path, const std::string& value, ZookeeperNodeType nodeType, std::string* sequentialNodeName = nullptr);

	int32_t SetNode(const std::string& path, const std::string& value);

	int32_t DeleteNode(const std::string& path);

	/**
	* \brief 返回与服务器连接的状态
	**/
	int32_t connectState() const;

	virtual void ListenNodeDelete(int type, int state, const char* path);

	virtual void ListenNodeChange(int type, int state, const char* path, const ZookeeperList& zookeeperList);

private:
	static void Watcher(zhandle_t* zkh, int type, int state, const char* path, void* context);

	static void Listener(zhandle_t* zkh, int type, int state, const char* path, void* context);

	int32_t convertZookeeperResult(int32_t ret);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::atomic<bool> m_isConnect;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	zhandle_t* m_handle;
	Semaphore m_sem;
};