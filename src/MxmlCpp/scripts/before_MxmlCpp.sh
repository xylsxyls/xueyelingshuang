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

root=$(cd "$(dirname "$0")/../../.." && pwd) || exit 1
case "$1" in
    32|arm32|wasm32) bits=x86 ;;
    64|arm64|wasm64) bits=x64 ;;
    *) exit 2 ;;
esac
config=release
archive=libmxml.a
platform=${THIRDPARTY_PLATFORM:-${CMAKE_SYSTEM_NAME:-}}
if [[ -z $platform && ${CMAKE_TOOLCHAIN_FILE:-} == *android.toolchain.cmake ]]; then platform=android; fi
if [[ $platform == [Aa]ndroid && $1 != wasm* ]]; then
    abi=${CMAKE_ANDROID_ARCH_ABI:-${ANDROID_ABI:-}}
    case "$1:$abi" in
        arm32:armeabi-v7a|arm64:arm64-v8a|arm32:|arm64:) ;;
        *) echo "Mini-XML Android ABI unavailable: $bits $abi" >&2; exit 1 ;;
    esac
fi
case "$1:${platform:-$OSTYPE}" in
    wasm*:*) librariesrely+=($'vendor\t'"tools/mxml/lib/wasm/static/$bits/$archive") ;;
    *:[Ww]indows|*:msys*|*:cygwin*|*:win32*)
        config=$3
        if [[ $config == debug ]]; then archive=mxmlstatd.lib; else archive=mxmlstat.lib; fi
        librariesrely+=($'link\t'"$archive") ;;
    *) librariesrely+=($'link\t'"$archive") ;;
esac
THIRDPARTY_FILES="$archive" bash "$root/tools/thirdparty.sh" mxml "$1" lib "$config" "$4" || exit 1

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return