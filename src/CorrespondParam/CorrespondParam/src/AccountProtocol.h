#pragma once
#include <stdint.h>
#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

// 业务结果与网关传输状态分离；任何错误都不能被解释为VIP授权。
// 短信挑战按用途隔离，登录/绑定验证码不能用于修改密码。
enum AccountSmsPurpose
{
    ACCOUNT_SMS_LOGIN = 1,
    ACCOUNT_SMS_BIND_PHONE = 2,
    ACCOUNT_SMS_RESET_PASSWORD = 3
};

enum AccountResult
{
    ACCOUNT_OK = 0,
    ACCOUNT_INVALID = 1,
    ACCOUNT_UNAUTHORIZED = 2,
    ACCOUNT_CONFLICT = 3,
    ACCOUNT_EXPIRED = 4,
    ACCOUNT_UNAVAILABLE = 5,
    ACCOUNT_BUSY = 6,
    ACCOUNT_WRONG_ANSWER = 7,
    ACCOUNT_ATTEMPTS_EXHAUSTED = 8,
    ACCOUNT_NOT_FOUND = 9,
    ACCOUNT_CHANNEL_DISABLED = 10,
    ACCOUNT_VIP_REQUIRED = 11,
    ACCOUNT_STORAGE_FAILED = 12,
    ACCOUNT_CLOCK_ERROR = 13,
    ACCOUNT_CANCELLED = 14
};

// ProtoMessage字段号使用独立显式区，微信Wasm与原生共用同一序列化。
enum AccountField
{
    ACCOUNT_FIELD_RESULT = 50000,
    ACCOUNT_FIELD_LOGIN = 50001,
    ACCOUNT_FIELD_PASSWORD = 50002,
    ACCOUNT_FIELD_TOKEN = 50003,
    ACCOUNT_FIELD_USER_ID = 50004,
    ACCOUNT_FIELD_PHONE = 50005,
    ACCOUNT_FIELD_SERVER_UTC = 50006,
    ACCOUNT_FIELD_SESSION_EXPIRES = 50007,
    ACCOUNT_FIELD_VIP_UNTIL = 50008,
    ACCOUNT_FIELD_VIP_LEVEL = 50009,
    ACCOUNT_FIELD_ORDER_ID = 50010,
    ACCOUNT_FIELD_QUESTION_ID = 50011,
    ACCOUNT_FIELD_QUESTION = 50012,
    ACCOUNT_FIELD_QUESTION_EXPIRES = 50013,
    ACCOUNT_FIELD_ANSWER = 50014,
    ACCOUNT_FIELD_ATTEMPTS_LEFT = 50015,
    ACCOUNT_FIELD_ORDER_STATE = 50016,
    ACCOUNT_FIELD_IDEMPOTENCY_KEY = 50017,
    ACCOUNT_FIELD_DURATION_SECONDS = 50018,
    ACCOUNT_FIELD_GENERATION = 50019,
    ACCOUNT_FIELD_CODE = 50020,
    ACCOUNT_FIELD_PROVIDER = 50021,
    ACCOUNT_FIELD_AUTHORIZATION = 50022,
    ACCOUNT_FIELD_SMS_ENABLED = 50023,
    ACCOUNT_FIELD_WECHAT_ENABLED = 50024,
    ACCOUNT_FIELD_QQ_ENABLED = 50025,
    ACCOUNT_FIELD_MATH_ENABLED = 50026,
    ACCOUNT_FIELD_FULFILLED_UNTIL = 50027,
    ACCOUNT_FIELD_PRODUCT = 50028,
    ACCOUNT_FIELD_CONNECTED = 50029,
    ACCOUNT_FIELD_CHALLENGE = 50030,
    ACCOUNT_FIELD_VERIFICATION = 50032, // 改密凭据，客户端库内部保存，不用于显示
    ACCOUNT_FIELD_NEW_PASSWORD = 50033,
    ACCOUNT_FIELD_PASSWORD_CHANGE_REQUIRED = 50034,
    ACCOUNT_FIELD_PHONE_BOUND = 50035,
    ACCOUNT_FIELD_PASSWORD_VERIFIED = 50036, // SDK持有凭据，是否过期仍由服务端判定
    ACCOUNT_FIELD_VERIFICATION_EXPIRES = 50037,
    ACCOUNT_FIELD_AUTHENTICATED = 50038,
    ACCOUNT_FIELD_PHONE_DISPLAY = 50039,
    ACCOUNT_FIELD_BIND_PHONE_REQUIRED = 50040,
    ACCOUNT_FIELD_PURPOSE = 50031
};

enum AccountOrderState
{
    ACCOUNT_ORDER_PENDING = 0,
    ACCOUNT_ORDER_FULFILLED = 1,
    ACCOUNT_ORDER_CANCELLED = 2,
    ACCOUNT_ORDER_EXPIRED = 3,
    ACCOUNT_ORDER_EXHAUSTED = 4
};

#define ACCOUNT_MATH_VIP_SECONDS 180

/** 账号协议结果对应的固定UTF8文案，不包含用户数据
@param [in] result AccountResult
@return 静态借用文本
*/
static __inline const char* accountProtocolResultText(int32_t result)
{
    switch (result)
    {
    case ACCOUNT_OK:
        return "成功";
    case ACCOUNT_INVALID:
        return "输入信息有误";
    case ACCOUNT_UNAUTHORIZED:
        return "账号或密码错误，或登录已失效，请重新登录";
    case ACCOUNT_CONFLICT:
        return "账号或绑定信息已存在";
    case ACCOUNT_EXPIRED:
        return "订单已过期，请重新创建";
    case ACCOUNT_UNAVAILABLE:
        return "账号服务暂时不可用";
    case ACCOUNT_BUSY:
        return "操作过于频繁，请稍后重试";
    case ACCOUNT_WRONG_ANSWER:
        return "答案不正确，请重试";
    case ACCOUNT_ATTEMPTS_EXHAUSTED:
        return "尝试次数已用完";
    case ACCOUNT_NOT_FOUND:
        return "未找到该订单";
    case ACCOUNT_CHANNEL_DISABLED:
        return "此登录或充值方式尚未开通";
    case ACCOUNT_VIP_REQUIRED:
        return "当前软件的VIP已到期或尚未开通";
    case ACCOUNT_STORAGE_FAILED:
        return "账号信息保存失败，请稍后重试";
    case ACCOUNT_CLOCK_ERROR:
        return "账号服务时间异常，请稍后重试";
    case ACCOUNT_CANCELLED:
        return "操作已取消";
    default:
        return "账号操作失败";
    }
}