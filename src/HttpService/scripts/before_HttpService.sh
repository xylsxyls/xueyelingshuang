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

if [[ $1 == wasm32 || $1 == wasm64 ]]; then
    echo 'HttpService server targets Windows and Linux' >&2
    exit 1
fi
case "${THIRDPARTY_PLATFORM:-${CMAKE_SYSTEM_NAME:-}}:${CMAKE_TOOLCHAIN_FILE:-}:$OSTYPE" in
    :*android.toolchain.cmake:*) echo 'HttpService server targets Windows and Linux' >&2; exit 1 ;;
    windows:*|Windows:*|:*:msys*|:*:cygwin*|:*:win32*)
        archive=civetweb.lib
        [[ $3 != debug ]] || archive=civetwebd.lib
        config=$3 ;;
    linux:*|Linux:*|arm:*|:*:linux*) archive=libcivetweb.a; config=release ;;
    *) echo 'HttpService server targets Windows and Linux' >&2; exit 1 ;;
esac

#--------------------------------------------------------------------
CStringManager_bit=$1
CStringManager_dlllib=$2
CStringManager_debugRelease=$3
CStringManager_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CStringManager "$CStringManager_bit" "$CStringManager_dlllib" "$CStringManager_debugRelease" "$CStringManager_allSame"
fi
librariesrely+=($'project\tCStringManager\t'"$CStringManager_dlllib")

#--------------------------------------------------------------------
CSystem_bit=$1
CSystem_dlllib=$2
CSystem_debugRelease=$3
CSystem_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CSystem "$CSystem_bit" "$CSystem_dlllib" "$CSystem_debugRelease" "$CSystem_allSame"
fi
librariesrely+=($'project\tCSystem\t'"$CSystem_dlllib")

root=$(cd "$(dirname "$0")/../../.." && pwd)
THIRDPARTY_FILES="$archive" bash "$root/tools/thirdparty.sh" civetweb "$1" lib "$config" "$4" || exit 1
librariesrely+=($'link\t'"$archive")

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return