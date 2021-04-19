#pragma once
#include "NetSenderMacro.h"
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"

/** 网络发送接收类，调用顺序就是函数顺序
*/
class NetSenderAPI NetSender
{
protected:
	NetSender();

public:
	static NetSender& instance();

public:
	void initPostThread();

	void initReceive(ProcessReceiveCallback* callback, int32_t receiveSize = 500 * 1024, int32_t areaCount = 50);

	void init(ProtoMessage& message, CorrespondParam::ProtocolId protocolId, bool isServer = false);

	void send(ProtoMessage& message, bool isServer = false);

	void post(ProtoMessage& message, bool isServer = false);

	void uninitPostThread();

	void uninitReceive();
};