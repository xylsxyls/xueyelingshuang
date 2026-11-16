#pragma once

// 密码分层的独立预期与边界测试；实现和白盒访问均只存在于测试工程。
class NetCryptoTestCases
{
public:
    /** SHA256固定向量、分块、容量、完成、溢出与独立生命周期
    */
    static void hashBoundary();

    /** HMAC/PBKDF2固定向量、非法参数、别名和安全擦除
    */
    static void derivationBoundary();

    /** 多片段摘要固定向量、任意切分、别名、总量与地址溢出边界
    */
    static void partsBoundary();
};