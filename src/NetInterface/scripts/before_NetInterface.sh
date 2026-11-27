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

#--------------------------------------------------------------------
NetChannel_bit=$1
NetChannel_dlllib=$2
NetChannel_debugRelease=$3
NetChannel_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" NetChannel "$NetChannel_bit" "$NetChannel_dlllib" "$NetChannel_debugRelease" "$NetChannel_allSame"
fi
librariesrely+=($'project\tNetChannel\t'"$NetChannel_dlllib")

#--------------------------------------------------------------------
Compress_bit=$1
Compress_dlllib=$2
Compress_debugRelease=$3
Compress_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" Compress "$Compress_bit" "$Compress_dlllib" "$Compress_debugRelease" "$Compress_allSame"
fi
librariesrely+=($'project\tCompress\t'"$Compress_dlllib")

#--------------------------------------------------------------------
ProtoMessageManager_bit=$1
ProtoMessageManager_dlllib=$2
ProtoMessageManager_debugRelease=$3
ProtoMessageManager_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" ProtoMessageManager "$ProtoMessageManager_bit" "$ProtoMessageManager_dlllib" "$ProtoMessageManager_debugRelease" "$ProtoMessageManager_allSame"
fi
librariesrely+=($'project\tProtoMessageManager\t'"$ProtoMessageManager_dlllib")

#--------------------------------------------------------------------
Variant_bit=$1
Variant_dlllib=$2
Variant_debugRelease=$3
Variant_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" Variant "$Variant_bit" "$Variant_dlllib" "$Variant_debugRelease" "$Variant_allSame"
fi
librariesrely+=($'project\tVariant\t'"$Variant_dlllib")

#--------------------------------------------------------------------
CEncodeDecode_bit=$1
CEncodeDecode_dlllib=$2
CEncodeDecode_debugRelease=$3
CEncodeDecode_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CEncodeDecode "$CEncodeDecode_bit" "$CEncodeDecode_dlllib" "$CEncodeDecode_debugRelease" "$CEncodeDecode_allSame"
fi
librariesrely+=($'project\tCEncodeDecode\t'"$CEncodeDecode_dlllib")


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
LockFreeHashMap_bit=$1
LockFreeHashMap_dlllib=$2
LockFreeHashMap_debugRelease=$3
LockFreeHashMap_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" LockFreeHashMap "$LockFreeHashMap_bit" "$LockFreeHashMap_dlllib" "$LockFreeHashMap_debugRelease" "$LockFreeHashMap_allSame"
fi
librariesrely+=($'project\tLockFreeHashMap\t'"$LockFreeHashMap_dlllib")

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return