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
LockFreeQueue_bit=$1
LockFreeQueue_dlllib=$2
LockFreeQueue_debugRelease=$3
LockFreeQueue_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" LockFreeQueue "$LockFreeQueue_bit" "$LockFreeQueue_dlllib" "$LockFreeQueue_debugRelease" "$LockFreeQueue_allSame"
fi
librariesrely+=($'project\tLockFreeQueue\t'"$LockFreeQueue_dlllib")

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
LockFreeHashMap_bit=$1
LockFreeHashMap_dlllib=$2
LockFreeHashMap_debugRelease=$3
LockFreeHashMap_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" LockFreeHashMap "$LockFreeHashMap_bit" "$LockFreeHashMap_dlllib" "$LockFreeHashMap_debugRelease" "$LockFreeHashMap_allSame"
fi
librariesrely+=($'project\tLockFreeHashMap\t'"$LockFreeHashMap_dlllib")

SHELL_FOLDER=$(cd "$(dirname "$0")" && pwd)
xueyelingshuang="$SHELL_FOLDER/../../.."
sdkpath="${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}"

# Select the target SDK; a cross build must never copy the host's binary archive.
if [[ $1 != wasm32 && $1 != wasm64 ]]; then
    native_target="${CMAKE_SYSTEM_NAME:-}"
    if [[ -z "$native_target" && ${CMAKE_TOOLCHAIN_FILE:-} == *android.toolchain.cmake ]]; then
        native_target=Android
    fi
    if [[ -z "$native_target" && ${CMAKE_OSX_SYSROOT:-} == *iphone* ]]; then
        native_target=iOS
    fi
    if [[ -z "$native_target" ]]; then
        case "$OSTYPE" in
            linux*) native_target=Linux ;;
            darwin*) native_target=Darwin ;;
            msys*|cygwin*|win32*) native_target=Windows ;;
        esac
    fi
    case "$native_target" in
        Windows)
            archive="uvstatic${1}.lib"
            source="$xueyelingshuang/tools/libuv/lib/$archive"
            if [[ $3 == debug ]]; then
                archive="uvstatic${1}d.lib"
                source="$xueyelingshuang/tools/libuv/lib/MD/$archive"
            fi
            ;;
        Linux)
            archive="libuvstatic${1}.a"
            source="$xueyelingshuang/tools/libuv/lib/$archive"
            ;;
        Darwin|iOS|Android)
            archive="libuvstatic${1}.a"
            source=""
            ;;
        *) false ;;
    esac
    if [[ -n "$source" ]]; then
        if [[ -f "$source" && -f "$xueyelingshuang/tools/libuv/include/uv.h" ]]; then
            mkdir -p "$sdkpath/include" "$sdkpath/lib"
            cp -Rf "$xueyelingshuang/tools/libuv/include/." "$sdkpath/include/"
            cp -f "$source" "$sdkpath/lib/$archive"
        else
            false
        fi
    elif [[ ! -f "$sdkpath/include/uv.h" || ! -f "$sdkpath/lib/$archive" ]]; then
        false
    fi
    librariesrely+=($'link\t'"$archive")
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return