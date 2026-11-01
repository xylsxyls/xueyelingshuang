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

#--------------------------------------------------------------------
Semaphore_bit=$1
Semaphore_dlllib=$2
Semaphore_debugRelease=$3
Semaphore_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" Semaphore "$Semaphore_bit" "$Semaphore_dlllib" "$Semaphore_debugRelease" "$Semaphore_allSame"
fi
librariesrely+=($'project\tSemaphore\t'"$Semaphore_dlllib")

#--------------------------------------------------------------------
CStringManager_bit=$1
CStringManager_dlllib=$2
CStringManager_debugRelease=$3
CStringManager_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CStringManager "$CStringManager_bit" "$CStringManager_dlllib" "$CStringManager_debugRelease" "$CStringManager_allSame"
fi
librariesrely+=($'project\tCStringManager\t'"$CStringManager_dlllib")

if [[ $1 != wasm32 && $1 != wasm64 ]]; then
SHELL_FOLDER=$(cd "$(dirname "$0")"; pwd)
xueyelingshuang="$SHELL_FOLDER/../../.."
sdkpath="${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}"
native_target="${CMAKE_SYSTEM_NAME:-}"
if [[ -z "$native_target" && "${CMAKE_TOOLCHAIN_FILE:-}" == *android.toolchain.cmake ]]; then native_target=Android; fi
if [[ -z "$native_target" && "${CMAKE_OSX_SYSROOT:-}" == *iphone* ]]; then native_target=iOS; fi
if [[ -z "$native_target" ]]; then
    case "$OSTYPE" in
        linux*) native_target=Linux ;;
        msys*|cygwin*|win32*) native_target=Windows ;;
        darwin*) native_target=Darwin ;;
    esac
fi
sdk_debug=""
if [[ $3 == debug ]]; then sdk_debug=d; fi
case "$native_target" in
    Windows)
        vendor_archives=("zookeeperstatic${1}${sdk_debug}.lib" "hashtablestatic${1}${sdk_debug}.lib")
        vendor_headers="$xueyelingshuang/tools/zookeeper/include"
        ;;
    Linux)
        vendor_archives=("libzookeeper_mtstatic${1}.a")
        vendor_headers="$xueyelingshuang/tools/zookeeper/include"
        ;;
    Darwin|iOS|Android)
        vendor_archives=("libzookeeper_mtstatic${1}.a")
        vendor_headers=""
        if [[ ! -f "$sdkpath/include/zookeeper/zookeeper.h" ]]; then
            echo "Prepare $native_target zookeeper headers in $sdkpath/include/zookeeper first." >&2
            false
        fi
        ;;
    *) echo "Unsupported native target: $native_target" >&2; false ;;
esac
mkdir -p "$sdkpath/include/zookeeper" "$sdkpath/lib"
if [[ -n "$vendor_headers" ]]; then
    cp -Rf "$vendor_headers/." "$sdkpath/include/zookeeper/"
fi
for vendor_archive in "${vendor_archives[@]}"; do
    if [[ -n "$vendor_headers" ]]; then
        cp -f "$xueyelingshuang/tools/zookeeper/lib/$vendor_archive" "$sdkpath/lib/"
    elif [[ ! -f "$sdkpath/lib/$vendor_archive" ]]; then
        echo "Prepare the matching $native_target zookeeper archive: $sdkpath/lib/$vendor_archive" >&2
        false
    fi
    librariesrely+=($'link\t'"$vendor_archive")
done
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return
