#pragma once

// 从密码库迁出的兼容、边界和性能测试，所有测试实现仅属于NetWorkTest。
class NetCryptoLegacyTest
{
public:
    /** 逐项执行MD5、AES、畸形输入、SM4、RSA与并发兼容回归
    */
    static void regression();

    /** 比较旧AES算法与mbedTLS，记录RSA密钥生成和OAEP耗时
    */
    static void benchmark();

    /** 执行真实TLS分片、背压、篡改、截断和关闭回归
    */
    static void tls();

    /** 在独占临时路径执行文件兼容和分组边界回归
    */
    static void fileBoundary();

    /** 运行保留的独立用例，包括可选OpenSSL互通配置
    @param [in] id 1至11：散列、AES、畸形AES、SM4、RSA、性能、并发、TLS、SM2、互通、文件
    @return 0通过，1断言失败，2当前构建不支持该可选配置
    */
    static int check(int id);
};