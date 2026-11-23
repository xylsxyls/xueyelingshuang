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
# Android 包交付的是 armeabi-v7a 和 arm64-v8a，分别存于 x86 和 x64 位宽目录。
if [[ $1 == android32 || $1 == android64 ]]; then
    abi=${CMAKE_ANDROID_ARCH_ABI:-${ANDROID_ABI:-}}
    case "$1:$abi" in
        android32:armeabi-v7a|android64:arm64-v8a|android32:|android64:) ;;
        *) echo "Mini-XML Android ABI unavailable: $1 $abi" >&2; exit 1 ;;
    esac
fi
records=$(bash "$thirdparty" --print-rely mxml "$1" lib "$3" "$4") || exit 1
while IFS= read -r record; do
    if [[ -n $record ]]; then librariesrely+=("$record"); fi
done <<< "$records"

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return