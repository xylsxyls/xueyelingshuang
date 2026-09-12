#pragma once
#include "LumaPlayerAudioRender.h"
#include "LumaPlayerVideoRender.h"
#include "LumaPlayerCoreBridge.h"
#include "LumaPlayerLogicController.h"
#include "Config.h"
#include <QtWidgets>
#include <QElapsedTimer>
// 仅测试构建统一访问权限，避免MSVC访问级别参与符号修饰造成不一致
#define private public
#include "LumaPlayer.h"
#undef private