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
# The build host and the native target can differ (Android/iOS cross builds).
if [[ $1 != 'wasm32' ]] && [[ $1 != 'wasm64' ]]; then
    native_target="${CMAKE_SYSTEM_NAME:-}"
    if [[ -z "$native_target" ]]; then
        case "${CMAKE_TOOLCHAIN_FILE:-}" in
            *android.toolchain.cmake) native_target=Android ;;
        esac
    fi
    if [[ -z "$native_target" ]]; then
        case "${CMAKE_OSX_SYSROOT:-}" in
            *iphone*) native_target=iOS ;;
        esac
    fi
    if [[ -z "$native_target" ]]; then
        case "$OSTYPE" in
            linux*) native_target=Linux ;;
            darwin*) native_target=Darwin ;;
            msys*|cygwin*|win32*) native_target=Windows ;;
            *) echo "Unsupported native host: $OSTYPE" >&2; false ;;
        esac
    fi
    case "$native_target" in
        Darwin|iOS|Android)
            native_root=$(cd "$(dirname "$0")/../../.." && pwd)
            native_sdk="${CLOUD_REBUILD_SDK_ROOT:-$native_root}"
            if [[ ! -f "$native_sdk/include/zlib/zlib.h" ]] || [[ ! -f "$native_sdk/lib/libzstatic${1}.a" ]]; then
                echo "Prepare the $native_target zlib headers and libzstatic${1}.a in $native_sdk first; host libraries are not interchangeable." >&2
                false
            fi
            librariesrely+=($'link\t'"libzstatic${1}.a")
            native_prepared=1
            ;;
        Linux|Windows) ;;
        *) echo "Unsupported native target: $native_target" >&2; false ;;
    esac
fi

if [[ ${native_prepared:-0} != 1 ]]; then
SHELL_FOLDER=$(cd $(dirname $0); pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

#该写法支持目录下含有子目录和空格
thirdpartylib="zlib"
thirdpartywasmlibname="libz"
thirdpartypath="$xueyelingshuang/tools/$thirdpartylib"
sdkpath="${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}"
thirdpartybit=$1

if [[ $1 == 'wasm32' ]] || [[ $1 == 'wasm64' ]]; then
    thirdpartybit=${1#wasm}
    thirdpartyincludepath="$thirdpartypath/include/${thirdpartylib}wasm"
    # The prepared Wasm zlib has one archive per ABI, shared by Debug and Release.
    thirdpartylibpath="$thirdpartypath/lib/wasm/${thirdpartywasmlibname}${thirdpartybit}.a"
elif [[ "$native_target" == Linux ]]; then
    if [[ $1 == 'arm64' ]]; then
        thirdpartyincludepath="$thirdpartypath/include/${thirdpartylib}arm64"
    else
        thirdpartyincludepath="$thirdpartypath/include/${thirdpartylib}linux"
    fi
    # This zlib SDK provides static archives for both Compress dll and lib builds.
    thirdpartylibpath="$thirdpartypath/lib/libzstatic${thirdpartybit}.a"
else
    thirdpartyincludepath="$thirdpartypath/include/${thirdpartylib}windows"
    if [[ $3 == 'debug' ]]; then
        thirdpartylibpath="$thirdpartypath/lib/MD/${thirdpartylib}static${thirdpartybit}d.lib"
    else
        thirdpartylibpath="$thirdpartypath/lib/${thirdpartylib}static${thirdpartybit}.lib"
    fi
fi

mkdir -p "$sdkpath/include/$thirdpartylib" "$sdkpath/lib"
cp -Rf "$thirdpartyincludepath/." "$sdkpath/include/$thirdpartylib/"
cp -f "$thirdpartylibpath" "$sdkpath/lib/"
if [[ $1 == 'wasm32' ]] || [[ $1 == 'wasm64' ]]; then
    librariesrely+=($'vendor\t'"tools/$thirdpartylib/lib/wasm/${thirdpartywasmlibname}${thirdpartybit}.a")
fi
if [[ $1 != wasm32 && $1 != wasm64 ]]; then
    librariesrely+=($'link\t'"${thirdpartylibpath##*/}")
fi
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return