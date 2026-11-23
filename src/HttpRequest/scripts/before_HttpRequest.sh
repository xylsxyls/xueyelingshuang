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
CEncodeDecode_bit=$1
CEncodeDecode_dlllib=$2
CEncodeDecode_debugRelease=$3
CEncodeDecode_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CEncodeDecode "$CEncodeDecode_bit" "$CEncodeDecode_dlllib" "$CEncodeDecode_debugRelease" "$CEncodeDecode_allSame" || exit 1
fi
librariesrely+=($'project\tCEncodeDecode\t'"$CEncodeDecode_dlllib")

thirdparty="${XUEYELINGSHUANG}tools/thirdparty.sh"
platform=$(bash "$thirdparty" --target-platform "$1") || exit 1
if [[ $platform != wasm ]]; then
    linkage=lib
    if [[ $platform == linux || $platform == arm ]]; then linkage=dll; fi
    records=$(THIRDPARTY_INCLUDE_NAME=curl bash "$thirdparty" --print-rely libcurl "$1" "$linkage" "$3" "$4") || exit 1
    while IFS= read -r record; do
        if [[ -n $record ]]; then librariesrely+=("$record"); fi
    done <<< "$records"
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return