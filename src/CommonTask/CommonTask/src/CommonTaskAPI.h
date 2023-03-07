#pragma once
#ifdef _WIN32
#define _CommonTaskAPI
#endif
#include "SleepBaseTask.h"
#include "SleepTask.h"
#include "MoveTask.h"
#include "MoveClickTask.h"
#include "KeyTask.h"
#include "ClickTask.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"CommonTaskd.lib")
#else
#pragma comment(lib,"CommonTask.lib")
#endif
#endif