#pragma once
#include <map>
#include <string>

/** 表情资源加载结果，基础库仅返回结果，不写业务日志
*/
enum ExpressionLoadResult
{
    ExpressionLoadSuccess = 0,
    ExpressionLoadInvalidOutput = 1,
    ExpressionLoadParseFailed = 2,
    ExpressionLoadMissingRoot = 3,
    ExpressionLoadEmptyConfig = 4,
    ExpressionLoadEmptyGroups = 5
};

// 静态中文原因；XML路径、行列等动态诊断通过接口详情参数返回
const std::map<ExpressionLoadResult, std::string> kExpressionLoadResultDescriptions =
{
    { ExpressionLoadSuccess, "成功" },
    { ExpressionLoadInvalidOutput, "表情配置输出参数为空" },
    { ExpressionLoadParseFailed, "表情配置文件读取或解析失败" },
    { ExpressionLoadMissingRoot, "表情配置缺少根节点" },
    { ExpressionLoadEmptyConfig, "表情配置内容为空" },
    { ExpressionLoadEmptyGroups, "表情分组列表为空" }
};