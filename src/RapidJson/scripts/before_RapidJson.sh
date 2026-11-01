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

SHELL_FOLDER=$(cd "$(dirname "$0")"; pwd)
xueyelingshuang="$SHELL_FOLDER/../../.."
sdkpath="${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}"
if [[ $1 == wasm32 || $1 == wasm64 ]]; then
    sdkpath="${CLOUD_REBUILD_PRIVATE_INCLUDE:?CloudRebuild must provide the private Wasm include directory}"
else
    sdkpath="$sdkpath/include"
fi
mkdir -p "$sdkpath/rapid_json"
cp -Rf "$xueyelingshuang/tools/rapid_json/." "$sdkpath/rapid_json/"

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return