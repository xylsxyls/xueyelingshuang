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
CStringManager_bit=$1
CStringManager_dlllib=$2
CStringManager_debugRelease=$3
CStringManager_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CStringManager "$CStringManager_bit" "$CStringManager_dlllib" "$CStringManager_debugRelease" "$CStringManager_allSame"
fi
librariesrely+=($'project\tCStringManager\t'"$CStringManager_dlllib")

SHELL_FOLDER=$(cd "$(dirname "$0")"; pwd)
root="$SHELL_FOLDER/../../.."
case "${THIRDPARTY_PLATFORM:-${CMAKE_SYSTEM_HiRedis:-$OSTYPE}}" in
    [Ww]indows|msys*|cygwin*|win32*) windows=1 ;;
    [Ll]inux*|arm) windows=0 ;;
    *) echo "HiRedis supports Windows and Linux" >&2; exit 2 ;;
esac
if [[ $windows == 1 ]]; then
    suffix=""
    if [[ $3 == debug ]]; then suffix=d; fi
    vendor_archive="hiredisstatic${suffix}.lib"
    THIRDPARTY_INCLUDE_NAME=hiredisinclude THIRDPARTY_FILES="$vendor_archive" \
        bash "$root/tools/thirdparty.sh" hiredis "$1" lib "$3" "$4" || exit $?
else
    vendor_archive=libhiredis.a
    THIRDPARTY_INCLUDE_NAME=hiredisinclude THIRDPARTY_FILES="$vendor_archive" bash "$root/tools/thirdparty.sh" hiredis "$1" lib release "$4" || exit 1
fi
librariesrely+=($'link\t'"$vendor_archive")

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return