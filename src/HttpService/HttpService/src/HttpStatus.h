#pragma once
#include <stdint.h>
#include <map>
#include <string>

/** HTTP状态枚举，避免业务代码直接散落裸数字
*/
enum HttpStatus
{
    // 请求成功，响应体通常有内容
    kHttpStatusOk = 200,
    // 请求成功并创建了新资源
    kHttpStatusCreated = 201,
    // 请求已接受，但可能尚未处理完成
    kHttpStatusAccepted = 202,
    // 请求成功，响应体为空
    kHttpStatusNoContent = 204,
    // 客户端请求格式或参数错误
    kHttpStatusBadRequest = 400,
    // 请求缺少有效身份认证
    kHttpStatusUnauthorized = 401,
    // 服务器理解请求，但拒绝执行
    kHttpStatusForbidden = 403,
    // 请求的资源不存在
    kHttpStatusNotFound = 404,
    // 当前接口不支持该HTTP方法
    kHttpStatusMethodNotAllowed = 405,
    // 请求和当前资源状态冲突
    kHttpStatusConflict = 409,
    // 请求体过大
    kHttpStatusPayloadTooLarge = 413,
    // 请求媒体类型不支持
    kHttpStatusUnsupportedMediaType = 415,
    // 请求过于频繁
    kHttpStatusTooManyRequests = 429,
    // 服务端内部错误
    kHttpStatusInternalServerError = 500,
    // 网关或上游服务返回错误
    kHttpStatusBadGateway = 502,
    // 服务暂时不可用
    kHttpStatusServiceUnavailable = 503
};

// HTTP状态到默认状态文本的映射，HttpResponse构造和setStatus时统一从这里取值。
static const std::map<HttpStatus, std::string> kHttpStatusTextMap =
{
    {kHttpStatusOk, "OK"},
    {kHttpStatusCreated, "Created"},
    {kHttpStatusAccepted, "Accepted"},
    {kHttpStatusNoContent, "No Content"},
    {kHttpStatusBadRequest, "Bad Request"},
    {kHttpStatusUnauthorized, "Unauthorized"},
    {kHttpStatusForbidden, "Forbidden"},
    {kHttpStatusNotFound, "Not Found"},
    {kHttpStatusMethodNotAllowed, "Method Not Allowed"},
    {kHttpStatusConflict, "Conflict"},
    {kHttpStatusPayloadTooLarge, "Payload Too Large"},
    {kHttpStatusUnsupportedMediaType, "Unsupported Media Type"},
    {kHttpStatusTooManyRequests, "Too Many Requests"},
    {kHttpStatusInternalServerError, "Internal Server Error"},
    {kHttpStatusBadGateway, "Bad Gateway"},
    {kHttpStatusServiceUnavailable, "Service Unavailable"}
};