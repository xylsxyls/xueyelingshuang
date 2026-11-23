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
Variant_bit=$1
Variant_dlllib=$2
Variant_debugRelease=$3
Variant_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" Variant "$Variant_bit" "$Variant_dlllib" "$Variant_debugRelease" "$Variant_allSame" || exit 1
fi
librariesrely+=($'project\tVariant\t'"$Variant_dlllib")

#--------------------------------------------------------------------
thirdparty="${XUEYELINGSHUANG}tools/thirdparty.sh"
version=3.3.0
package=protobuf
vendor="$package-$version"

records=$(bash "$thirdparty" --print-rely "$vendor" "$1" lib "$3" "$4") || exit 1
while IFS= read -r record; do
    if [[ -n $record ]]; then librariesrely+=("$record"); fi
done <<< "$records"

src="${XUEYELINGSHUANG}src/ProtoMessageManager/ProtoMessageManager/src"
tool=$(bash "$thirdparty" --pause-missing --host-tool "$vendor" protoc) || exit 1
if [[ -n $tool ]]; then
    "${XUEYELINGSHUANG}tools/$vendor/$tool" -I="$src" --cpp_out="$src" "$src/ProtoMessage.proto" || exit 1
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return