#include "ZookeeperClient.h"
#include <zookeeper/zookeeper.h>
#include <string.h>
#include <thread>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

ZookeeperClient::ZookeeperClient():
m_isConnect(false),
m_handle(nullptr)
{

}

ZookeeperClient::~ZookeeperClient()
{
	Close();
}

int32_t ZookeeperClient::Connect(const std::string& host, const int32_t recv_timeout_ms)
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

int32_t ZookeeperClient::SearchNode(const std::string& path, ZookeeperList& nodeList)
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
	int32_t ret = zoo_get(m_handle, path.c_str(), 0, buf, &len, &stat);
	if (ret != ZOK)
	{
		return convertZookeeperResult(ret);
	}
	if (stat.dataLength > len)
	{
		nodeList.m_value.clear();
		len = stat.dataLength;
		nodeList.m_value.resize(len);
		ret = zoo_get(m_handle, path.c_str(), 0, &(nodeList.m_value[0]), &len, &stat);
		if (ret != ZOK)
		{
			return convertZookeeperResult(ret);
		}
		if (stat.dataLength > len)
		{
			printf("zookeeper get value error, path = %s\n", path.c_str());
			return E_ZM_FAIL;
		}
	}
	else
	{
		nodeList.m_value = std::string(buf, len);
	}
	nodeList.m_name = path;
	
	String_vector strings;
	ret = zoo_get_children(m_handle, path.c_str(), 0, &strings);
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
				return ret;
			}
		}
	}
	deallocate_String_vector(&strings);
	return ret;
}

int32_t ZookeeperClient::ListenNode(const std::string& path)
{
	int32_t ret = zoo_wget_children(m_handle, path.c_str(), Listener, this, nullptr);
	return convertZookeeperResult(ret);
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

void ZookeeperClient::ListenNodeDelete(int type, int state, const char* path)
{
	printf("into listen node delete, path = %s\n", path);
}

void ZookeeperClient::ListenNodeChange(int type, int state, const char* path, const ZookeeperList& zookeeperList)
{
	printf("into listen node change, path = %s\n", path);
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
	int32_t res = zoo_wget_children(zm->m_handle, path, Listener, zm, nullptr);
	if (res != E_ZM_OK)
	{
		zm->ListenNodeDelete(type, state, path);
		return;
	}
	ZookeeperList nodeList;
	res = zm->SearchNode(path, nodeList);
	if (res != E_ZM_OK)
	{
		zm->ListenNodeDelete(type, state, path);
		return;
	}
	zm->ListenNodeChange(type, state, path, nodeList);
	//Sleep(1000);
	//LOG_SEND_LOCAL("zkh = %d,type = %d,state = %d,path = %s,context = %d", zkh, type, state, path, context);

	//emit VideoLogicManager::instance().listenChanged();

	//zoo_wget_children(zm->handle_, path, Watcher2, nullptr, nullptr);
	//LOG_SEND_LOCAL("Watcher2:%s,%d", path, context);
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
//	client1.CreateNode("/clienttest", "clienttestvalue", ZookeeperNodeType::PERSISTENT_NODE);
//	client1.CreateNode("/clienttest/test1", "test1value", ZookeeperNodeType::PERSISTENT_NODE);
//	client1.CreateNode("/clienttest/test2", "test2value", ZookeeperNodeType::PERSISTENT_NODE);
//	client1.CreateNode("/clienttest/test2/test1", "test2test1value", ZookeeperNodeType::PERSISTENT_NODE);
//	//client1.ListenNode("/clienttest");
//	ZookeeperClient client2;
//	int32_t res2 = client2.Connect("10.151.3.166:2181,10.151.3.166:2182,10.151.3.166:2183", 10000);
//	client2.ListenNode("/clienttest");
//	client1.CreateNode("/clienttest/a1", "a1value", ZookeeperNodeType::EPHEMERAL_NODE);
//	client1.DeleteNode("/clienttest/a1");
//	getchar();
//	return 0;
//}