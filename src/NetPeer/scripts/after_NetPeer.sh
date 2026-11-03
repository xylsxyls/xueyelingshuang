#!/bin/bash
# 普通头文件和消费Macro.h已发布；从发布目录选定统一NetSender入口。
script_root=$(cd -- "$(dirname -- "$0")/../../.." && pwd) || exit 1
sdk_root="${CLOUD_REBUILD_SDK_ROOT:-$script_root}"
sdk_root="${sdk_root//\\//}"
source_header="$sdk_root/include/NetPeer/NetSenderAPI.h"
sender_directory="$sdk_root/include/NetSender"

[[ -s "$source_header" && -f "$sdk_root/include/NetPeer/NetPeerMacro.h" ]] || exit 1
[[ ! -d "$sender_directory/NetSenderAPI.h" ]] || exit 1
mkdir -p -- "$sender_directory" || exit 1
staged_header=$(mktemp "$sender_directory/.NetSenderAPI.XXXXXX") || exit 1
trap 'rm -f -- "$staged_header"' EXIT
cp -p -- "$source_header" "$staged_header" && mv -f -- "$staged_header" "$sender_directory/NetSenderAPI.h"