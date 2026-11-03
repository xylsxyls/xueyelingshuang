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
NetChannel_bit=$1
NetChannel_dlllib=$2
NetChannel_debugRelease=$3
NetChannel_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" NetChannel "$NetChannel_bit" "$NetChannel_dlllib" "$NetChannel_debugRelease" "$NetChannel_allSame"
fi
librariesrely+=($'project\tNetChannel\t'"$NetChannel_dlllib")

#--------------------------------------------------------------------
Compress_bit=$1
Compress_dlllib=$2
Compress_debugRelease=$3
Compress_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" Compress "$Compress_bit" "$Compress_dlllib" "$Compress_debugRelease" "$Compress_allSame"
fi
librariesrely+=($'project\tCompress\t'"$Compress_dlllib")

#--------------------------------------------------------------------
ProtoMessage_bit=$1
ProtoMessage_dlllib=$2
ProtoMessage_debugRelease=$3
ProtoMessage_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" ProtoMessage "$ProtoMessage_bit" "$ProtoMessage_dlllib" "$ProtoMessage_debugRelease" "$ProtoMessage_allSame"
fi
librariesrely+=($'project\tProtoMessage\t'"$ProtoMessage_dlllib")

# 交叉编译按目标选择已准备SDK，不把宿主平台归档用于手机或Apple目标
SHELL_FOLDER=$(cd "$(dirname "$0")" && pwd)
xueyelingshuang=$SHELL_FOLDER/../../..
thirdpartylib="mbedtls"
thirdpartylibname="mbedtls"
sdkpath="${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}"
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
        darwin*) native_target=Darwin ;;
        msys*|cygwin*|win32*) native_target=Windows ;;
    esac
fi

if [[ $1 == wasm32 || $1 == wasm64 ]]; then
    thirdpartybit=${1#wasm}
    thirdpartyarchive="lib${thirdpartylibname}${thirdpartybit}.a"
    thirdpartylibrary="wasm/$thirdpartyarchive"
    thirdpartyheaders="${thirdpartylib}wasm"
else
    thirdpartybit=$1
    case "$native_target" in
        Windows)
            thirdpartyarchive="${thirdpartylibname}static${thirdpartybit}.lib"
            thirdpartylibrary="$thirdpartyarchive"
            thirdpartyheaders="${thirdpartylib}windows"
            if [[ $3 == debug ]]; then
                thirdpartyarchive="${thirdpartylibname}static${thirdpartybit}d.lib"
                thirdpartylibrary="MD/$thirdpartyarchive"
            fi
            ;;
        Linux)
            thirdpartyarchive="lib${thirdpartylibname}static${thirdpartybit}.a"
            thirdpartylibrary="$thirdpartyarchive"
            thirdpartyheaders="${thirdpartylib}linux"
            ;;
        Darwin|iOS|Android)
            thirdpartyarchive="lib${thirdpartylibname}static${thirdpartybit}.a"
            ;;
        *) echo "Unsupported native target: $native_target" >&2; false ;;
    esac
fi

if [[ -n "${thirdpartylibrary:-}" ]]; then
    thirdpartyinclude="$xueyelingshuang/tools/$thirdpartylib/include/$thirdpartyheaders"
    thirdpartysource="$xueyelingshuang/tools/$thirdpartylib/lib/$thirdpartylibrary"
    if [[ -f "$thirdpartysource" && -f "$thirdpartyinclude/mbedtls_sdk_config.h" ]]; then
        mkdir -p "$sdkpath/include/$thirdpartylib" "$sdkpath/lib"
        cp -Rf "$thirdpartyinclude/." "$sdkpath/include/$thirdpartylib/"
        cp -f "$thirdpartysource" "$sdkpath/lib/$thirdpartyarchive"
    else
        echo "Missing prepared $thirdpartylib SDK: $thirdpartysource" >&2
        false
    fi
else
    # 尚无仓库预编译包的目标，由调用方提前在独立SDK中准备同目标的头与库
    if [[ ! -f "$sdkpath/include/$thirdpartylib/mbedtls_sdk_config.h" || ! -f "$sdkpath/lib/$thirdpartyarchive" ]]; then
        echo "Missing prepared $native_target $thirdpartylib SDK in $sdkpath" >&2
        false
    fi
fi
if [[ $1 == wasm32 || $1 == wasm64 ]]; then
    librariesrely+=($'vendor\t'"tools/$thirdpartylib/lib/$thirdpartylibrary")
else
    librariesrely+=($'link\t'"$thirdpartyarchive")
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return