#pragma once
#include <string>

/** 头像文件的有界校验、原子保存及读取；账号引用归AccountStore所有
*/
class CookAvatarHelper
{
public:
    /** 判断服务端图片键，拒绝路径输入
    @param [in] key 32位十六进制ID及png/jpg扩展名
    @return 合法返回true
    */
    static bool isKey(const std::string& key);

    /** 校验并保存裁剪后的图片，失败清理本次半成品
    @param [in] bytes PNG/JPEG原始数据
    @param [out] key 新生成的图片键
    @param [out] message 失败原因
    @return 文件保存成功返回true
    */
    static bool save(const std::string& bytes, std::string& key, std::string& message);

    /** 读取有界头像文件
    @param [in] key 图片键
    @param [out] bytes 图片数据
    @param [out] contentType 实际格式对应MIME
    @return 读取成功返回true
    */
    static bool read(const std::string& key, std::string& bytes, std::string& contentType);

    /** 删除本工程生成的头像文件，不接受外部路径
    @param [in] key 图片键
    */
    static void remove(const std::string& key);

private:
    /** 获取配置的头像根目录，基于EXE位置
    @return 目录绝对路径
    */
    static std::string root();
};