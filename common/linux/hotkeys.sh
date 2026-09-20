#!/bin/bash
# 用法：bash hotkeys.sh install | uninstall | status
# 移动脚本或修改 KEYS 后，重新执行 install 更新注册。
# 由 GNOME 常驻监听；脚本无需一直运行，也无需 sudo。
set -euo pipefail

KEYS=(D)
ACTIONS=(group_0)

group_0() {
    # 只结束当前用户名称精确匹配 ToDesk 的客户端，保留 ToDesk_Service。
    local result=0
    pkill -KILL -x -u "$(id -u)" -- ToDesk || result=$?
    if ((result != 0 && result != 1)); then
        return "$result"
    fi
}

SCHEMA=org.gnome.settings-daemon.plugins.media-keys
ITEM_SCHEMA="$SCHEMA.custom-keybinding"
# 固定标识用于更新自己的配置，不覆盖其他自定义快捷键。
PREFIX=/org/gnome/settings-daemon/plugins/media-keys/custom-keybindings/voyah-hotkeys-

fail() { printf '%s\n' "$*" >&2; exit 1; }

# 将字符串编码为 GVariant 字符串，避免路径中的引号被误解析。
variant_string() {
    local value=$1
    value=${value//\\/\\\\}
    value=${value//\'/\\\'}
    printf "'%s'" "$value"
}

load_paths() {
    local raw rest pattern="^'(/[A-Za-z0-9_/-]*/)'"
    raw=$(gsettings get "$SCHEMA" custom-keybindings)
    raw=${raw#@as }
    [[ $raw == \[*\] ]] || fail "无法解析现有快捷键列表，未修改配置。"
    rest=${raw:1:${#raw}-2}
    PATHS=()
    while [[ -n $rest ]]; do
        rest=${rest#"${rest%%[![:space:]]*}"}
        [[ -n $rest ]] || break
        [[ $rest =~ $pattern ]] || fail "现有快捷键路径格式不受支持，未修改配置。"
        PATHS+=("${BASH_REMATCH[1]}")
        rest=${rest:${#BASH_REMATCH[0]}}
        rest=${rest#"${rest%%[![:space:]]*}"}
        [[ -n $rest ]] || break
        [[ $rest == ,* ]] || fail "无法解析快捷键分隔符，未修改配置。"
        rest=${rest:1}
    done
}

save_paths() {
    local path value='[' separator=''
    for path in "${KEPT[@]}"; do
        value+="$separator$(variant_string "$path")"
        separator=', '
    done
    gsettings set "$SCHEMA" custom-keybindings "$value]"
}

mode=${1:-help}
case $mode in
    run)
        index=${2:-}
        # 只允许配置过的下标，不把传入参数直接当作命令执行。
        for i in "${!KEYS[@]}"; do
            if [[ $index == "$i" ]]; then
                "${ACTIONS[$i]}"
                exit 0
            fi
        done
        fail "无效的命令组下标：$index"
        ;;
    install|uninstall|status) ;;
    *)
        printf '用法：bash "%s" install | uninstall | status\n' "${BASH_SOURCE[0]}"
        exit 0
        ;;
esac

command -v gsettings >/dev/null || fail "系统缺少 gsettings。"
[[ ${EUID} -ne 0 ]] || fail "请在当前桌面用户的终端执行，不要使用 sudo。"
load_paths
KEPT=()
OWNED=()
for path in "${PATHS[@]}"; do
    if [[ $path == "$PREFIX"* ]]; then
        OWNED+=("$path")
    else
        KEPT+=("$path")
    fi
done

if [[ $mode == status ]]; then
    if ((${#OWNED[@]} == 0)); then
        printf '尚未注册快捷键。\n'
    fi
    for path in "${OWNED[@]}"; do
        gsettings get "$ITEM_SCHEMA:$path" binding
        gsettings get "$ITEM_SCHEMA:$path" command
    done
    exit 0
fi

if [[ $mode == install ]]; then
    ((${#KEYS[@]} == ${#ACTIONS[@]})) || fail "KEYS 与 ACTIONS 数量必须相同。"
    seen=' '
    for i in "${!KEYS[@]}"; do
        key=${KEYS[$i]}
        [[ $key =~ ^[A-Z]$ ]] || fail "字母必须为 A-Z：$key"
        [[ $seen != *" $key "* ]] || fail "字母重复：$key"
        seen+="$key "
        declare -F "${ACTIONS[$i]}" >/dev/null || fail "命令函数不存在：${ACTIONS[$i]}"
    done
    self=$(readlink -f -- "${BASH_SOURCE[0]}")
    # GNOME 命令使用双引号包住路径，兼容空格及引号。
    escaped=${self//\\/\\\\}
    escaped=${escaped//\"/\\\"}
    escaped=${escaped//\$/\\\$}
    escaped=${escaped//\`/\\\`}
fi

# 先移除旧的自有注册，保留用户其他快捷键。
save_paths
for path in "${OWNED[@]}"; do
    gsettings reset-recursively "$ITEM_SCHEMA:$path"
done

if [[ $mode == uninstall ]]; then
    printf '已移除本脚本注册的快捷键。\n'
    exit 0
fi

for i in "${!KEYS[@]}"; do
    path="$PREFIX$i/"
    gsettings set "$ITEM_SCHEMA:$path" name "$(variant_string "Voyah Alt+${KEYS[$i]}")"
    gsettings set "$ITEM_SCHEMA:$path" command "$(variant_string "/bin/bash \"$escaped\" run $i")"
    gsettings set "$ITEM_SCHEMA:$path" binding "$(variant_string "<Alt>${KEYS[$i],,}")"
    KEPT+=("$path")
done
save_paths
printf '已注册快捷键，脚本路径：%s\n' "$self"
printf '  Alt+%s\n' "${KEYS[@]}"
