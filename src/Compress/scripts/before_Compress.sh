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

root=$(cd "$(dirname "$0")/../../.." && pwd) || exit 1
records=$(bash "$root/tools/thirdparty.sh" --print-rely zstandard "$1" lib "$3" "$4") || exit 1
while IFS= read -r record; do
    if [[ -n $record ]]; then librariesrely+=("$record"); fi
done <<< "$records"

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return