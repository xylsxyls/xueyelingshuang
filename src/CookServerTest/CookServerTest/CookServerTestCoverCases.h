#pragma once
#include "CookServerTestFeedCases.h"

/** 正方形封面真实上传和授权测试。 */
class CookServerTestCoverCases
{
public:
    /** 注册62号用例。@param [in] client HTTP客户端 @param [in] registerCase 执行记录器 */
    static void Run(CookServerTestHttpClient& client, const CookServerTestFeedCases::RegisterFunction& registerCase);
};