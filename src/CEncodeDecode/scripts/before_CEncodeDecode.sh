librariesrely=()

:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" DllRelyTest "$DllRelyTest_bit" "$DllRelyTest_dlllib" "$DllRelyTest_debugRelease" "$DllRelyTest_allSame"
fi
librariesrely+=($'project\tDllRelyTest\t'"$DllRelyTest_dlllib")
!EOF!

root=$(cd "$(dirname "$0")/../../.." && pwd)
if [[ $1 == wasm32 || $1 == wasm64 ]]; then
    abi=x86
    [[ $1 != wasm64 ]] || abi=x64
    THIRDPARTY_FILES=libmbedtls.a bash "$root/tools/thirdparty.sh" mbedtls "$1" lib release "$4" || exit 1
    librariesrely+=($'vendor\t'"tools/mbedtls/lib/wasm/static/$abi/libmbedtls.a")
else
    case "${THIRDPARTY_PLATFORM:-${CMAKE_SYSTEM_NAME:-}}:${CMAKE_TOOLCHAIN_FILE:-}:$OSTYPE" in
        :*android.toolchain.cmake:*) archive=libmbedtls.a; config=release ;;
        windows:*|Windows:*|:*:msys*|:*:cygwin*|:*:win32*)
            archive=mbedtls.lib
            [[ $3 != debug ]] || archive=mbedtlsd.lib
            config=$3 ;;
        *) archive=libmbedtls.a; config=release ;;
    esac
    THIRDPARTY_FILES="$archive" bash "$root/tools/thirdparty.sh" mbedtls "$1" lib "$config" "$4" || exit 1
    librariesrely+=($'link\t'"$archive")
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return