#pragma once
#include "onnxruntime/onnxruntime_c_api.h"

/** Internal C-only runtime ownership; not exposed outside Core
*/
typedef struct KaraokeOrtContext
{
    // 拥有的运行时动态库引用，静态链接目标为空
    void* m_library;
    // 运行时API表，借用至动态库卸载
    const OrtApi* m_api;
    // 拥有的ORT环境
    OrtEnv* m_env;
    // 拥有的ORT会话设置
    OrtSessionOptions* m_options;
    // 拥有的模型会话
    OrtSession* m_session;
    // 拥有的CPU张量内存描述
    OrtMemoryInfo* m_memory;
    // 运行时默认分配器，借用不单独销毁
    OrtAllocator* m_allocator;
    // 默认分配器返回的输入名称，销毁时归还
    char* m_inputName;
    // 默认分配器返回的输出名称，销毁时归还
    char* m_outputName;
} KaraokeOrtContext;