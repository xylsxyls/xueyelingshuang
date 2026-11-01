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

if [[ $1 != wasm32 && $1 != wasm64 ]]; then
SHELL_FOLDER=$(cd "$(dirname "$0")"; pwd)
xueyelingshuang="$SHELL_FOLDER/../../.."
sdkpath="${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}"
native_target="${CMAKE_SYSTEM_NAME:-}"
if [[ -z "$native_target" && "${CMAKE_TOOLCHAIN_FILE:-}" == *android.toolchain.cmake ]]; then native_target=Android; fi
if [[ -z "$native_target" && "${CMAKE_OSX_SYSROOT:-}" == *iphone* ]]; then native_target=iOS; fi
if [[ -z "$native_target" ]]; then
    case "$OSTYPE" in
        linux*) native_target=Linux ;;
        msys*|cygwin*|win32*) native_target=Windows ;;
        darwin*) native_target=Darwin ;;
    esac
fi
sdk_debug=""
if [[ $3 == debug ]]; then sdk_debug=d; fi
case "$native_target" in
    Windows)
        vendor_archives=("hiredisstatic${1}${sdk_debug}.lib" "Win32_Interopstatic${1}${sdk_debug}.lib")
        vendor_headers="$xueyelingshuang/tools/hiredis/include/hirediswindows"
        ;;
    Linux)
        vendor_archives=("libhiredisstatic${1}.a")
        vendor_headers="$xueyelingshuang/tools/hiredis/include/hiredislinux"
        ;;
    Darwin|iOS|Android)
        vendor_archives=("libhiredisstatic${1}.a")
        vendor_headers=""
        if [[ ! -f "$sdkpath/include/hiredisinclude/hiredis.h" ]]; then
            echo "Prepare $native_target hiredis headers in $sdkpath/include/hiredisinclude first." >&2
            false
        fi
        ;;
    *) echo "Unsupported native target: $native_target" >&2; false ;;
esac
mkdir -p "$sdkpath/include/hiredisinclude" "$sdkpath/lib"
if [[ -n "$vendor_headers" ]]; then
    cp -Rf "$vendor_headers/." "$sdkpath/include/hiredisinclude/"
fi
for vendor_archive in "${vendor_archives[@]}"; do
    if [[ -n "$vendor_headers" ]]; then
        cp -f "$xueyelingshuang/tools/hiredis/lib/$vendor_archive" "$sdkpath/lib/"
    elif [[ ! -f "$sdkpath/lib/$vendor_archive" ]]; then
        echo "Prepare the matching $native_target hiredis archive: $sdkpath/lib/$vendor_archive" >&2
        false
    fi
    librariesrely+=($'link\t'"$vendor_archive")
done
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return
