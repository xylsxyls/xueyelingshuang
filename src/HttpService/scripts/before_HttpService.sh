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
CSystem_bit=$1
CSystem_dlllib=$2
CSystem_debugRelease=$3
CSystem_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" CSystem "$CSystem_bit" "$CSystem_dlllib" "$CSystem_debugRelease" "$CSystem_allSame"
fi
librariesrely+=($'project\tCSystem\t'"$CSystem_dlllib")

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
        vendor_archives=("civetwebstatic${1}${sdk_debug}.lib")
        vendor_headers="$xueyelingshuang/tools/civetweb/include"
        ;;
    Linux)
        vendor_archives=("libcivetwebstatic${1}.a")
        vendor_headers="$xueyelingshuang/tools/civetweb/include"
        ;;
    Darwin|iOS|Android)
        vendor_archives=("libcivetwebstatic${1}.a")
        vendor_headers=""
        if [[ ! -f "$sdkpath/include/civetweb/civetweb.h" ]]; then
            echo "Prepare $native_target civetweb headers in $sdkpath/include/civetweb first." >&2
            false
        fi
        ;;
    *) echo "Unsupported native target: $native_target" >&2; false ;;
esac
mkdir -p "$sdkpath/include/civetweb" "$sdkpath/lib"
if [[ -n "$vendor_headers" ]]; then
    cp -Rf "$vendor_headers/." "$sdkpath/include/civetweb/"
fi
for vendor_archive in "${vendor_archives[@]}"; do
    if [[ -n "$vendor_headers" ]]; then
        cp -f "$xueyelingshuang/tools/civetweb/lib/$vendor_archive" "$sdkpath/lib/"
    elif [[ ! -f "$sdkpath/lib/$vendor_archive" ]]; then
        echo "Prepare the matching $native_target civetweb archive: $sdkpath/lib/$vendor_archive" >&2
        false
    fi
    librariesrely+=($'link\t'"$vendor_archive")
done
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return
