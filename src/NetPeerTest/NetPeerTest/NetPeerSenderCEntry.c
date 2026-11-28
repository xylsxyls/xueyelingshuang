#ifdef __cplusplus
#error NetPeerSenderCEntry.c requires a C compiler
#endif

#include "NetSender/NetSenderAPI.h"
#include <stddef.h>

/** 从统一发送器的纯C入口读取已准备客户端，并核对错误时的输出
@param [in] client 由本提供方准备的静止客户端，只借用到返回
@return 指针位宽、客户端状态、票据重叠和独立输出均符合预期时返回1
*/
int NetPeerTestCEntry(NetSenderClientHandle client)
{
    uint64_t generation = 99;
    int32_t ready = 7;
    uint64_t request = 99;
    uint64_t words[] = {31, 57, 83};
    const uint8_t* bytes = (const uint8_t*)words;
    NetInterfaceFrame frame = {7, 73, 11, NI_DATA, 2, bytes + 2 * sizeof(words[0]) - 1};
    return netPeerPointerBytes() == sizeof(void*) &&
        netSenderClientState(client, &generation, &ready) == NI_OK && generation == 0 && ready == 0 &&
        netSenderClientBegin(NULL, 7, 11, 1000, &request) == NI_INVALID_ARGUMENT && request == 0 &&
        netSenderClientSendBegin(client, 1, bytes + sizeof(words[0]) - 1, 2, &words[1]) == NI_INVALID_ARGUMENT && words[1] == 57 &&
        netSenderClientSendDataBegin(client, &frame, &words[1]) == NI_INVALID_ARGUMENT && words[1] == 57 &&
        netSenderClientSendDataBegin(client, &frame, &frame.m_requestId) == NI_INVALID_ARGUMENT && frame.m_requestId == 73 &&
        netSenderClientSendBegin(NULL, 1, bytes + 2 * sizeof(words[0]), sizeof(words[0]), &words[1]) == NI_INVALID_ARGUMENT &&
        words[1] == 0 && words[2] == 83;
}