#pragma once

#ifdef _ProtoMessageAPI
#define ProtoMessageAPI _declspec(dllimport)
#else
#define ProtoMessageAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef ProtoMessageAPI
#define ProtoMessageAPI 
#endif