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
ProcessWork_bit=$1
ProcessWork_dlllib=$2
ProcessWork_debugRelease=$3
ProcessWork_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" ProcessWork "$ProcessWork_bit" "$ProcessWork_dlllib" "$ProcessWork_debugRelease" "$ProcessWork_allSame"
fi
librariesrely+=($'project\tProcessWork\t'"$ProcessWork_dlllib")

#--------------------------------------------------------------------
ProtoMessage_bit=$1
ProtoMessage_dlllib=$2
ProtoMessage_debugRelease=$3
ProtoMessage_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" ProtoMessage "$ProtoMessage_bit" "$ProtoMessage_dlllib" "$ProtoMessage_debugRelease" "$ProtoMessage_allSame"
fi
librariesrely+=($'project\tProtoMessage\t'"$ProtoMessage_dlllib")

#--------------------------------------------------------------------
NetInterface_bit=$1
NetInterface_dlllib=$2
NetInterface_debugRelease=$3
NetInterface_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" NetInterface "$NetInterface_bit" "$NetInterface_dlllib" "$NetInterface_debugRelease" "$NetInterface_allSame"
fi
librariesrely+=($'project\tNetInterface\t'"$NetInterface_dlllib")

#--------------------------------------------------------------------
CorrespondParam_bit=$1
CorrespondParam_dlllib=$2
CorrespondParam_debugRelease=$3
CorrespondParam_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CorrespondParam "$CorrespondParam_bit" "$CorrespondParam_dlllib" "$CorrespondParam_debugRelease" "$CorrespondParam_allSame"
fi
librariesrely+=($'project\tCorrespondParam\t'"$CorrespondParam_dlllib")

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return