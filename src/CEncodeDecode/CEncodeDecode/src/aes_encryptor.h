#ifndef SRC_UTILS_AES_ENCRYPTOR_H
#define SRC_UTILS_AES_ENCRYPTOR_H

#include "AES.h"
#include <string>

// 兼容历史AES128-ECB文本格式；独占密钥，不提供认证或TLS能力
class AesEncryptor
{
public:
    /** 展开16字节密钥
    @param [in] key 有效16字节，仅构造时借用
    */
    AesEncryptor(unsigned char* key);

    /** 释放并擦除内部密钥
    */
    ~AesEncryptor();

    /** 按历史规则补零并转换成大写十六进制
    @param [in] strInfor 文本，解密时以首个零字节结束
    @return 密文；容量越界或分配失败返回空串
    */
    std::string EncryptString(const std::string& strInfor);

    /** 校验完整十六进制分组后解密
    @param [in] strMessage 长度为32的倍数，允许大小写十六进制
    @return 首个零字节前的文本；非法输入或分配失败返回空串
    */
    std::string DecryptString(const std::string& strMessage);

    /** 兼容历史文本模式的文件加密；读取失败不覆盖目标
    @param [in] inputFileName 有效路径，内容应为无零字节文本
    @param [in] outputFileName 有效输出路径，可与输入相同
    */
    void EncryptTxtFile(const char* inputFileName, const char* outputFileName);

    /** 兼容历史文本模式的文件解密
    @param [in] inputFileName 有效密文文件路径
    @param [in] outputFileName 有效输出路径，可与输入相同
    */
    void DecryptTxtFile(const char* inputFileName, const char* outputFileName);

private:
    /** 解码并明确区分失败和合法空明文，失败时禁止文件接口覆盖目标
    @param [in] strMessage 完整HEX密文，不得与output别名
    @param [out] output 成功时明文，失败清空
    @return 完整解码成功为true，包括合法空明文
    */
    bool decryptText(const std::string& strMessage, std::string& output);

    /** 读取完整文本并检查流状态
    @param [in] path 文件路径；可空，空时失败
    @param [out] data 成功时完整内容，失败清空
    @return 读取成功返回true
    */
    bool readText(const char* path, std::string& data);

    /** 转换一个十六进制字符
    @param [in] value 输入字符
    @return 0至15，非法字符返回-1
    */
    int Char2Int(char value);

    /** 禁止复制敏感上下文
    @param [in] other 不可复制的源
    */
    AesEncryptor(const AesEncryptor& other);

    /** 禁止赋值敏感上下文
    @param [in] other 不可复制的源
    @return 不可调用
    */
    AesEncryptor& operator=(const AesEncryptor& other);

private:
    // 每实例独占的分组加密器，不在堆上重复分配
    AES m_encryptor;
};

#endif