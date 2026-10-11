#pragma once
#include "CookServerTestFeedCases.h"
#include "RapidJson/RapidJsonAPI.h"

/** 头像上传和系统登录身份的真实HTTP回归，追加67至69号用例
*/
class CookServerTestIdentityCases
{
public:
    /** 注册并执行三个隔离HTTP用例。
    @param [in] client HTTP客户端
    @param [in] registerCase 结果记录入口
    */
    static void run(CookServerTestHttpClient& client, const CookServerTestFeedCases::RegisterFunction& registerCase);

private:
    /** 执行JSON业务请求并检查预期结果。
    @param [in] client 客户端
    @param [in] token 会话
    @param [in] path 相对路径
    @param [in] body 请求体，空表示GET
    @param [in] ok 预期业务结果
    @return 返回响应JSON
    */
    static RapidJsonDocument request(CookServerTestHttpClient& client, const std::string& token,
        const std::string& path, const std::string& body = "", bool ok = true);

    /** 创建独立普通账号。
    @param [in] client 客户端
    @param [in] suffix 用例标识
    @return 返回注册JSON
    */
    static RapidJsonDocument user(CookServerTestHttpClient& client, const std::string& suffix);

    /** 上传真实二进制头像。
    @param [in] client 客户端
    @param [in] token 会话
    @param [in] bytes 图片数据
    @param [in] ok 预期业务结果
    @param [in] query 额外查询参数
    @return 返回上传JSON
    */
    static RapidJsonDocument upload(CookServerTestHttpClient& client, const std::string& token,
        const std::string& bytes, bool ok = true, const std::string& query = "");

    /** 读取固定JPEG测试素材。
    @return 返回JPEG字节
    */
    static std::string jpeg();

    /** 读取固定方形或长方形PNG测试素材。
    @param [in] square 是否方形
    @return 返回PNG字节
    */
    static std::string png(bool square = true);

    /** 读取1025像素正方形PNG，验证实际尺寸上限
    @return 返回PNG字节
    */
    static std::string oversizedPng();

    /** 断言条件。
    @param [in] value 条件
    @param [in] message 失败说明
    */
    static void require(bool value, const char* message);
};