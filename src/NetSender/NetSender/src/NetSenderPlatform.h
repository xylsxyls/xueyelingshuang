#pragma once
// src/NetSender仅提供桌面ProcessWork；进程内接入使用构建选定的NetSender/NetSenderAPI.h
#if defined(NET_SENDER_IN_PROCESS) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#error Select the NetWork NetSender provider for same-process networking
#endif