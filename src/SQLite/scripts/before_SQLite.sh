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

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return