#pragma once
#include <stdint.h>

// 文件后端操作，不把网络路径或远端声明当作本地文件权限
enum NetFileIoOperation
{
    // 以只读方式打开既有普通文件
    NET_FILE_IO_OPEN_READ = 1,
    // 独占新建临时文件，不覆盖既有目标
    NET_FILE_IO_OPEN_NEW = 2,
    // 打开既有可写普通文件
    NET_FILE_IO_OPEN_WRITE = 3,
    // 复制宿主原生只读句柄；不支持的宿主明确拒绝
    NET_FILE_IO_DUPLICATE_READ = 4,
    // 最多读取65536字节
    NET_FILE_IO_READ = 5,
    // 最多写入65536字节
    NET_FILE_IO_WRITE = 6,
    // 查询文件字节长度
    NET_FILE_IO_SIZE = 7,
    // 显式调整临时文件长度
    NET_FILE_IO_RESIZE = 8,
    // 完成持久化刷新
    NET_FILE_IO_FLUSH = 9,
    // 有界增量摘要，支持停止/取消
    NET_FILE_IO_HASH = 10,
    // 释放本队列文件号
    NET_FILE_IO_CLOSE = 11,
    // 原子发布，位0控制覆盖；file非零时后端关闭并提交该句柄，完成后文件号失效。file为0仅按宿主授权路径发布。
    NET_FILE_IO_PUBLISH = 12,
    // 独占创建宿主临时文件及父目录，取消/停止自动删除；PUBLISH携带file才能提交
    NET_FILE_IO_OPEN_TEMPORARY = 13
};

// 文件后端提交参数；成功接纳后不再借用调用方输入
struct NetFileIoRequest
{
    // 本对象生命周期内递增且非零，拒绝也消耗编号。
    uint64_t id;
    // 本后端分配的文件号，打开类操作填0
    uint64_t file;
    // NetFileIoOperation操作类型
    uint32_t operation;
    // WRITE位0表示持久化后才完成；PUBLISH位0允许替换既有目标。
    uint32_t flags;
    // 读写字节偏移，或HASH/RESIZE使用的边界值
    uint64_t offset;
    // 本次输入或输出请求字节数，读写不得超过65536
    uint32_t size;
    // WRITE输入借用到submit返回，成功接纳后后端持有副本
    const uint8_t* bytes;
    // 本地UTF8源路径，不能直接使用远端文件名；submit接纳时复制
    const char* path;
    // PUBLISH的本地UTF8目标路径，submit接纳时复制
    const char* destination;
    // DUPLICATE_READ借用的系统句柄，后端复制后独立关闭
    intptr_t descriptor;
};

// 文件后端完成结果；必须按id消费，字节由后端拥有
struct NetFileIoResult
{
    // 原请求编号，与提交时保持一致
    uint64_t id;
    // 对应文件号；打开成功时为新文件号，关闭类完成后不得复用
    uint64_t file;
    // 原NetFileIoOperation操作类型
    uint32_t operation;
    // NI_OK或明确失败码，不以size非零代替成功判断
    int32_t result;
    // 系统错误诊断码，不解释为业务或协议状态
    uint32_t detail;
    // 原请求的字节偏移
    uint64_t offset;
    // SIZE、HASH等操作报告的文件长度或处理长度，按操作含义解释
    uint64_t length;
    // 借用至consume/uninit；不可由外部释放。
    const uint8_t* bytes;
    // bytes中可借用的结果字节数
    uint32_t size;
};