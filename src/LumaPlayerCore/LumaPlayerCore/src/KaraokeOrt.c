#include "KaraokeOrt.h"
#include "KaraokeOrtContext.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#if defined(LUMAPLAYER_ORT_LINKED) && defined(_WIN32)
#error LUMAPLAYER_ORT_LINKED is only supported by the POSIX target-SDK build
#endif
#if defined(LUMAPLAYER_ORT_LINKED)
/* iOS and embedded targets link the platform's ORT library at build time. */
#elif defined(_WIN32)
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

/** 有界复制并终止诊断字符串，不分配内存
@param [out] output 输出缓冲区或结果对象，非空时容量须满足声明，借用至返回
@param [in] capacity 诊断缓冲区字节数，包含终止符
@param [in] text 有效诊断字符串，借用至返回
*/
static void OrtError(char* output, size_t capacity, const char* text)
{
    if (output != NULL && capacity > 0)
    {
        size_t length = strlen(text);
        if (length >= capacity)
        {
            length = capacity - 1;
        }
        memcpy(output, text, length);
        output[length] = 0;
    }
}

/** 消费ORT状态并将失败诊断交给调用方
@param [in] context 有效ORT会话上下文
@param [in] status ORT状态，可为空，非空时本函数消费释放
@param [out] error 可为空的诊断输出缓冲区或对象
@param [in] capacity 诊断缓冲区字节数，包含终止符
@return 非零表示没有ORT错误，零表示消费失败状态并写入诊断
*/
static int OrtCheck(KaraokeOrtContext* context, OrtStatus* status, char* error, size_t capacity)
{
    if (status == NULL)
    {
        return 1;
    }
    OrtError(error, capacity, context->m_api->GetErrorMessage(status));
    context->m_api->ReleaseStatus(status);
    return 0;
}

void karaokeOrtDestroy(void* handle)
{
    KaraokeOrtContext* context = (KaraokeOrtContext*)handle;
    if (context == NULL)
    {
        return;
    }
    if (context->m_api != NULL)
    {
        if (context->m_inputName != NULL)
        {
            context->m_allocator->Free(context->m_allocator, context->m_inputName);
        }
        if (context->m_outputName != NULL)
        {
            context->m_allocator->Free(context->m_allocator, context->m_outputName);
        }
        if (context->m_memory != NULL)
        {
            context->m_api->ReleaseMemoryInfo(context->m_memory);
        }
        if (context->m_session != NULL)
        {
            context->m_api->ReleaseSession(context->m_session);
        }
        if (context->m_options != NULL)
        {
            context->m_api->ReleaseSessionOptions(context->m_options);
        }
        if (context->m_env != NULL)
        {
            context->m_api->ReleaseEnv(context->m_env);
        }
    }
#if defined(LUMAPLAYER_ORT_LINKED)
#elif defined(_WIN32)
    if (context->m_library != NULL)
    {
        FreeLibrary((HMODULE)context->m_library);
    }
#else
    if (context->m_library != NULL)
    {
        dlclose(context->m_library);
    }
#endif
    free(context);
}

/** 核对模型的名称、元素类型和固定张量形状
@param [in] context 有效ORT会话上下文
@param [out] error 可为空的诊断输出缓冲区或对象
@param [in] capacity 诊断缓冲区字节数，包含终止符
@return 非零表示布局匹配，零表示诊断失败
*/
static int OrtValidateModel(KaraokeOrtContext* context, char* error, size_t capacity)
{
    int output;
    for (output = 0; output < 2; ++output)
    {
        size_t count = 0, dimensions = 0;
        int64_t shape[4] = {0};
        OrtTypeInfo* info = NULL;
        const OrtTensorTypeAndShapeInfo* tensor = NULL;
        ONNXTensorElementDataType type;
        int valid = OrtCheck(context, output ? context->m_api->SessionGetOutputCount(context->m_session, &count) : context->m_api->SessionGetInputCount(context->m_session, &count), error, capacity);
        if (!valid || count != 1)
        {
            goto invalid;
        }
        valid = OrtCheck(context, output ? context->m_api->SessionGetOutputTypeInfo(context->m_session, 0, &info) : context->m_api->SessionGetInputTypeInfo(context->m_session, 0, &info), error, capacity);
        if (valid)
        {
            valid = OrtCheck(context, context->m_api->CastTypeInfoToTensorInfo(info, &tensor), error, capacity);
        }
        if (valid && tensor)
        {
            valid = OrtCheck(context, context->m_api->GetDimensionsCount(tensor, &dimensions), error, capacity);
        }
        if (!valid || !tensor || dimensions != 4)
        {
            goto invalid;
        }
        valid = OrtCheck(context, context->m_api->GetDimensions(tensor, shape, 4), error, capacity) &&
            OrtCheck(context, context->m_api->GetTensorElementType(tensor, &type), error, capacity);
        if (!valid || shape[0] == 0 || shape[0] > 1 || shape[1] != 4 || shape[2] != 2560 || shape[3] != 256 ||
            type != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
        {
            goto invalid;
        }
        context->m_api->ReleaseTypeInfo(info);
        continue;
    invalid:
        if (info)
        {
            context->m_api->ReleaseTypeInfo(info);
        }
        OrtError(error, capacity, "Incompatible model; requires MDX-HQ5 float [batch,4,2560,256] input/output");
        return 0;
    }
    return 1;
}

void* karaokeOrtCreate(const char* runtimePath, const char* modelPath, char* error, size_t capacity)
{
    typedef const OrtApiBase*(ORT_API_CALL * GetApiFn)(void);
    typedef OrtStatus*(ORT_API_CALL * AppendDmlFn)(OrtSessionOptions*, int);
    GetApiFn getApi = NULL;
    AppendDmlFn appendDml = NULL;
    KaraokeOrtContext* context = (KaraokeOrtContext*)calloc(1, sizeof(KaraokeOrtContext));
#ifdef _WIN32
    wchar_t runtimeWide[32768];
    wchar_t modelWide[32768];
#endif
    if (context == NULL || runtimePath == NULL || modelPath == NULL)
    {
        free(context);
        OrtError(error, capacity, "Invalid runtime/model or allocation failure");
        return NULL;
    }
#if defined(LUMAPLAYER_ORT_LINKED) && !defined(_WIN32)
    getApi = &OrtGetApiBase;
#elif defined(_WIN32)
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, runtimePath, -1, runtimeWide, 32768) ||
        !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, modelPath, -1, modelWide, 32768))
    {
        OrtError(error, capacity, "Invalid UTF-8 model/runtime path");
        goto fail;
    }
    // Use explicit dependency directories; native file dialogs can change the process CWD.
    context->m_library = LoadLibraryExW(runtimeWide, NULL, 0x00000100 | 0x00001000);
    if (context->m_library == NULL)
    {
        const DWORD code = GetLastError();
        char message[1024];
        _snprintf_s(message, sizeof(message), _TRUNCATE,
            "ONNX Runtime load failed (Win32=%lu, API=%u): %s", code, ORT_API_VERSION, runtimePath);
        OrtError(error, capacity, message);
        goto fail;
    }
    if (context->m_library != NULL)
    {
        getApi = (GetApiFn)GetProcAddress((HMODULE)context->m_library, "OrtGetApiBase");
        appendDml = (AppendDmlFn)GetProcAddress((HMODULE)context->m_library, "OrtSessionOptionsAppendExecutionProvider_DML");
    }
#else
    context->m_library = dlopen(runtimePath, RTLD_NOW | RTLD_LOCAL);
    if (context->m_library != NULL)
    {
        getApi = (GetApiFn)dlsym(context->m_library, "OrtGetApiBase");
    }
#endif
    if (getApi == NULL)
    {
        OrtError(error, capacity, "Cannot load ONNX Runtime or find OrtGetApiBase; check target architecture and dependencies");
        goto fail;
    }
    context->m_api = getApi()->GetApi(ORT_API_VERSION);
    if (context->m_api == NULL)
    {
        OrtError(error, capacity, "ONNX Runtime API mismatch; requires API 24 from ONNX Runtime 1.24 or newer");
        goto fail;
    }
#define ORT_CHECK(expression)                              \
    if (!OrtCheck(context, (expression), error, capacity)) \
    goto fail
    ORT_CHECK(context->m_api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "LumaKaraoke", &context->m_env));
    ORT_CHECK(context->m_api->CreateSessionOptions(&context->m_options));
    ORT_CHECK(context->m_api->SetIntraOpNumThreads(context->m_options, appendDml != NULL ? 1 : 4));
    ORT_CHECK(context->m_api->SetSessionExecutionMode(context->m_options, ORT_SEQUENTIAL));
    if (appendDml != NULL)
    {
        ORT_CHECK(context->m_api->DisableMemPattern(context->m_options));
        ORT_CHECK(appendDml(context->m_options, 0));
    }
#ifdef _WIN32
    ORT_CHECK(context->m_api->CreateSession(context->m_env, modelWide, context->m_options, &context->m_session));
#else
    ORT_CHECK(context->m_api->CreateSession(context->m_env, modelPath, context->m_options, &context->m_session));
#endif
    if (!OrtValidateModel(context, error, capacity))
    {
        goto fail;
    }
    ORT_CHECK(context->m_api->GetAllocatorWithDefaultOptions(&context->m_allocator));
    ORT_CHECK(context->m_api->SessionGetInputName(context->m_session, 0, context->m_allocator, &context->m_inputName));
    ORT_CHECK(context->m_api->SessionGetOutputName(context->m_session, 0, context->m_allocator, &context->m_outputName));
    ORT_CHECK(context->m_api->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &context->m_memory));
    return context;
fail:
    karaokeOrtDestroy(context);
    return NULL;
#undef ORT_CHECK
}

int karaokeOrtRun(void* handle, const float* spectrum, float* prediction, char* error, size_t capacity)
{
    KaraokeOrtContext* context = (KaraokeOrtContext*)handle;
    OrtValue* input = NULL;
    OrtValue* output = NULL;
    OrtTensorTypeAndShapeInfo* info = NULL;
    const int64_t shape[4] = {1, 4, 2560, 256};
    const size_t elements = 4 * 2560 * 256;
    size_t outputElements = 0;
    size_t dimensions = 0;
    int64_t outputShape[4] = {0};
    void* data = NULL;
    int result = 0;
    ONNXTensorElementDataType type;
    if (context == NULL || spectrum == NULL || prediction == NULL)
    {
        return 0;
    }
#define ORT_CHECK(expression)                              \
    if (!OrtCheck(context, (expression), error, capacity)) \
    goto done
    ORT_CHECK(context->m_api->CreateTensorWithDataAsOrtValue(context->m_memory, (void*)spectrum,
        elements * sizeof(float), shape, 4, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &input));
    ORT_CHECK(context->m_api->Run(context->m_session, NULL, (const char* const*)&context->m_inputName,
        (const OrtValue* const*)&input, 1, (const char* const*)&context->m_outputName, 1, &output));
    ORT_CHECK(context->m_api->GetTensorTypeAndShape(output, &info));
    ORT_CHECK(context->m_api->GetTensorShapeElementCount(info, &outputElements));
    ORT_CHECK(context->m_api->GetDimensionsCount(info, &dimensions));
    ORT_CHECK(context->m_api->GetTensorElementType(info, &type));
    if (outputElements != elements || dimensions != 4 || type != ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)
    {
        OrtError(error, capacity, "Unexpected MDX instrumental shape/type");
        goto done;
    }
    ORT_CHECK(context->m_api->GetDimensions(info, outputShape, 4));
    if (outputShape[0] != 1 || outputShape[1] != 4 || outputShape[2] != 2560 || outputShape[3] != 256)
    {
        OrtError(error, capacity, "Unexpected MDX instrumental dimensions");
        goto done;
    }
    ORT_CHECK(context->m_api->GetTensorMutableData(output, &data));
    memcpy(prediction, data, elements * sizeof(float));
    result = 1;
done:
    if (info != NULL)
    {
        context->m_api->ReleaseTensorTypeAndShapeInfo(info);
    }
    if (output != NULL)
    {
        context->m_api->ReleaseValue(output);
    }
    if (input != NULL)
    {
        context->m_api->ReleaseValue(input);
    }
    return result;
#undef ORT_CHECK
}