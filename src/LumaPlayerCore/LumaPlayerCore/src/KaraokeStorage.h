#pragma once
#include <cstdio>
#include <mutex>
#include <stdint.h>
#include <stddef.h>

/** Append-only temporary PCM storage; shared snapshots keep the file alive until their last read
*/
class KaraokeStorage
{
public:
    /** Initialize an empty, automatically deleted temporary file
    */
    KaraokeStorage();

    /** Close and delete the private temporary file
    */
    ~KaraokeStorage();

    /** Report whether construction opened the private temporary file
    @return True if usable; false exposes the initialization error through error()
    */
    bool valid() const;

    /** Return the last failure diagnostic without allocating
    @return Static text, empty before any failure; successes do not erase past diagnostics
    */
    const char* error();

    /** Append bytes atomically with respect to readers
    @param [in] data Source bytes, nullptr writes silence
    @param [in] bytes Byte count
    @return Starting byte offset, UINT64_MAX on failure without publishing partial data
    */
    uint64_t append(const void* data, size_t bytes);

    /** Read an existing immutable byte range
    @param [in] offset Byte offset
    @param [out] data Destination
    @param [in] bytes Byte count
    @return True on success, false with error() on invalid range or IO failure
    */
    bool read(uint64_t offset, void* data, size_t bytes);

    /** Replace a committed scratch range; callers must not mutate published snapshots
    @param [in] offset Byte offset
    @param [in] data Replacement bytes
    @param [in] bytes Byte count, entirely inside existing storage
    @return True on success, false with error() on invalid range or IO failure
    */
    bool write(uint64_t offset, const void* data, size_t bytes);

    /** Return committed byte length
    @return 已提交存储的字节数
    */
    uint64_t size();

private:
    /** 初始化KaraokeStorage对象的状态，不转移输入借用对象的所有权
    @param [in] other 禁止拷贝的源对象，不取得所有权
    */
    KaraokeStorage(const KaraokeStorage& other);

    /** 禁止对象赋值，避免复制资源所有权
    @param [in] other 禁止拷贝的源对象，不取得所有权
    @return 禁止赋值，该函数没有实现
    */
    KaraokeStorage& operator=(const KaraokeStorage& other);

    /** Position the file using a checked 64-bit offset
    @param [in] offset Byte offset, called with m_mutex held
    @return True on success, false with a diagnostic on failure
    */
    bool seek(uint64_t offset);

    /** Retain a static diagnostic, called during construction or with m_mutex held
    @param [in] message Static error text
    @return Always false for error-return branches
    */
    bool fail(const char* message);

private:
    // 拥有的临时文件，析构时关闭
    std::FILE* m_file;
    // 对象独占拥有的同步锁，const快照读取仍按原锁范围保护
    std::mutex m_mutex;
    // 已提交的临时文件字节数
    uint64_t m_size;
    // 借用的静态错误文本，文件操作与读取按m_mutex串行化
    const char* m_error;
};