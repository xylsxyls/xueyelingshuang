#ifndef PROTO_MESSAGE_API_INCLUDED
#define PROTO_MESSAGE_API_INCLUDED
#ifdef _WIN32
#define _ProtoMessageAPI
#endif
#include "ProtoMessage.h"

#ifdef NET_GATEWAY_TEST_ACCESS
// 仅供静态测试构造非法线上输入和验证第三方初始化，不属于业务接入接口。
#include "ProtoMessage.pb.h"
#include <google/protobuf/stubs/once.h>
#endif

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"ProtoMessaged.lib")
#else
#pragma comment(lib,"ProtoMessage.lib")
#endif
#endif

#endif