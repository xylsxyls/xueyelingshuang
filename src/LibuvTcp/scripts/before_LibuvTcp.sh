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
    "$CLOUD_REBUILD" LockFreeQueue "$LockFreeQueue_bit" "$LockFreeQueue_dlllib" "$LockFreeQueue_debugRelease" "$LockFreeQueue_allSame" || exit 1
fi
librariesrely+=($'project\tLockFreeQueue\t'"$LockFreeQueue_dlllib")

#--------------------------------------------------------------------
Semaphore_bit=$1
Semaphore_dlllib=$2
Semaphore_debugRelease=$3
Semaphore_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" Semaphore "$Semaphore_bit" "$Semaphore_dlllib" "$Semaphore_debugRelease" "$Semaphore_allSame" || exit 1
fi
librariesrely+=($'project\tSemaphore\t'"$Semaphore_dlllib")

#--------------------------------------------------------------------
LockFreeHashMap_bit=$1
LockFreeHashMap_dlllib=$2
LockFreeHashMap_debugRelease=$3
LockFreeHashMap_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" LockFreeHashMap "$LockFreeHashMap_bit" "$LockFreeHashMap_dlllib" "$LockFreeHashMap_debugRelease" "$LockFreeHashMap_allSame" || exit 1
fi
librariesrely+=($'project\tLockFreeHashMap\t'"$LockFreeHashMap_dlllib")

root=$(cd "$(dirname "$0")/../../.." && pwd) || exit 1
config=release
archive=libuv.a
target=${THIRDPARTY_PLATFORM:-${CMAKE_SYSTEM_NAME:-}}
if [[ -z $target && ${CMAKE_TOOLCHAIN_FILE:-} == *android.toolchain.cmake ]]; then target=android; fi
if [[ -z $target && -n ${CMAKE_OSX_SYSROOT:-} ]]; then
    echo "libuv: select the Apple target using THIRDPARTY_PLATFORM" >&2; exit 1
fi
case "$1:${target:-$OSTYPE}" in
    wasm*:*) ;;
    *:[Ww]indows|*:msys*|*:cygwin*|*:win32*)
        case "$1" in 32|64) ;; *) echo "libuv: unsupported Windows target $1" >&2; exit 1 ;; esac
        config=$3
        if [[ $config == debug ]]; then archive=libuvd.lib; else archive=libuv.lib; fi
        ;;
esac
THIRDPARTY_FILES="$archive" bash "$root/tools/thirdparty.sh" libuv "$1" lib "$config" "$4" || exit 1
librariesrely+=($'link\t'"$archive")

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return