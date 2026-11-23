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

thirdparty="${XUEYELINGSHUANG}tools/thirdparty.sh"
platform=$(bash "$thirdparty" --target-platform "$1") || exit 1
vendors=(ffmpeg soundtouch-2.4.1)
if [[ $platform == windows ]]; then
    #--------------------------------------------------------------------
    CStringManager_bit=$1
    CStringManager_dlllib=$2
    CStringManager_debugRelease=$3
    CStringManager_allSame=$4
    if [[ $4 != only ]]; then
        "$CLOUD_REBUILD" CStringManager "$CStringManager_bit" "$CStringManager_dlllib" "$CStringManager_debugRelease" "$CStringManager_allSame" || exit 1
    fi
    librariesrely+=($'project\tCStringManager\t'"$CStringManager_dlllib")
    linkage=dll
else
    linkage=lib
    vendors+=(openh264)
fi
for vendor in "${vendors[@]}"; do
    records=$(bash "$thirdparty" --print-rely "$vendor" "$1" "$linkage" "$3" "$4") || exit 1
    while IFS= read -r record; do
        if [[ -n $record ]]; then librariesrely+=("$record"); fi
    done <<< "$records"
done

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return