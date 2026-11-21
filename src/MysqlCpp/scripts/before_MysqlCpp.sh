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
SqlString_bit=$1
SqlString_dlllib=$2
SqlString_debugRelease=$3
SqlString_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" SqlString "$SqlString_bit" "$SqlString_dlllib" "$SqlString_debugRelease" "$SqlString_allSame"
fi
librariesrely+=($'project\tSqlString\t'"$SqlString_dlllib")

#--------------------------------------------------------------------
CSystem_bit=$1
CSystem_dlllib=$2
CSystem_debugRelease=$3
CSystem_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CSystem "$CSystem_bit" "$CSystem_dlllib" "$CSystem_debugRelease" "$CSystem_allSame"
fi
librariesrely+=($'project\tCSystem\t'"$CSystem_dlllib")

SHELL_FOLDER=$(cd "$(dirname "$0")"; pwd)
root="$SHELL_FOLDER/../../.."
suffix=""
if [[ $3 == debug ]]; then suffix=d; fi
vendor_archive="mysqlcppconnstatic${suffix}.lib"
THIRDPARTY_INCLUDE_NAME=mysql-connector-cpp THIRDPARTY_FILES="$vendor_archive" \
    bash "$root/tools/thirdparty.sh" mysqlcpp "$1" lib "$3" "$4" || exit $?
# Oracle 的发布 DLL 供该架构的所有封装配置运行时加载。
THIRDPARTY_CRT=md THIRDPARTY_FILES=$'libmysql.dll\nlibmysql.lib' \
    bash "$root/tools/thirdparty.sh" mysqlclient "$1" dll release "$4" || exit $?
librariesrely+=($'link\t'"$vendor_archive")

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return
