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
case "$1" in 32|wasm32|arm32) bits=x86 ;; 64|wasm64|arm64) bits=x64 ;; *) exit 2 ;; esac
case "${THIRDPARTY_PLATFORM:-${CMAKE_SYSTEM_NAME:-$OSTYPE}}" in
    [Ww]indows|msys*|cygwin*|win32*) windows=1 ;;
    *) windows=0 ;;
esac
config=release
if [[ $1 == wasm* ]]; then
    archive=libkissfft.a
    librariesrely+=($'vendor\t'"tools/KissFFT/lib/wasm/static/$bits/$archive")
else
    if [[ $windows == 1 ]]; then
        suffix=
        if [[ $3 == debug ]]; then suffix=d; config=debug; fi
        archive="kissfft${suffix}.lib"
    else
        archive=libkissfft.a
    fi
    librariesrely+=($'link\t'"$archive")
fi
THIRDPARTY_FILES="$archive" bash "$root/tools/thirdparty.sh" KissFFT "$1" lib "$config" "$4" || exit 1

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return