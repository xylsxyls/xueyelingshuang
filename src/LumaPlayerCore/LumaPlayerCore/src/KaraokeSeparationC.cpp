#include "LumaPlayerCoreC.h"
#include "LumaPlayerCoreCContext.h"
#include "KaraokeSeparation.h"
#include <algorithm>
#include <cstring>
#include <exception>

typedef std::shared_ptr<const KaraokeBacking> BackingHandle;

LumaPlayerCoreCBacking lumaPlayerCoreSeparateVocals(const char* source, const char* runtime,
    const char* model, size_t limit, LumaPlayerCoreCSeparationProgress callback, void* user,
    char* error, size_t capacity)
{
    return lumaPlayerCoreSeparateVocalsEx(source, runtime, model, limit, nullptr, callback, user, error, capacity);
}

LumaPlayerCoreCBacking lumaPlayerCoreSeparateVocalsEx(const char* source, const char* runtime,
    const char* model, size_t limit, const LumaPlayerCoreCSeparationOptions* options,
    LumaPlayerCoreCSeparationProgress callback, void* user, char* error, size_t capacity)
{
    if (options && options->m_size != sizeof(*options))
    {
        if (error != nullptr && capacity > 0)
        {
            const char message[] = "Invalid separation options";
            const size_t count = (std::min)(capacity - 1, sizeof(message) - 1);
            std::memcpy(error, message, count);
            error[count] = 0;
        }
        return nullptr;
    }
    try
    {
        std::string diagnostic;
        BackingHandle value = KaraokeSeparation::run(source, runtime, model, limit, callback, user, &diagnostic,
            options ? options->m_windowFrames : 261120, options ? options->m_overlapFrames : 130560);
        if (error != nullptr && capacity > 0)
        {
            const size_t count = (std::min)(capacity - 1, diagnostic.size());
            std::memcpy(error, diagnostic.data(), count);
            error[count] = 0;
        }
        return value ? new BackingHandle(value) : nullptr;
    }
    catch (const std::exception& exception)
    {
        if (error != nullptr && capacity > 0)
        {
            const size_t count = (std::min)(capacity - 1, std::strlen(exception.what()));
            std::memcpy(error, exception.what(), count);
            error[count] = 0;
        }
        return nullptr;
    }
    catch (...)
    {
        if (error != nullptr && capacity > 0)
        {
            const char message[] = "Unexpected vocal separation failure";
            const size_t count = (std::min)(capacity - 1, sizeof(message) - 1);
            std::memcpy(error, message, count);
            error[count] = 0;
        }
        return nullptr;
    }
}

void lumaPlayerCoreBackingDestroy(LumaPlayerCoreCBacking backing)
{
    delete static_cast<BackingHandle*>(backing);
}

int32_t lumaPlayerCoreBackingApply(LumaPlayerCoreHandle handle, const char* source, LumaPlayerCoreCBacking backing)
{
    if (handle == nullptr || source == nullptr)
    {
        return LumaPlayerCoreCResultInvalidParam;
    }
    try
    {
        LumaPlayerCoreCContext* context = static_cast<LumaPlayerCoreCContext*>(handle);
        return context->m_core->setKaraokeBacking(source, backing ? *static_cast<BackingHandle*>(backing) : BackingHandle())
            ? LumaPlayerCoreCResultSuccess
            : LumaPlayerCoreCResultInvalidParam;
    }
    catch (...)
    {
        return LumaPlayerCoreCResultInternalError;
    }
}

size_t lumaPlayerCoreBackingRead(LumaPlayerCoreCBacking backing, uint64_t offset, float* output, size_t capacity)
{
    if (backing == nullptr || output == nullptr)
    {
        return 0;
    }
    try
    {
        const std::shared_ptr<KaraokeStorage> samples = (*static_cast<BackingHandle*>(backing))->m_samples;
        if (!samples || offset >= samples->size() / 8)
        {
            return 0;
        }
        const size_t count = static_cast<size_t>((std::min<uint64_t>)(capacity, samples->size() / 8 - offset));
        return samples->read(offset * 8, output, count * 8) ? count : 0;
    }
    catch (...)
    {
        return 0;
    }
}