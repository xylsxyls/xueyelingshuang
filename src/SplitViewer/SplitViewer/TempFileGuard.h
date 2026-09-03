#pragma once

#include "SplitViewerCommon.h"

/** 临时文件RAII守卫，负责在对象销毁时删除读取sv配置时生成的临时配置文件
*/
struct SplitViewerTempFileGuard
{
public:
    // 需要自动删除的临时文件路径
    std::wstring path;

public:
    /** 构造临时文件守卫，不主动创建文件
    */
    SplitViewerTempFileGuard();

    /** 析构时删除path指向的临时文件
    */
    ~SplitViewerTempFileGuard();

private:
    /** 禁止拷贝构造，避免多个守卫重复删除同一个临时文件
    @param [in] other 另一个临时文件守卫
    */
    SplitViewerTempFileGuard(const SplitViewerTempFileGuard& other);

    /** 禁止赋值，避免临时文件删除所有权被复制
    @param [in] other 另一个临时文件守卫
    @return 返回当前对象引用
    */
    SplitViewerTempFileGuard& operator=(const SplitViewerTempFileGuard& other);
};