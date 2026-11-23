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
    "$CLOUD_REBUILD" CStringManager "$CStringManager_bit" "$CStringManager_dlllib" "$CStringManager_debugRelease" "$CStringManager_allSame" || exit 1
fi
librariesrely+=($'project\tCStringManager\t'"$CStringManager_dlllib")

thirdparty="${XUEYELINGSHUANG}tools/thirdparty.sh"
platform=$(bash "$thirdparty" --target-platform "$1") || exit 1
case "$platform" in
    windows|linux|arm) ;;
    *) echo "MysqlCpp supports Windows and Linux" >&2; exit 1 ;;
esac

records=$(THIRDPARTY_INCLUDE_NAME=mysql-connector-cpp bash "$thirdparty" --print-rely mysqlcpp "$1" lib "$3" "$4") || exit 1
while IFS= read -r record; do
    if [[ -n $record ]]; then librariesrely+=("$record"); fi
done <<< "$records"

# Windows 的连接器在运行时加载客户端 DLL，仅发布；Linux 还需要链接客户端。
if [[ $platform == windows ]]; then
    THIRDPARTY_CRT=md bash "$thirdparty" mysqlclient "$1" dll "$3" "$4" || exit 1
else
    records=$(bash "$thirdparty" --print-rely mysqlclient "$1" dll "$3" "$4") || exit 1
    while IFS= read -r record; do
        if [[ -n $record ]]; then librariesrely+=("$record"); fi
    done <<< "$records"
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return