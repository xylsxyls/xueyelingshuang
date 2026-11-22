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