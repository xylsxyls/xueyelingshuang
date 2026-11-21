#pragma once
#include <string>
#include <fstream>
#include <stdint.h>

typedef unsigned char byte;
typedef uint32_t uint32;

// 历史校验摘要上下文；同实例串行调用，摘要快照不影响后续追加
class MD5
{
public:
    /** 初始化空消息摘要
    */
    MD5();

    /** 初始化并输入消息
    @param [in] input length字节，零长度可空；无效空指针不追加
    @param [in] length 输入字节数
    */
    MD5(const void* input, size_t length);

    /** 初始化并输入字符串全部字节
    @param [in] str 可含NUL的字节串
    */
    MD5(const std::string& str);

    /** 初始化并读取文件到流结束
    @param [in,out] in 有效输入流，处理后按旧接口关闭
    */
    MD5(std::ifstream& in);

    /** 追加消息，允许在查询摘要后继续追加
    @param [in] input length字节，仅本次借用
    @param [in] length 字节数，零长度或空指针不追加
    */
    void update(const void* input, size_t length);

    /** 追加字符串全部字节
    @param [in] str 可含NUL的字节串
    */
    void update(const std::string& str);

    /** 读取文件并关闭流，流错误时停止，不重复空转
    @param [in,out] in 已打开的流，沿用流自身的异常策略
    */
    void update(std::ifstream& in);

    /** 获取当前消息的摘要快照，不消耗增量状态
    @return 借用16字节，下一次digest或reset后内容可能改变
    */
    const byte* digest();

    /** 获取当前摘要的十六进制快照
    @return 32字节小写HEX，内存异常由公开CEncodeDecode入口捕获
    */
    std::string toString();

    /** 清空输入、缓存摘要并恢复初始状态
    */
    void reset();

private:
    /** 向分组缓存追加字节
    @param [in] input length字节有效内存，零长度可空
    @param [in] length 以size_t完整计数
    */
    void update(const byte* input, size_t length);

    /** 对状态副本进行填充，保存摘要后恢复增量状态
    */
    void final();

    /** 执行MD5单块压缩
    @param [in] block 恰好64字节
    */
    void transform(const byte block[64]);

    /** 将32位字编码为小端字节
    @param [in] input 至少length/4个字
    @param [out] output length字节
    @param [in] length 为4的整数倍
    */
    void encode(const uint32* input, byte* output, size_t length);

    /** 将小端字节解码为32位字
    @param [in] input length字节
    @param [out] output 至少length/4个字
    @param [in] length 为4的整数倍
    */
    void decode(const byte* input, uint32* output, size_t length);

    /** 转换摘要字节为小写HEX
    @param [in] input length字节
    @param [in] length 字节数
    @return 两倍长度字符串，可能发生分配异常
    */
    std::string bytesToHexString(const byte* input, size_t length);

    /** 禁止复制增量上下文
    @param [in] other 不可复制实例
    */
    MD5(const MD5& other);

    /** 禁止赋值增量上下文
    @param [in] other 不可复制实例
    @return 无实现
    */
    MD5& operator=(const MD5& other);

private:
    // 四个32位链值
    uint32 _state[4];
    // 输入位数模2^64，低字在前
    uint32 _count[2];
    // 尚未压缩的分组字节
    byte _buffer[64];
    // 最近一次查询的16字节快照
    byte _digest[16];
    // 当前输入的摘要快照已经计算
    bool _finished;
    // 终结填充首字节0x80，其余为零
    static const byte PADDING[64];
    // 小写十六进制字符表
    static const char HEX[16];
    // 文件读取块大小，单位字节
    static const size_t BUFFER_SIZE = 1024;
};