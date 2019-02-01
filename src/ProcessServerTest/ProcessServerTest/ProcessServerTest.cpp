#include "ProcessServerTest.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessWork/ProcessWorkAPI.h"
#include "CGetPath/CGetPathAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "CDump/CDumpAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"

class Receive : public ReceiveCallback
{
public:
	/** �����麯��
	@param [in] buffer �����ڴ�
	@param [in] length ���ݳ���
	@param [in] sendPid ���ͽ���ID
	@param [in] protocolId ��������Э��
	*/
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId)
	{
		static int count = 0;
		++count;
		if (count % 1000000 == 0)
		{
			RCSend("count = %d, time = %d", count, ::GetTickCount());
		}
		static SharedMemory* client = nullptr;
		ProcessWork::instance().send("123456", 6, SharedMemory::readPid("ProcessClientTest.exe", client), CorrespondParam::PROTO_MESSAGE);
		//RCSend("%s,%d,%d,%d", buffer, length, sendPid, (int32_t)protocolId);
	}
};

int32_t main()
{
	CDump::declareDumpFile();
	SharedMemory::createPid();
	printf("pid = %d\n", CSystem::processPid());
	Receive receive;
	ProcessWork::instance().initReceive(&receive);
	getchar();
	return 0;
}