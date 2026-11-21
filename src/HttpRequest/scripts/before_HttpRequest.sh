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

if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CEncodeDecode "$1" "$2" "$3" "$4"
fi
librariesrely+=($'project\tCEncodeDecode\t'"$2")

root=$(cd "$(dirname "$0")/../../.." && pwd)
if [[ $1 != wasm32 && $1 != wasm64 ]]; then
    case "${THIRDPARTY_PLATFORM:-${CMAKE_SYSTEM_NAME:-}}:${CMAKE_TOOLCHAIN_FILE:-}:$OSTYPE" in
        :*android.toolchain.cmake:*) archive=libcurl.a; linkage=lib; config=release ;;
        windows:*|Windows:*|:*:msys*|:*:cygwin*|:*:win32*)
            archive=libcurl.lib
            [[ $3 != debug ]] || archive=libcurld.lib
            linkage=lib; config=$3 ;;
        linux:*|Linux:*|arm:*|:*:linux*) archive=libcurl.so.4; linkage=dll; config=release ;;
        *) archive=libcurl.a; linkage=lib; config=release ;;
    esac
    THIRDPARTY_INCLUDE_NAME=curl THIRDPARTY_FILES="$archive" bash "$root/tools/thirdparty.sh" libcurl "$1" "$linkage" "$config" "$4" || exit 1
    librariesrely+=($'link\t'"$archive")
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return