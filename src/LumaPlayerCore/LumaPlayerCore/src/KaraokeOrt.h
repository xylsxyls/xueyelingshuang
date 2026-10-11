#pragma once
#include <stddef.h>
#ifdef __cplusplus
extern "C"
{
#endif

    /** Load runtime/model synchronously on a worker; UTF-8 paths, caller owns error buffer
    @param [in] runtimePath 运行时UTF-8路径，借用至返回
    @param [in] modelPath 模型UTF-8路径，借用至返回
    @param [out] error 可为空的诊断输出缓冲区或对象
    @param [in] errorSize 错误缓冲区字节数，包含终止符
    @return 会话句柄，失败为空并写入诊断
    */
    void* karaokeOrtCreate(const char* runtimePath, const char* modelPath, char* error, size_t errorSize);

    /** MDX-HQ5 [1,4,2560,256] spectrum in/out; output is instrumental, not a mask
    @param [in] context 有效的模型会话指针，借用至返回
    @param [in] spectrum MDX浮点张量，布局为1×4×2560×256
    @param [out] prediction 接收同布局的模型推理输出
    @param [out] error 可为空的诊断输出缓冲区或对象
    @param [in] errorSize 错误缓冲区字节数，包含终止符
    @return 非零表示完成，零表示失败并写入诊断
    */
    int karaokeOrtRun(void* context, const float* spectrum, float* prediction, char* error, size_t errorSize);

    /** Release only after run has returned; null allowed
    @param [in] context 有效的模型会话指针，借用至返回
    */
    void karaokeOrtDestroy(void* context);

#ifdef __cplusplus
}
#endif