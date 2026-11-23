set -e
bit=${1:-32}
platform_link=dll
if [[ $bit == wasm32 || $bit == wasm64 ]]; then platform_link=lib; fi
"$CLOUD_REBUILD" ProtoMessageManager "$bit" lib release same
"$CLOUD_REBUILD" PlatformTest "$bit" "$platform_link" release same
# Wasm 发布链已将对应白盒头写入带回执的 include/wasm。
if [[ $bit == wasm32 || $bit == wasm64 ]]; then exit 0; fi
root=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
headers="$root/src/ProtoMessageManager/ProtoMessageManager/src"
cp -f -- "$headers/ProtoMessage.pb.h" "$headers/ProtoMessageProtobuf.h" \
    "${CLOUD_REBUILD_SDK_ROOT:-$root}/include/ProtoMessageManager/"