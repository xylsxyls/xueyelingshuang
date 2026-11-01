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
thirdpartypath="$xueyelingshuang/tools/mxml"

if [[ $1 == wasm32 || $1 == wasm64 ]]; then
    thirdpartyarchive="libmxml${1#wasm}.a"
    if [[ ! -f "$thirdpartypath/lib/wasm/$thirdpartyarchive" ]]; then
        echo "Missing Mini-XML Wasm SDK: $thirdpartypath/lib/wasm/$thirdpartyarchive" >&2
        false
    else
        mkdir -p "$sdkpath/include/mxml" "$sdkpath/lib"
        cp -Rf "$thirdpartypath/include/." "$sdkpath/include/mxml/"
        cp -f "$thirdpartypath/lib/wasm/$thirdpartyarchive" "$sdkpath/lib/"
        librariesrely+=($'vendor\ttools/mxml/lib/wasm/'"$thirdpartyarchive")
    fi
else
    native_target="${CMAKE_SYSTEM_NAME:-}"
    if [[ -z "$native_target" && "${CMAKE_TOOLCHAIN_FILE:-}" == *android.toolchain.cmake ]]; then
        native_target=Android
    fi
    if [[ -z "$native_target" && "${CMAKE_OSX_SYSROOT:-}" == *iphone* ]]; then
        native_target=iOS
    fi
    if [[ -z "$native_target" ]]; then
        case "$OSTYPE" in
            linux*) native_target=Linux ;;
            msys*|cygwin*|win32*) native_target=Windows ;;
            darwin*) native_target=Darwin ;;
        esac
    fi
    case "$native_target" in
        Windows|Linux)
            thirdpartylibpath="$thirdpartypath/lib"
            if [[ "$native_target" == Windows ]]; then
                thirdpartyarchive="mxmlstat_${1}.lib"
                if [[ $3 == debug ]]; then
                    thirdpartylibpath="$thirdpartylibpath/MD"
                    thirdpartyarchive="mxmlstat_${1}d.lib"
                fi
            else
                thirdpartyarchive="libmxmlstatic${1}.a"
            fi
            if [[ ! -f "$thirdpartylibpath/$thirdpartyarchive" ]]; then
                echo "Missing $native_target Mini-XML SDK: $thirdpartylibpath/$thirdpartyarchive; prepare the matching ABI and CRT first." >&2
                false
            else
                mkdir -p "$sdkpath/include/mxml" "$sdkpath/lib"
                cp -Rf "$thirdpartypath/include/." "$sdkpath/include/mxml/"
                cp -f "$thirdpartylibpath/$thirdpartyarchive" "$sdkpath/lib/"
                librariesrely+=($'link\t'"$thirdpartyarchive")
            fi
            ;;
        Darwin|iOS|Android)
            thirdpartyarchive="libmxmlstatic${1}.a"
            if [[ ! -f "$sdkpath/lib/$thirdpartyarchive" || ! -f "$sdkpath/include/mxml/mxml.h" ]]; then
                echo "Prepare the $native_target Mini-XML SDK in $sdkpath first; host archives are not interchangeable." >&2
                false
            else
                librariesrely+=($'link\t'"$thirdpartyarchive")
            fi
            ;;
        *) echo "Unsupported native target: $native_target" >&2; false ;;
    esac
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return