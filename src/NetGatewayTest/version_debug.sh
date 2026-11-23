set -e
case "${1:-}" in
    32|64|arm32|arm64) gateway_bit=$1 ;;
    Win32) gateway_bit=32 ;;
    x64) gateway_bit=64 ;;
    '')
        case "$(uname -sm)" in
            "Linux aarch64"|"Linux arm64") gateway_bit=arm64 ;;
            "Linux arm"|"Linux armv"*) gateway_bit=arm32 ;;
            *) gateway_bit=32 ;;
        esac ;;
    *) echo "Expected 32, 64, arm32 or arm64" >&2; exit 2 ;;
esac

"$CLOUD_REBUILD" NetWork "$gateway_bit" dll debug same
"$CLOUD_REBUILD" LibuvTcp "$gateway_bit" dll debug same
"$CLOUD_REBUILD" AtomicMath "$gateway_bit" dll debug same
"$CLOUD_REBUILD" CDump "$gateway_bit" dll debug same
"$CLOUD_REBUILD" CRandom "$gateway_bit" dll debug same
"$CLOUD_REBUILD" NetSender "$gateway_bit" dll debug same

# 依赖发布完毕后，为本工程的白盒用例准备私有头。
xueyelingshuang="${xueyelingshuang:-${XUEYELINGSHUANG:?}}"
headers="$xueyelingshuang/src/ProcessWork/ProcessWork/src"
cp -f -- "$headers/ProcessWorkSharedQueue.h" "$headers/ProcessWorkSharedQueue.inl" \
    "$headers/ProcessWorkStorage.h" "$headers/ProcessWorkStorage.inl" \
    "${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}/include/ProcessWork/"
cp -f -- "$xueyelingshuang/src/ProtoMessageManager/ProtoMessageManager/src/ProtoMessage.pb.h" \
    "${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}/include/ProtoMessageManager/"