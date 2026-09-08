#pragma once
#ifdef _WIN32
#define _DialogManagerAPI
#endif
#include "CustomDialog.h"
#include "CustomDialogFactory.h"
#include "CustomViewBase.h"
#include "CustomViewFactory.h"
#include "DialogManager.h"
#include "DialogHelper.h"
#include "ViewBase.h"

#ifdef _WIN32
#if defined _DEBUG
#pragma comment(lib,"DialogManagerd.lib")
#else
#pragma comment(lib,"DialogManager.lib")
#endif
#endif