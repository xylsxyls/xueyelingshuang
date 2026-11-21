#pragma once

// 历史AES-128-ECB内部实现；每实例拥有密钥，调用者须串行使用，不提供认证
class AES
{
public:
    /** 构造历史AES-128分组变换器
    @param [in] key 16字节密钥，仅构造时借用；空时实例拒绝运算
    */
    AES(unsigned char* key);

    /** 擦除展开密钥
    */
    virtual ~AES();

    /** 原地加密一个完整分组
    @param [in,out] input 可写16字节，空时拒绝
    @return 原输入指针，参数或密钥非法时为空
    */
    unsigned char* Cipher(unsigned char* input);

    /** 原地解密一个完整分组
    @param [in,out] input 可写16字节，空时拒绝
    @return 原输入指针，参数或密钥非法时为空
    */
    unsigned char* InvCipher(unsigned char* input);

    /** 原地加密多个完整分组，不添加填充
    @param [in,out] input length字节，仅本次借用
    @param [in] length 非负且为16的整数倍；零长度不读写
    @return 成功返回input，非法时为空且不修改缓冲
    */
    void* Cipher(void* input, int length);

    /** 原地解密多个完整分组，不验证密钥或完整性
    @param [in,out] input length字节，仅本次借用
    @param [in] length 非负且为16的整数倍；零长度不读写
    @return 成功返回input，非法时为空且不修改缓冲
    */
    void* InvCipher(void* input, int length);

private:
    /** 按AES-128规则展开轮密钥
    @param [in] key 16字节有效密钥
    @param [out] w 11组4乘4轮密钥
    */
    void KeyExpansion(unsigned char* key, unsigned char w[][4][4]);

    /** 执行GF(2^8)乘法
    @param [in] a 第一个域元素
    @param [in] b 第二个域元素
    @return 乘积域元素
    */
    unsigned char FFmul(unsigned char a, unsigned char b);

    /** 对当前分组执行字节替换
    @param [in,out] state 4乘4字节状态
    */
    void SubBytes(unsigned char state[][4]);

    /** 对当前分组执行行移位
    @param [in,out] state 4乘4字节状态
    */
    void ShiftRows(unsigned char state[][4]);

    /** 对当前分组执行列混合
    @param [in,out] state 4乘4字节状态
    */
    void MixColumns(unsigned char state[][4]);

    /** 对当前分组执行逆字节替换
    @param [in,out] state 4乘4字节状态
    */
    void InvSubBytes(unsigned char state[][4]);

    /** 对当前分组执行逆行移位
    @param [in,out] state 4乘4字节状态
    */
    void InvShiftRows(unsigned char state[][4]);

    /** 对当前分组执行逆列混合
    @param [in,out] state 4乘4字节状态
    */
    void InvMixColumns(unsigned char state[][4]);

    /** 合并当前轮密钥
    @param [in,out] state 4乘4字节状态
    @param [in] k 当前4乘4轮密钥
    */
    void AddRoundKey(unsigned char state[][4], unsigned char k[][4]);

private:
    // 构造密钥有效才允许变换
    bool m_valid;
    // 正向固定替换表
    unsigned char Sbox[256];
    // 逆向固定替换表
    unsigned char InvSbox[256];
    // 当前实例独占的11轮密钥，析构时擦除
    unsigned char w[11][4][4];
};