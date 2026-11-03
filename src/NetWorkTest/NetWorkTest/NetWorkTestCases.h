#pragma once

// 协议独立预期集合；不复制产品的整数编解码实现
class NetWorkTestCases
{
public:
    /** 用手写字节向量核对线格式和全部字段，失败抛出测试异常
*/
    static void wireVector();

    /** 遍历全部切分位置和逐字节输入，失败抛出测试异常
*/
    static void fragmentation();

    /** 验证粘包消费长度、背压及借用视图不被覆盖
*/
    static void backpressure();

    /** 验证非法头拒绝、错误持续及新连接重置
*/
    static void invalidHeaders();

    /** 验证消息长度边界和无效配置在分配之前拒绝
*/
    static void sizeLimits();

    /** 验证空消息、空输入、截断及未完成时禁止消费
*/
    static void endOfInput();

    /** 验证编码缓冲哨兵和C++入口错误参数
*/
    static void arguments();

    /** 编码前拒绝正文重叠且保持输出，允许实际输出区外的正文和元数据别名
    */
    static void encodingAliases();

    /** 调用独立C编译单元，验证头文件与动态链接
*/
    static void cLanguage();

    /** 使用确定性输入验证多帧和各种碎片大小
*/
    static void deterministic();

    /** 流式摘要已知向量、容量探测及唯一完成，用于大文件框架基础验证
    */
    static void streamingHash();

    /** 文件发送/持久确认/完整摘要/实际提交后终态的基础闭环
    */
    static void fileFlowBasic();

    /** 新传输及恢复前缀完成后，快照保持TRANSFERRING和原stream；不得因进度增长退回PREPARING。
    */
    static void fileFlowSnapshot();

    /** 有界执行100000条消息，观察取消和期限
*/
    static void stress();
};

#include "NetWorkTest.h"

#include "NetInterfaceCppProbe.h"

#include "NetSenderProviderCProbe.h"