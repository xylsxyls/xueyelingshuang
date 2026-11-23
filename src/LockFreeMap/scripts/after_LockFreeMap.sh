#!/bin/bash
set -e
xueyelingshuang="${CLOUD_REBUILD_SDK_ROOT:-${xueyelingshuang:-${XUEYELINGSHUANG:?}}}"

# 旧跳表节点供兼容性测试使用，公开模板不引用这两组文件。
rm -f -- "$xueyelingshuang/include/LockFreeMap/MapNode.h"
rm -f -- "$xueyelingshuang/include/LockFreeMap/MapNode.inl"
rm -f -- "$xueyelingshuang/include/LockFreeMap/MapValueHolder.h"
rm -f -- "$xueyelingshuang/include/LockFreeMap/MapValueHolder.inl"
