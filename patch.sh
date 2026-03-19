#!/bin/bash

# 自动获取当前最新的 commit hash
LAST_COMMIT=$(git rev-parse HEAD)

echo "最新 commit: $LAST_COMMIT"

# 导出 patch
git format-patch -1 $LAST_COMMIT

echo "导出完成！当前目录已生成 0001-xxx.patch"
