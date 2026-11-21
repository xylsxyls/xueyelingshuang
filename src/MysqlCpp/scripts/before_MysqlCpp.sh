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
    "$CLOUD_REBUILD" CStringManager "$1" "$2" "$3" "$4"
fi
librariesrely+=($'project\tCStringManager\t'"$2")

SHELL_FOLDER=$(cd "$(dirname "$0")"; pwd)
root="$SHELL_FOLDER/../../.."
case "${THIRDPARTY_PLATFORM:-${CMAKE_SYSTEM_MysqlCpp:-$OSTYPE}}" in
    [Ww]indows|msys*|cygwin*|win32*) windows=1 ;;
    [Ll]inux*|arm) windows=0 ;;
    *) echo "MysqlCpp supports Windows and Linux" >&2; exit 2 ;;
esac
if [[ $windows == 1 ]]; then
    suffix=""
    if [[ $3 == debug ]]; then suffix=d; fi
    vendor_archive="mysqlcppconnstatic${suffix}.lib"
    THIRDPARTY_INCLUDE_NAME=mysql-connector-cpp THIRDPARTY_FILES="$vendor_archive" \
        bash "$root/tools/thirdparty.sh" mysqlcpp "$1" lib "$3" "$4" || exit $?
    # Oracle 的发布 DLL 供该架构的所有封装配置运行时加载。
    THIRDPARTY_CRT=md THIRDPARTY_FILES=$'libmysql.dll\nlibmysql.lib' \
        bash "$root/tools/thirdparty.sh" mysqlclient "$1" dll release "$4" || exit $?
else
    vendor_archive=libmysqlcppconn-static.a
    THIRDPARTY_INCLUDE_NAME=mysql-connector-cpp THIRDPARTY_FILES="$vendor_archive" bash "$root/tools/thirdparty.sh" mysqlcpp "$1" lib release "$4" || exit 1
    bash "$root/tools/thirdparty.sh" mysqlclient "$1" dll release "$4" || exit 1
    librariesrely+=($'link\tlibmysqlclient.so')
fi
librariesrely+=($'link\t'"$vendor_archive")

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return