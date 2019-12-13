#include "ZookeeperClient.h"
#include <zookeeper/zookeeper.h>
#include <string.h>
#include <thread>
#include "CStringManager/CStringManagerAPI.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

ZookeeperClient::ZookeeperClient():
m_isConnect(false),
m_handle(nullptr),
m_isListenChildrenNodeChange(false)
{

}

ZookeeperClient::~ZookeeperClient()
{
	Close();
}

int32_t ZookeeperClient::Connect(const std::string& host, int32_t recv_timeout_ms)
{
	if (m_handle != nullptr)
	{
		return E_ZM_FAIL;
	}
	m_handle = zookeeper_init(host.c_str(), Watcher, recv_timeout_ms, nullptr, this, 0);
	if (m_handle == nullptr)
	{
		return E_ZM_FAIL;
	}
	m_sem.wait();
	if (m_isConnect != 1)
	{
		return E_ZM_FAIL;
	}
	return E_ZM_OK;
}

int32_t ZookeeperClient::Close()
{
	if (m_handle == nullptr)
	{
		return E_ZM_OK;
	}
	int32_t ret = zookeeper_close(m_handle);
	m_handle = nullptr;
	return convertZookeeperResult(ret);
}

int32_t ZookeeperClient::SearchNode(const std::string& path, ZookeeperList& nodeList, watcher_fn watcher, void* watcherCtx)
{
	int32_t ret = SearchValue(path, nodeList.m_value);
	if (ret != E_ZM_OK)
	{
		return ret;
	}
	nodeList.m_name = path;
	
	String_vector strings;
	ret = zoo_wget_children(m_handle, path.c_str(), watcher, watcherCtx, &strings);
	if (ret != ZOK)
	{
		return convertZookeeperResult(ret);
	}
	nodeList.m_children.resize(strings.count);
	int32_t index = -1;
	while (index++ != strings.count - 1)
	{
		if (strlen(strings.data[index]) != 0)
		{
			ret = SearchNode(path + "/" + strings.data[index], nodeList.m_children[index]);
			if (ret != E_ZM_OK)
			{
				printf("search path error, path = %s\n", (path + "/" + strings.data[index]).c_str());
				continue;
			}
		}
	}
	deallocate_String_vector(&strings);
	return ret;
}


int32_t ZookeeperClient::SearchValue(const std::string& path, std::string& value, watcher_fn watcher, void* watcherCtx)
{
	if (!m_isConnect)
	{
		printf("[%s]connect_state_(%d)\n", __FUNCTION__, (int32_t)m_isConnect);
		return E_ZM_FAIL;
	}

	if (path.empty())
	{
		return E_ZM_OK;
	}

	char buf[1024] = {};
	// notice: len must be the size of buf, or there will appear some problem in getting the data of the node
	int32_t len = 1024;
	Stat stat;
	int32_t ret = zoo_wget(m_handle, path.c_str(), watcher, watcherCtx, buf, &len, &stat);
	if (ret != ZOK)
	{
		return convertZookeeperResult(ret);
	}
	if (stat.dataLength > len)
	{
		value.clear();
		len = stat.dataLength;
		value.resize(len);
		ret = zoo_wget(m_handle, path.c_str(), watcher, watcherCtx, &(value[0]), &len, &stat);
		if (stat.dataLength > len)
		{
			printf("zookeeper get value error, path = %s\n", path.c_str());
			return E_ZM_FAIL;
		}
		return convertZookeeperResult(ret);
	}
	value = std::string(buf, len);
	return E_ZM_OK;
}

int32_t ZookeeperClient::ListenChildrenNode(const std::string& path, bool isListenChildrenNodeChange)
{
	ZookeeperList list;
	int32_t ret = SearchNode(path, list, ListenChildrener, this);
	if (ret != E_ZM_OK)
	{
		return ret;
	}
	m_isListenChildrenNodeChange = isListenChildrenNodeChange;
	std::set<std::string> childrenNode;
	int32_t index = -1;
	while (index++ != list.m_children.size() - 1)
	{
		childrenNode.insert(list.m_children[index].m_name);
		if (m_isListenChildrenNodeChange)
		{
			ListenNode(list.m_children[index].m_name);
		}
	}
	SetChildrenMemoryNode(path, childrenNode);
	return E_ZM_OK;
}

int32_t ZookeeperClient::ListenNode(const std::string& path)
{
	std::string value;
	int32_t ret = SearchValue(path, value, Listener, this);
	if (ret != E_ZM_OK)
	{
		return ret;
	}
	SetMemoryNode(path, value);
	return E_ZM_OK;
}

int32_t ZookeeperClient::CreateNode(const std::string& path,
	const std::string& value,
	ZookeeperNodeType nodeType,
	std::string* sequentialNodeName)
{
	int32_t flag;
	switch (nodeType)
	{
	case E_PERSISTENT_NODE:
		flag = 0;
		break;
	case E_PERSISTENT_SEQUENTIAL_NODE:
		flag = ZOO_SEQUENCE;
		break;
	case E_EPHEMERAL_NODE:
		flag = ZOO_EPHEMERAL;
		break;
	case E_EPHEMERAL_SEQUENTIAL_NODE:
		flag = ZOO_EPHEMERAL | ZOO_SEQUENCE;
		break;
	default:
		return ZBADARGUMENTS;
	}
	char* buf = nullptr;
	int32_t len = 0;
	if ((nodeType == E_PERSISTENT_SEQUENTIAL_NODE || nodeType == E_EPHEMERAL_SEQUENTIAL_NODE) && sequentialNodeName != nullptr)
	{
		len = (int32_t)path.size() + 10;
		buf = new char[len + 1];
		buf[len] = 0;
	}
	int32_t ret = zoo_create(m_handle, path.c_str(),
		value.c_str(),
		(int32_t)value.size(),
		&ZOO_OPEN_ACL_UNSAFE,
		flag,
		buf,
		len);
	if (ret != ZOK)
	{
		return convertZookeeperResult(ret);
	}
	if ((nodeType == E_PERSISTENT_SEQUENTIAL_NODE || nodeType == E_EPHEMERAL_SEQUENTIAL_NODE) && sequentialNodeName != nullptr)
	{
		*sequentialNodeName = buf;
	}
	return ret;
}


int32_t ZookeeperClient::SetNode(const std::string& path, const std::string& value)
{
	int32_t ret = zoo_set(m_handle, path.c_str(), value.c_str(), (int32_t)value.size(), -1);
	return convertZookeeperResult(ret);
}

int32_t ZookeeperClient::DeleteNode(const std::string& path)
{
	int32_t ret = zoo_delete(m_handle, path.c_str(), -1);
	return convertZookeeperResult(ret);
}

int32_t ZookeeperClient::connectState() const
{
	return m_isConnect;
}

void ZookeeperClient::ListenNodeDeleted(const std::string& path, const std::string& node)
{
	printf("into listen node deleted, path = %s\n", path.c_str());
}

void ZookeeperClient::ListenNodeValueChanged(const std::string& path, const std::string& node, const std::string& value)
{
	printf("into listen node value changed, path = %s, value = %s\n", path.c_str(), value.c_str());
}

void ZookeeperClient::ListenChildrenNodeChanged(const std::string& path, const std::string& node, const ZookeeperList& zookeeperList)
{
	printf("into listen children node changed, path = %s, size = %d\n", path.c_str(), zookeeperList.m_children.size());
}

void ZookeeperClient::ListenChildrenFatherNodeDeleted(const std::string& path, const std::string& node)
{
	printf("into listen children father node deleted, path = %s\n", path.c_str());
}

void ZookeeperClient::SetMemoryNode(const std::string& path, const std::string& value)
{
	std::unique_lock<std::mutex> lock(m_nodeMutex);
	m_nodeValueMap[path] = value;
}

void ZookeeperClient::EraseMemoryNode(const std::string& path)
{
	std::unique_lock<std::mutex> lock(m_nodeMutex);
	auto itPath = m_nodeValueMap.find(path);
	if (itPath == m_nodeValueMap.end())
	{
		return;
	}
	m_nodeValueMap.erase(itPath);
}

bool ZookeeperClient::GetMemoryNode(const std::string& path, std::string& value)
{
	std::unique_lock<std::mutex> lock(m_nodeMutex);
	auto itPath = m_nodeValueMap.find(path);
	if (itPath == m_nodeValueMap.end())
	{
		value.clear();
		return false;
	}
	value = itPath->second;
	return true;
}

void ZookeeperClient::SetChildrenMemoryNode(const std::string& path, const std::set<std::string>& childrenNode)
{
	std::unique_lock<std::mutex> lock(m_childrenNodeMutex);
	m_childrenNodeMap[path] = childrenNode;
}

void ZookeeperClient::EraseChildrenFatherMemoryNode(const std::string& path)
{
	std::unique_lock<std::mutex> lock(m_childrenNodeMutex);
	auto itChildrenFatherPath = m_childrenNodeMap.find(path);
	if (itChildrenFatherPath == m_childrenNodeMap.end())
	{
		return;
	}
	m_childrenNodeMap.erase(itChildrenFatherPath);
}

bool ZookeeperClient::GetChildrenMemoryNode(const std::string& path, std::set<std::string>& childrenNode)
{
	std::unique_lock<std::mutex> lock(m_childrenNodeMutex);
	auto itChildrenFatherPath = m_childrenNodeMap.find(path);
	if (itChildrenFatherPath == m_childrenNodeMap.end())
	{
		childrenNode.clear();
		return false;
	}
	childrenNode = itChildrenFatherPath->second;
	return true;
}

void ZookeeperClient::Watcher(zhandle_t* zkh, int type, int state, const char* path, void* context)
{
	auto zm = (ZookeeperClient*)context;
	if (type == ZOO_SESSION_EVENT)
	{
		if (state == ZOO_CONNECTED_STATE)
		{
			printf("session connected\n");
			zm->m_isConnect = true;
			zm->m_sem.notify();
		}
		else if (state == ZOO_CONNECTING_STATE)
		{
			if (zm->m_isConnect)
			{
				printf("session disconnected...\n");
				//LOG_SEND_LOCAL("session disconnected...\n");
			}
			zm->m_isConnect = false;
		}
		else if (state == ZOO_EXPIRED_SESSION_STATE)
		{
			zm->m_isConnect = false;
			//LOG_SEND_LOCAL("session ZOO_EXPIRED_SESSION_STATE...\n");
		}
	}
	else if (type == ZOO_CHILD_EVENT)
	{
		// std::cout << __func__ << ": children event detected! path:" << path  <<
		// std::endl;
		printf("%s:children event detected! path:%s\n", __FUNCTION__, path);
	}
}

void ZookeeperClient::Listener(zhandle_t* zkh, int type, int state, const char* path, void* context)
{
	auto zm = (ZookeeperClient*)context;
	std::string value;
	int32_t res = zm->SearchValue(path, value, Listener, zm);
	std::string node = CStringManager::Right(path, strlen(path) - CStringManager::ReserveFind(path, '/') - 1);
	if (res != E_ZM_OK)
	{
		zm->EraseMemoryNode(path);
		zm->ListenNodeDeleted(path, node);
		return;
	}
	zm->SetMemoryNode(path, value);
	zm->ListenNodeValueChanged(path, node, value);
}

void ZookeeperClient::ListenChildrener(zhandle_t* zkh, int type, int state, const char* path, void* context)
{
	auto zm = (ZookeeperClient*)context;
	ZookeeperList nodeList;
	int32_t res = zm->SearchNode(path, nodeList, ListenChildrener, zm);
	std::string node = CStringManager::Right(path, strlen(path) - CStringManager::ReserveFind(path, '/') - 1);
	if (res != E_ZM_OK)
	{
		zm->EraseChildrenFatherMemoryNode(path);
		zm->ListenChildrenFatherNodeDeleted(path, node);
		return;
	}

	std::set<std::string> childrenNode;
	int32_t index = -1;
	while (index++ != nodeList.m_children.size() - 1)
	{
		childrenNode.insert(nodeList.m_children[index].m_name);
		if (zm->m_isListenChildrenNodeChange)
		{
			zm->ListenNode(nodeList.m_children[index].m_name);
		}
	}
	zm->SetChildrenMemoryNode(path, childrenNode);
	zm->ListenChildrenNodeChanged(path, node, nodeList);
}

int32_t ZookeeperClient::convertZookeeperResult(int32_t ret)
{
	switch (ret)
	{
	case ZOK:
		ret = E_ZM_OK;
		break;
	case ZNONODE:
		ret = E_ZM_NO_NODE;
		break;
	case ZNODEEXISTS:
		ret = E_ZM_NODE_EXISTS;
		break;
	case ZNOAUTH:
		ret = E_ZM_NO_AUTH;
		break;
	case ZNOCHILDRENFOREPHEMERALS:
		ret = E_ZM_NO_CHILDREN_FOR_EPHEMERALS;
		break;
	case ZINVALIDSTATE:
		ret = E_ZM_INVALIDSTATE;
		break;
	case ZBADARGUMENTS:
		ret = E_ZM_BADARGUMENTS;
		break;
	default:
		ret = E_ZM_FAIL;
	}
	return ret;
}

//int32_t main()
//{
//	ZookeeperClient client1;
//	int32_t res1 = client1.Connect("10.151.3.166:2181,10.151.3.166:2182,10.151.3.166:2183");
//	client1.CreateNode("/clienttest", "clienttestvalue", ZookeeperNodeType::E_PERSISTENT_NODE);
//	client1.CreateNode("/clienttest/test1", "test1value", ZookeeperNodeType::E_PERSISTENT_NODE);
//	client1.CreateNode("/clienttest/test2", "test2value", ZookeeperNodeType::E_PERSISTENT_NODE);
//	client1.CreateNode("/clienttest/test2/test1", "test2test1value", ZookeeperNodeType::E_PERSISTENT_NODE);
//	//client1.ListenNode("/clienttest");
//	ZookeeperClient client2;
//	int32_t res2 = client2.Connect("10.151.3.166:2181,10.151.3.166:2182,10.151.3.166:2183", 10000);
//	client2.ListenNode("/clienttest/test2");
//	client2.ListenChildrenNode("/clienttest/test2");
//	client1.CreateNode("/clienttest/test2/a1", "a1value", ZookeeperNodeType::E_PERSISTENT_NODE);
//	client1.CreateNode("/clienttest/test2/a2", "a2value", ZookeeperNodeType::E_EPHEMERAL_NODE);
//	client1.CreateNode("/clienttest/test2/a3", "a3value", ZookeeperNodeType::E_EPHEMERAL_NODE);
//	client1.CreateNode("/clienttest/test2/a4", "a4value", ZookeeperNodeType::E_EPHEMERAL_NODE);
//	client1.CreateNode("/clienttest/test2/a5", "a5value", ZookeeperNodeType::E_EPHEMERAL_NODE);
//	client1.CreateNode("/clienttest/test2/a6", "a6value", ZookeeperNodeType::E_EPHEMERAL_NODE);
//	client1.CreateNode("/clienttest/test2/a7", "a7value", ZookeeperNodeType::E_EPHEMERAL_NODE);
//	client1.CreateNode("/clienttest/test2/a8", "a8value", ZookeeperNodeType::E_EPHEMERAL_NODE);
//	client1.CreateNode("/clienttest/test2/a9", "a9value", ZookeeperNodeType::E_EPHEMERAL_NODE);
//	std::set<std::string> childrenNode;
//	Sleep(1000);
//	std::string value;
//	client2.GetMemoryNode("/clienttest/test2/a9", value);
//	client2.GetChildrenMemoryNode("/clienttest/test2", childrenNode);
//	//int x = client1.DeleteNode("/clienttest/test2");
//	//printf("x = %d", x);
//	//client1.SetNode("/clienttest/test2/test1", "120");
//	//client1.SetNode("/clienttest/test2/test1", "121");
//	//client1.SetNode("/clienttest/test2/test1", "122");
//	//client1.SetNode("/clienttest/test2/test1", "123");
//	//client1.SetNode("/clienttest/test2/test1", "124");
//	//client1.SetNode("/clienttest/test2/test1", "125");
//	//client1.SetNode("/clienttest/test2/test1", "126");
//	//client1.SetNode("/clienttest/test2/test1", "127");
//	//client1.SetNode("/clienttest/test2/test1", "128");
//	//client1.SetNode("/clienttest/test2/test1", "129");
//	getchar();
//	return 0;
//}