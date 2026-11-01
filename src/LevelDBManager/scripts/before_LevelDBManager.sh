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
thirdpartypath="$xueyelingshuang/tools/leveldb"

if [[ $1 != wasm32 && $1 != wasm64 ]]; then
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
            if [[ "$native_target" == Windows ]]; then
                thirdpartyarchive="leveldbstatic${1}.lib"
            else
                thirdpartyarchive="libleveldbstatic${1}.a"
            fi
            if [[ ! -f "$thirdpartypath/lib/$thirdpartyarchive" ]]; then
                echo "Missing $native_target LevelDB SDK: $thirdpartypath/lib/$thirdpartyarchive; prepare the matching ABI and CRT first." >&2
                false
            else
                mkdir -p "$sdkpath/include" "$sdkpath/lib"
                cp -Rf "$thirdpartypath/include/." "$sdkpath/include/"
                cp -f "$thirdpartypath/lib/$thirdpartyarchive" "$sdkpath/lib/"
                librariesrely+=($'link\t'"$thirdpartyarchive")
            fi
            ;;
        Darwin|iOS|Android)
            thirdpartyarchive="libleveldbstatic${1}.a"
            if [[ ! -f "$sdkpath/lib/$thirdpartyarchive" || ! -f "$sdkpath/include/leveldb/db.h" ]]; then
                echo "Prepare the $native_target LevelDB SDK in $sdkpath first; host archives are not interchangeable." >&2
                false
            else
                librariesrely+=($'link\t'"$thirdpartyarchive")
            fi
            ;;
        *) echo "Unsupported native target: $native_target" >&2; false ;;
    esac
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return
