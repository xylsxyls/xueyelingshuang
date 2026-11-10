#pragma once
#ifdef _WIN32
#define _ProtoMessageManagerAPI
#endif
#include "ProtoMessageManager.h"
#include "ProtoMessage.h"
#include "ProtoMessageRecord.h"
#include "ProtoMessageArray.h"
#include "ProtoMessageTable.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"ProtoMessageManagerd.lib")
#else
#pragma comment(lib,"ProtoMessageManager.lib")
#endif
#endif