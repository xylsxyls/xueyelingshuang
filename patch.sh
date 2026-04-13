#!/bin/bash

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PATCH_FILE="$SCRIPT_DIR/current_commit.patch"

if [ -f "$PATCH_FILE" ]; then
    # 导入patch模式：检测到patch文件存在，自动导入
    
    echo "检测到patch文件: $PATCH_FILE"
    
    # 获取当前commit（起点commit）
    START_COMMIT=$(git rev-parse HEAD)
    echo "起点commit: $START_COMMIT"
    
    # 清理可能存在的未完成操作
    if [ -d ".git/rebase-apply" ]; then
        git am --abort 2>/dev/null
    fi
    
    # 1. 将patch应用到工作区（不改变暂存区）
    echo "正在应用patch到工作区..."
    git apply --whitespace=nowarn "$PATCH_FILE"
    if [ $? -ne 0 ]; then
        echo "Patch应用失败，请检查冲突"
        exit 1
    fi
    
    # 2. 从patch文件中提取所有被修改的文件路径
    PATCH_FILES=$(grep '^diff --git' "$PATCH_FILE" | \
                  sed 's/^diff --git a\/\(.*\) b\/\(.*\)$/\2/' | \
                  sort -u)
    
    if [ -z "$PATCH_FILES" ]; then
        echo "警告: 未能从patch中解析出文件列表"
    else
        echo "patch涉及的文件："
        echo "$PATCH_FILES"
        # 3. 只将这些文件添加到暂存区
        echo "$PATCH_FILES" | xargs git add --
    fi
    
    # 4. 执行git reset --soft回到起点commit
    git reset --soft $START_COMMIT
    
    # 5. 删除patch文件
    rm -f "$PATCH_FILE"
    
    echo "========================================="
    echo "操作完成！"
    echo "✓ patch中的修改已暂存（可提交）"
    echo "✓ 您原有的本地修改保持未暂存状态"
    echo "✓ patch文件已删除"
    echo "执行 git status 查看详情"
    echo "========================================="
    
else
    # 导出patch模式：没有检测到patch文件，导出当前commit
    
    LAST_COMMIT=$(git rev-parse HEAD)
    echo "最新 commit: $LAST_COMMIT"
    
    git format-patch -1 $LAST_COMMIT --stdout > "$PATCH_FILE"
    
    echo "========================================="
    echo "导出完成！"
    echo "Patch文件: $PATCH_FILE"
    echo "将此文件复制到其他机器的脚本同目录下即可自动导入"
    echo "========================================="
fi