#include "CEncodeDecodeHashCases.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include "CEncodeDecode/CEncodeDecodeHash.h"
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>

/** 检查独立预期并交给统一runner收集错误
@param [in] condition 预期条件
@param [in] reason 失败步骤
*/
static void CryptoCheck(bool condition, const char* reason)
{
    if (!condition)
    {
        throw std::runtime_error(reason);
    }
}

/** 与固定小写HEX向量比较，不调用被测编码函数
@param [in] actual 实际32字节结果
@param [in] expected 恰好64字节的独立向量
@return 所有字节相同
*/
static bool CryptoDigestEquals(const uint8_t* actual, const char* expected)
{
    const char digits[] = "0123456789abcdef";
    if (std::strlen(expected) != 64)
    {
        return false;
    }
    for (unsigned index = 0; index < 32; ++index)
    {
        if (digits[actual[index] >> 4] != expected[index * 2] || digits[actual[index] & 15] != expected[index * 2 + 1])
        {
            return false;
        }
    }
    return true;
}

void CEncodeDecodeHashCases::hashBoundary()
{
    const char* empty = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    const char* abc = "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    const char* million = "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0";
    uint8_t output[33];
    std::memset(output, 0x5a, sizeof(output));
    CryptoCheck(CEncodeDecode::sha256(nullptr, 0, output, sizeof(output)) == CENCODE_OK &&
        CryptoDigestEquals(output, empty) && output[32] == 0x5a, "empty SHA256 vector or tail");
    const uint8_t* input = reinterpret_cast<const uint8_t*>("abc");
    CryptoCheck(CEncodeDecode::sha256(input, 3, output, sizeof(output)) == CENCODE_OK &&
        CryptoDigestEquals(output, abc), "abc SHA256 vector");
    uint8_t saved[33];
    std::memcpy(saved, output, sizeof(saved));
    CryptoCheck(CEncodeDecode::sha256(input, 3, output, 31) == CENCODE_BUFFER &&
        CEncodeDecode::sha256(nullptr, 1, output, 33) == CENCODE_ARGUMENT &&
        CEncodeDecode::sha256(input, 16777217, output, 33) == CENCODE_ARGUMENT &&
        CEncodeDecode::sha256(input, 3, nullptr, 32) == CENCODE_ARGUMENT &&
        std::memcmp(saved, output, sizeof(saved)) == 0, "hash invalid arguments modified output");
    std::memcpy(output, "abc", 3);
    CryptoCheck(CEncodeDecode::sha256(output, 3, output, 32) == CENCODE_OK &&
        CryptoDigestEquals(output, abc), "hash alias input/output");

    typedef std::unique_ptr<CEncodeDecodeHash, void (*)(CEncodeDecodeHash*)> HashOwner;
    for (uint32_t stride = 63; stride <= 65; ++stride)
    {
        CEncodeDecodeHash* raw = nullptr;
        CryptoCheck(CEncodeDecode::hashCreate(&raw) == CENCODE_OK && raw != nullptr, "hash create");
        HashOwner hash(raw, &CEncodeDecode::hashDestroy);
        const std::string chunk(stride, 'a');
        for (uint32_t done = 0; done < 1000000;)
        {
            const uint32_t count = 1000000 - done < stride ? 1000000 - done : stride;
            CryptoCheck(CEncodeDecode::hashUpdate(hash.get(), reinterpret_cast<const uint8_t*>(chunk.data()), count) == CENCODE_OK,
                "hash streaming append");
            done += count;
        }
        std::memset(output, 0x5a, sizeof(output));
        CryptoCheck(CEncodeDecode::hashUpdate(hash.get(), nullptr, 1) == CENCODE_ARGUMENT &&
            CEncodeDecode::hashUpdate(hash.get(), input, 16777217) == CENCODE_ARGUMENT &&
            CEncodeDecode::hashUpdate(hash.get(), nullptr, 0) == CENCODE_OK, "hash invalid update poisoned state");
        CryptoCheck(CEncodeDecode::hashFinish(hash.get(), output, 31) == CENCODE_BUFFER &&
            output[0] == 0x5a && output[32] == 0x5a, "hash short finish committed");
        CryptoCheck(CEncodeDecode::hashFinish(hash.get(), output, sizeof(output)) == CENCODE_OK &&
            CryptoDigestEquals(output, million) && output[32] == 0x5a, "streamed million-a vector");
        CryptoCheck(CEncodeDecode::hashFinish(hash.get(), output, 32) == CENCODE_STATE &&
            CEncodeDecode::hashUpdate(hash.get(), nullptr, 0) == CENCODE_STATE, "finished hash reused");
    }

    CEncodeDecodeHash* firstRaw = nullptr;
    CryptoCheck(CEncodeDecode::hashCreate(&firstRaw) == CENCODE_OK, "first independent hash");
    HashOwner first(firstRaw, &CEncodeDecode::hashDestroy);
    CEncodeDecodeHash* secondRaw = nullptr;
    CryptoCheck(CEncodeDecode::hashCreate(&secondRaw) == CENCODE_OK, "second independent hash");
    HashOwner second(secondRaw, &CEncodeDecode::hashDestroy);
    first->m_size = UINT64_C(0x1fffffffffffffff) - 1;
    CryptoCheck(CEncodeDecode::hashUpdate(first.get(), input, 2) == CENCODE_LIMIT &&
        CEncodeDecode::hashUpdate(first.get(), input, 1) == CENCODE_LIMIT &&
        CEncodeDecode::hashFinish(first.get(), output, 32) == CENCODE_LIMIT, "hash counter overflow or sticky error");
    CryptoCheck(CEncodeDecode::hashFinish(second.get(), output, 32) == CENCODE_OK &&
        CryptoDigestEquals(output, empty), "independent hash state leaked");
    CryptoCheck(CEncodeDecode::hashCreate(nullptr) == CENCODE_ARGUMENT &&
        CEncodeDecode::hashUpdate(nullptr, nullptr, 0) == CENCODE_ARGUMENT &&
        CEncodeDecode::hashFinish(nullptr, output, 32) == CENCODE_ARGUMENT, "null hash state");
    CEncodeDecode::hashDestroy(nullptr);
}

void CEncodeDecodeHashCases::derivationBoundary()
{
    uint8_t key[131];
    uint8_t output[33];
    std::memset(key, 0x0b, sizeof(key));
    std::memset(output, 0x5a, sizeof(output));
    const uint8_t* hi = reinterpret_cast<const uint8_t*>("Hi There");
    CryptoCheck(CEncodeDecode::hmacSha256(key, 20, hi, 8, output, 33) == CENCODE_OK && output[32] == 0x5a &&
        CryptoDigestEquals(output, "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"), "RFC4231 HMAC case 1");
    std::memset(key, 0xaa, sizeof(key));
    const char* longText = "Test Using Larger Than Block-Size Key - Hash Key First";
    CryptoCheck(CEncodeDecode::hmacSha256(key, sizeof(key), reinterpret_cast<const uint8_t*>(longText),
        static_cast<uint32_t>(std::strlen(longText)), output, 32) == CENCODE_OK &&
        CryptoDigestEquals(output, "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54"), "RFC4231 long HMAC key");
    CryptoCheck(CEncodeDecode::hmacSha256(nullptr, 0, nullptr, 0, output, 32) == CENCODE_OK &&
        CryptoDigestEquals(output, "b613679a0814d9ec772f95d778c35fc5ff1697c493715653c6c712144292c5ad"), "empty HMAC vector");
    uint8_t saved[33];
    std::memcpy(saved, output, sizeof(saved));
    CryptoCheck(CEncodeDecode::hmacSha256(nullptr, 1, hi, 8, output, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::hmacSha256(key, 16385, hi, 8, output, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::hmacSha256(key, 20, nullptr, 1, output, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::hmacSha256(key, 20, hi, 16777217, output, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::hmacSha256(key, 20, hi, 8, output, 31) == CENCODE_BUFFER &&
        std::memcmp(saved, output, sizeof(saved)) == 0, "HMAC invalid parameter output changed");
    std::memset(key, 0x0b, 20);
    CryptoCheck(CEncodeDecode::hmacSha256(key, 20, hi, 8, key, 32) == CENCODE_OK &&
        CryptoDigestEquals(key, "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"), "HMAC key/output alias");

    const uint8_t* password = reinterpret_cast<const uint8_t*>("password");
    const uint8_t* salt = reinterpret_cast<const uint8_t*>("salt");
    const uint32_t rounds[] = {1, 2, 4096};
    const char* expected[] = {"120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b",
        "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43",
        "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a"};
    for (unsigned index = 0; index < 3; ++index)
    {
        CryptoCheck(CEncodeDecode::pbkdf2Sha256(password, 8, salt, 4, rounds[index], output, 33) == CENCODE_OK &&
            CryptoDigestEquals(output, expected[index]) && output[32] == 0x5a, "PBKDF2 SHA256 fixed vector");
    }
    std::memcpy(saved, output, sizeof(saved));
    CryptoCheck(CEncodeDecode::pbkdf2Sha256(nullptr, 1, salt, 4, 1, output, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::pbkdf2Sha256(password, 0, salt, 4, 1, output, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::pbkdf2Sha256(password, 1025, salt, 4, 1, output, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::pbkdf2Sha256(password, 8, nullptr, 4, 1, output, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::pbkdf2Sha256(password, 8, salt, 0, 1, output, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::pbkdf2Sha256(password, 8, salt, 129, 1, output, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::pbkdf2Sha256(password, 8, salt, 4, 0, output, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::pbkdf2Sha256(password, 8, salt, 4, 2000001, output, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::pbkdf2Sha256(password, 8, salt, 4, 1, output, 31) == CENCODE_BUFFER &&
        std::memcmp(saved, output, sizeof(saved)) == 0, "PBKDF2 invalid parameters modified output");
    std::memcpy(output, password, 8);
    CryptoCheck(CEncodeDecode::pbkdf2Sha256(output, 8, salt, 4, 1, output, 32) == CENCODE_OK &&
        CryptoDigestEquals(output, expected[0]), "PBKDF2 input/output alias");
    std::memset(output, 0x5a, sizeof(output));
    CEncodeDecode::wipe(output + 1, 31);
    for (unsigned index = 1; index < 32; ++index)
    {
        CryptoCheck(output[index] == 0, "wipe missed a byte");
    }
    CryptoCheck(output[0] == 0x5a && output[32] == 0x5a, "wipe crossed buffer boundary");
    // 每个短长度及非对齐起点都只擦除指定区间，两侧哨兵必须保持。
    for (uint32_t size = 0; size <= 32; ++size)
    {
        uint8_t guarded[35];
        std::memset(guarded, 0x5a, sizeof(guarded));
        CEncodeDecode::wipe(guarded + 1, size);
        for (uint32_t index = 0; index < sizeof(guarded); ++index)
        {
            CryptoCheck(guarded[index] == (index >= 1 && index <= size ? 0 : 0x5a), "short secure wipe range");
        }
    }
    CEncodeDecode::wipe(nullptr, 1);
    CEncodeDecode::wipe(output, 0);
}

void CEncodeDecodeHashCases::partsBoundary()
{
    uint8_t output[33];
    uint8_t key[33];
    uint8_t saved[33];
    const uint8_t* abc = reinterpret_cast<const uint8_t*>("abc");
    const uint8_t* hi = reinterpret_cast<const uint8_t*>("Hi There");
    CEncodeDecodeBytes parts[17] = {};
    for (uint32_t first = 0; first <= 3; ++first)
    {
        for (uint32_t second = first; second <= 3; ++second)
        {
            parts[0].m_bytes = abc;
            parts[0].m_size = first;
            parts[1].m_bytes = abc + first;
            parts[1].m_size = second - first;
            parts[2].m_bytes = abc + second;
            parts[2].m_size = 3 - second;
            std::memset(output, 0x5a, sizeof(output));
            CryptoCheck(CEncodeDecode::sha256Parts(parts, 16, output, 33) == CENCODE_OK && output[32] == 0x5a &&
                CryptoDigestEquals(output, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"),
                "parts SHA256 split or zero fragments");
        }
    }
    for (uint32_t split = 0; split <= 8; ++split)
    {
        std::memset(parts, 0, sizeof(parts));
        parts[0].m_bytes = hi;
        parts[0].m_size = split;
        parts[1].m_bytes = hi + split;
        parts[1].m_size = 8 - split;
        std::memset(key, 0x0b, sizeof(key));
        CryptoCheck(CEncodeDecode::hmacSha256Parts(key, 20, parts, 16, output, 33) == CENCODE_OK && output[32] == 0x5a &&
            CryptoDigestEquals(output, "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"),
            "parts RFC4231 split vector");
    }
    CryptoCheck(CEncodeDecode::sha256Parts(nullptr, 0, output, 33) == CENCODE_OK &&
        CryptoDigestEquals(output, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"), "parts empty SHA256");
    CryptoCheck(CEncodeDecode::hmacSha256Parts(nullptr, 0, nullptr, 0, output, 33) == CENCODE_OK &&
        CryptoDigestEquals(output, "b613679a0814d9ec772f95d778c35fc5ff1697c493715653c6c712144292c5ad"), "parts empty HMAC");
    std::memcpy(saved, output, sizeof(saved));
    const CEncodeDecodeBytes* overflow = reinterpret_cast<const CEncodeDecodeBytes*>(UINTPTR_MAX - 1);
    CryptoCheck(CEncodeDecode::sha256Parts(parts, 17, output, 33) == CENCODE_ARGUMENT &&
        CEncodeDecode::sha256Parts(nullptr, 1, output, 33) == CENCODE_ARGUMENT &&
        CEncodeDecode::sha256Parts(overflow, 1, output, 33) == CENCODE_ARGUMENT &&
        CEncodeDecode::sha256Parts(parts, UINT32_MAX, output, 33) == CENCODE_ARGUMENT &&
        CEncodeDecode::hmacSha256Parts(key, 20, parts, 17, output, 33) == CENCODE_ARGUMENT &&
        CEncodeDecode::hmacSha256Parts(key, 20, overflow, 1, output, 33) == CENCODE_ARGUMENT,
        "parts count/address overflow");
    parts[0].m_bytes = nullptr;
    parts[0].m_size = 1;
    CryptoCheck(CEncodeDecode::sha256Parts(parts, 2, output, 33) == CENCODE_ARGUMENT &&
        CEncodeDecode::hmacSha256Parts(key, 20, parts, 2, output, 33) == CENCODE_ARGUMENT, "parts null nonempty");
    parts[0].m_bytes = reinterpret_cast<const uint8_t*>(UINTPTR_MAX - 1);
    parts[0].m_size = 3;
    CryptoCheck(CEncodeDecode::sha256Parts(parts, 1, output, 33) == CENCODE_ARGUMENT &&
        CEncodeDecode::hmacSha256Parts(key, 20, parts, 1, output, 33) == CENCODE_ARGUMENT, "parts body overflow");
    parts[0].m_bytes = abc;
    parts[0].m_size = 16777216;
    parts[1].m_bytes = abc;
    parts[1].m_size = 1;
    CryptoCheck(CEncodeDecode::sha256Parts(parts, 2, output, 33) == CENCODE_ARGUMENT &&
        CEncodeDecode::hmacSha256Parts(key, 20, parts, 2, output, 33) == CENCODE_ARGUMENT, "parts total overflow");
    parts[0].m_size = 3;
    CryptoCheck(CEncodeDecode::sha256Parts(parts, 1, output, 31) == CENCODE_BUFFER &&
        CEncodeDecode::sha256Parts(parts, 1, nullptr, 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::sha256Parts(parts, 1, reinterpret_cast<uint8_t*>(UINTPTR_MAX - 15), 32) == CENCODE_ARGUMENT &&
        CEncodeDecode::hmacSha256Parts(key, 20, parts, 1, output, 31) == CENCODE_BUFFER &&
        CEncodeDecode::hmacSha256Parts(nullptr, 1, parts, 1, output, 33) == CENCODE_ARGUMENT &&
        CEncodeDecode::hmacSha256Parts(key, 16385, parts, 1, output, 33) == CENCODE_ARGUMENT &&
        CEncodeDecode::hmacSha256Parts(reinterpret_cast<const uint8_t*>(UINTPTR_MAX), 2, parts, 1, output, 33) == CENCODE_ARGUMENT &&
        std::memcmp(saved, output, sizeof(saved)) == 0, "parts invalid parameters changed result");
    std::memcpy(output, "abc", 3);
    parts[0].m_bytes = output;
    CryptoCheck(CEncodeDecode::sha256Parts(parts, 1, output, 33) == CENCODE_OK &&
        CryptoDigestEquals(output, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"), "parts SHA256 body alias");
    parts[0].m_bytes = hi;
    parts[0].m_size = 8;
    CryptoCheck(CEncodeDecode::hmacSha256Parts(key, 20, parts, 1, key, 33) == CENCODE_OK && key[32] == 0x0b &&
        CryptoDigestEquals(key, "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7"), "parts HMAC key alias");
    // 上限由真实完整输入覆盖，不能只测越界拒绝后宣称最大正文可用。
    const std::string maximum(16777216, 'a');
    parts[0].m_bytes = reinterpret_cast<const uint8_t*>(maximum.data());
    parts[0].m_size = 8388608;
    parts[1].m_bytes = parts[0].m_bytes + parts[0].m_size;
    parts[1].m_size = 8388608;
    CryptoCheck(CEncodeDecode::sha256(parts[0].m_bytes, 16777216, saved, 33) == CENCODE_OK &&
        CEncodeDecode::sha256Parts(parts, 2, output, 33) == CENCODE_OK && std::memcmp(saved, output, 32) == 0,
        "parts SHA256 maximum accepted length");
    CryptoCheck(CEncodeDecode::hmacSha256(key, 20, parts[0].m_bytes, 16777216, saved, 33) == CENCODE_OK &&
        CEncodeDecode::hmacSha256Parts(key, 20, parts, 2, output, 33) == CENCODE_OK && std::memcmp(saved, output, 32) == 0,
        "parts HMAC maximum accepted length");
}