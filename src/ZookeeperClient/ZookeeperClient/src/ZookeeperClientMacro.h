#pragma once

#ifdef _ZookeeperClientAPI
#define ZookeeperClientAPI _declspec(dllimport)
#else
#define ZookeeperClientAPI _declspec(dllexport)
#endif

#if defined(STATIC_LIB)
#undef ZookeeperClientAPI
#define ZookeeperClientAPI 
#endif