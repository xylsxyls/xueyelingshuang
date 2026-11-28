set -e

"$CLOUD_REBUILD" NetPeer 64 lib release same
"$CLOUD_REBUILD" PlatformTest 64 lib release same

# 仅为本版本白盒测试准备内部布局；保留 SDK 选定的公开头和链接宏。
source_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
sdk_root="${CLOUD_REBUILD_SDK_ROOT:-$source_root}"
sdk_root="${sdk_root//\\//}"
[[ -f "$sdk_root/include/NetPeer/NetPeerMacro.h" ]]
cp -p -- "$source_root/src/NetPeer/NetPeer/src/NetPeerClientState.h" "$sdk_root/include/NetPeer/NetPeerClientState.h"