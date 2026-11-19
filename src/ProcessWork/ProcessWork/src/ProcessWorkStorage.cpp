#include "ProcessWorkStorage.h"
#include "ProcessWorkAtomic.h"
#include <cstring>
#include <thread>

ProcessWorkPage::ProcessWorkPage() :
m_metadata(nullptr)
{
    for (size_t i = 0; i < 256; ++i)
    {
        m_data[i].store(nullptr, std::memory_order_relaxed);
        m_addresses[i].store(nullptr, std::memory_order_relaxed);
        m_idleSince[i] = 0;
    }
}

ProcessWorkPage::~ProcessWorkPage()
{
    for (size_t i = 0; i < 256; ++i)
    {
        delete m_data[i].load(std::memory_order_relaxed);
    }
}

ProcessWorkPageGroup::ProcessWorkPageGroup()
{
    for (size_t i = 0; i < 256; ++i)
    {
        m_pages[i].store(nullptr, std::memory_order_relaxed);
    }
}

ProcessWorkPageGroup::~ProcessWorkPageGroup()
{
    for (size_t i = 0; i < 256; ++i)
    {
        delete m_pages[i].load(std::memory_order_relaxed);
    }
}

ProcessWorkStorage::ProcessWorkStorage(int32_t pid, uint64_t identity, uint32_t session,
    int32_t bytes, int32_t initial, bool receiver, const ProcessWorkMemoryConfig& config) :
m_pid(pid), m_identity(identity), m_session(session), m_bytes(bytes),
m_initial(initial), m_receiver(receiver), m_config(config)
{
    for (size_t i = 0; i < 256; ++i)
    {
        m_groups[i].store(nullptr, std::memory_order_relaxed);
    }
}

ProcessWorkStorage::~ProcessWorkStorage()
{
    for (size_t i = 0; i < 256; ++i)
    {
        delete m_groups[i].load(std::memory_order_relaxed);
    }
}

bool ProcessWorkStorage::prepare(uint32_t blocks, bool payloads)
{
    if (m_receiver || blocks == 0 || blocks > 0xffffff || blocks > static_cast<uint32_t>(m_initial))
    {
        return false;
    }
    const uint64_t pages = (blocks >> 8) + 1;
    const uint64_t body = payloads && static_cast<size_t>(m_bytes) > sizeof(static_cast<ProcessWorkSharedBlock*>(nullptr)->m_inlineData) ?
        static_cast<uint64_t>(m_bytes) + 65536 : 0;
    if (!ProcessWorkMemory::allows(m_config, pages * (sizeof(ProcessWorkSharedMetadata) + sizeof(ProcessWorkPage) + 65536) + body * blocks))
    {
        return false;
    }
    for (uint32_t offset = 0; offset < pages; ++offset)
    {
        if ((offset & 63U) == 0)
        {
            std::this_thread::yield();
            if (!ProcessWorkMemory::allows(m_config, sizeof(ProcessWorkSharedMetadata) + sizeof(ProcessWorkPage) + 65536))
            {
                return false;
            }
        }
        if (page(offset << 8) == nullptr)
        {
            return false;
        }
    }
    if (body != 0)
    {
        for (uint32_t index = 1; index <= blocks; ++index)
        {
            if ((index & 63U) == 0)
            {
                std::this_thread::yield();
                if (!ProcessWorkMemory::allows(m_config, body))
                {
                    return false;
                }
            }
            std::unique_ptr<SharedMemory> temporary;
            // 初始块不可退役；准备只打开映射，不读取或写入可能正在传输的正文。
            if (data(index, 1, temporary) == nullptr)
            {
                return false;
            }
        }
    }
    return true;
}

std::string ProcessWorkStorage::objectName(char kind, uint32_t pid, uint64_t identity,
    uint32_t session, uint32_t index, uint32_t generation)
{
    if (session > 0xffffff || index > 0xffffff || generation > 0xffffff)
    {
        return std::string();
    }
    unsigned char bytes[21] = {};
    for (int32_t i = 0; i < 4; ++i)
    {
        bytes[i] = static_cast<unsigned char>(pid >> (i * 8));
    }
    for (int32_t i = 0; i < 8; ++i)
    {
        bytes[4 + i] = static_cast<unsigned char>(identity >> (i * 8));
    }
    for (int32_t i = 0; i < 3; ++i)
    {
        bytes[12 + i] = static_cast<unsigned char>(session >> (i * 8));
        bytes[15 + i] = static_cast<unsigned char>(index >> (i * 8));
    }
    bytes[18] = static_cast<unsigned char>(generation);
    bytes[19] = static_cast<unsigned char>(generation >> 8);
    bytes[20] = static_cast<unsigned char>(generation >> 16);
    const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
#if defined(__APPLE__)
    std::string result(kind == 'R' || kind == 'W' ? "" : "Q");
#else
    std::string result("Q");
#endif
    result += kind;
    uint32_t pending = 0;
    int32_t bits = 0;
    for (int32_t i = 0; i < 21; ++i)
    {
        pending = (pending << 8) | bytes[i];
        bits += 8;
        while (bits >= 6)
        {
            bits -= 6;
            result += alphabet[(pending >> bits) & 63];
        }
    }
    if (bits != 0)
    {
        result += alphabet[(pending << (6 - bits)) & 63];
    }
    return result;
}

ProcessWorkPage* ProcessWorkStorage::page(uint32_t index, bool create)
{
    if (index > 0xffffff || (create && !m_receiver))
    {
        return nullptr;
    }
    const uint32_t pageIndex = index >> 8;
    const uint32_t groupIndex = pageIndex >> 8;
    try
    {
        ProcessWorkPageGroup* group = m_groups[groupIndex].load(std::memory_order_acquire);
        if (group == nullptr)
        {
            std::unique_ptr<ProcessWorkPageGroup> candidate(new ProcessWorkPageGroup);
            if (m_groups[groupIndex].compare_exchange_strong(group, candidate.get(), std::memory_order_acq_rel, std::memory_order_acquire))
            {
                group = candidate.release();
            }
        }
        ProcessWorkPage* mapped = group->m_pages[pageIndex & 255].load(std::memory_order_acquire);
        if (mapped != nullptr)
        {
            return mapped;
        }
        std::unique_ptr<ProcessWorkPage> candidate(new ProcessWorkPage);
        const std::string name = objectName('M', m_pid, m_identity, m_session, pageIndex);
        if (create)
        {
            if (!ProcessWorkMemory::allows(m_config, sizeof(ProcessWorkSharedMetadata) + 65536))
            {
                return nullptr;
            }
            candidate->m_memory.reset(new SharedMemory(name, sizeof(ProcessWorkSharedMetadata), false));
        }
        else
        {
            candidate->m_memory.reset(new SharedMemory(name));
        }
        SharedMemory& memory = *candidate->m_memory;
        ProcessWorkSharedMetadata* metadata = static_cast<ProcessWorkSharedMetadata*>(memory.writeWithoutLock());
        if (memory.isFailed() || metadata == nullptr || memory.realSize() < sizeof(ProcessWorkSharedMetadata))
        {
            return nullptr;
        }
        if (create)
        {
            std::memset(metadata, 0, sizeof(*metadata));
            metadata->m_page = pageIndex;
            for (size_t i = 0; i < 256; ++i)
            {
                metadata->m_blocks[i].m_owner = -1;
                metadata->m_nodes[i].m_owner = -1;
                metadata->m_streams[i].m_first.m_owner = -1;
            }
            // 登记下标先于元数据页存在；版本最后release发布，发送者不能使用半初始化页。
            ProcessWorkAtomic::replace(&metadata->m_version, 0, 5);
        }
        else if (ProcessWorkAtomic::load(&metadata->m_version) != 5 || metadata->m_page != static_cast<int32_t>(pageIndex))
        {
            return nullptr;
        }
        candidate->m_metadata = metadata;
        if (group->m_pages[pageIndex & 255].compare_exchange_strong(mapped, candidate.get(), std::memory_order_acq_rel, std::memory_order_acquire))
        {
            mapped = candidate.release();
        }
        return mapped;
    }
    catch (...)
    {
        return nullptr;
    }
}

bool ProcessWorkStorage::create(uint32_t index, uint64_t now)
{
    if (!m_receiver || index == 0 || m_bytes < 12 || !ProcessWorkMemory::allows(m_config, static_cast<uint64_t>(m_bytes) + 65536))
    {
        return false;
    }
    ProcessWorkPage* mapped = page(index, true);
    if (mapped == nullptr)
    {
        return false;
    }
    ProcessWorkSharedBlock& descriptor = mapped->m_metadata->m_blocks[index & 255];
    const int64_t oldGeneration = ProcessWorkAtomic::load(&descriptor.m_generation);
    if (ProcessWorkAtomic::load(&descriptor.m_owner) != -1 || oldGeneration >= 0xffffff)
    {
        return false;
    }
    if (static_cast<size_t>(m_bytes) <= sizeof(descriptor.m_inlineData))
    {
        mapped->m_idleSince[index & 255] = now;
        ProcessWorkAtomic::store(&descriptor.m_generation, oldGeneration + 1);
        ProcessWorkAtomic::store(&descriptor.m_owner, int64_t(0));
        return true;
    }
    try
    {
        std::unique_ptr<SharedMemory> memory(new SharedMemory(objectName('B', m_pid, m_identity,
            m_session, index, static_cast<uint32_t>(oldGeneration + 1)), m_bytes, false));
        if (memory->isFailed() || memory->writeWithoutLock() == nullptr || memory->realSize() < static_cast<uint32_t>(m_bytes))
        {
            return false;
        }
        mapped->m_addresses[index & 255].store(static_cast<char*>(memory->memory()), std::memory_order_release);
        mapped->m_data[index & 255].store(memory.release(), std::memory_order_release);
        mapped->m_idleSince[index & 255] = now;
        ProcessWorkAtomic::replace(&descriptor.m_generation, oldGeneration, oldGeneration + 1);
        ProcessWorkAtomic::replace(&descriptor.m_owner, int64_t(-1), int64_t(0));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

char* ProcessWorkStorage::data(uint32_t index, uint32_t generation, std::unique_ptr<SharedMemory>& temporary)
{
    ProcessWorkPage* mapped = cachedPage(index);
    if (mapped == nullptr)
    {
        mapped = page(index);
    }
    if (mapped == nullptr)
    {
        return nullptr;
    }
    return data(*mapped, index, generation, temporary);
}

char* ProcessWorkStorage::data(ProcessWorkPage& mapped, uint32_t index, uint32_t generation,
    std::unique_ptr<SharedMemory>& temporary)
{
    char* address = mapped.m_addresses[index & 255].load(std::memory_order_acquire);
    return address != nullptr ? address : mapData(mapped, index, generation, temporary);
}

char* ProcessWorkStorage::mapData(ProcessWorkPage& mapped, uint32_t index, uint32_t generation,
    std::unique_ptr<SharedMemory>& temporary)
{
    if (static_cast<size_t>(m_bytes) <= sizeof(mapped.m_metadata->m_blocks[0].m_inlineData))
    {
        if (index == 0 || ProcessWorkAtomic::load(&mapped.m_metadata->m_blocks[index & 255].m_generation) != generation)
        {
            return nullptr;
        }
        return mapped.m_metadata->m_blocks[index & 255].m_inlineData;
    }
    SharedMemory* memory = mapped.m_data[index & 255].load(std::memory_order_acquire);
    if (memory != nullptr)
    {
        char* address = static_cast<char*>(memory->memory());
        mapped.m_addresses[index & 255].store(address, std::memory_order_release);
        return address;
    }
    if (m_receiver)
    {
        return nullptr;
    }
    try
    {
        temporary.reset(new SharedMemory(objectName('B', m_pid, m_identity, m_session, index, generation)));
        if (temporary->writeWithoutLock() == nullptr || temporary->realSize() < static_cast<uint32_t>(m_bytes))
        {
            return nullptr;
        }
        if (index <= static_cast<uint32_t>(m_initial))
        {
            if (mapped.m_data[index & 255].compare_exchange_strong(memory, temporary.get(), std::memory_order_acq_rel, std::memory_order_acquire))
            {
                memory = temporary.release();
            }
            char* address = static_cast<char*>(memory->memory());
            mapped.m_addresses[index & 255].store(address, std::memory_order_release);
            return address;
        }
        return static_cast<char*>(temporary->memory());
    }
    catch (...)
    {
        return nullptr;
    }
}

bool ProcessWorkStorage::retire(uint32_t index)
{
    if (!m_receiver || index <= static_cast<uint32_t>(m_initial))
    {
        return false;
    }
    ProcessWorkPage* mapped = page(index);
    if (mapped == nullptr || !ProcessWorkAtomic::replace(&mapped->m_metadata->m_blocks[index & 255].m_owner, int64_t(0), int64_t(-1)))
    {
        return false;
    }
    mapped->m_addresses[index & 255].store(nullptr, std::memory_order_release);
    delete mapped->m_data[index & 255].exchange(nullptr, std::memory_order_acq_rel);
    return true;
}