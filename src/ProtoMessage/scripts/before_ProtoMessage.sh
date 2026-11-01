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
    "$CLOUD_REBUILD" Variant "$Variant_bit" "$Variant_dlllib" "$Variant_debugRelease" "$Variant_allSame"
fi
librariesrely+=($'project\t'"Variant"$'\t'"$Variant_dlllib")


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
            if [[ ! -f "$native_sdk/include/protobuf/google/protobuf/message_lite.h" ]] || [[ ! -f "$native_sdk/lib/libprotobuf-litestatic${1}.a" ]]; then
                echo "Prepare the $native_target protobuf headers and libprotobuf-litestatic${1}.a in $native_sdk first; host libraries are not interchangeable." >&2
                false
            fi
            librariesrely+=($'link\t'"libprotobuf-litestatic${1}.a")
            native_prepared=1
            ;;
        Linux|Windows) ;;
        *) echo "Unsupported native target: $native_target" >&2; false ;;
    esac
fi

if [[ ${native_prepared:-0} != 1 ]]; then
SHELL_FOLDER=$(cd "$(dirname "$0")" && pwd)
xueyelingshuang=$SHELL_FOLDER/../../..

thirdpartylib="protobuf"
thirdpartywasmlibname="libprotobuf-lite"
sdkpath="${CLOUD_REBUILD_SDK_ROOT:-$xueyelingshuang}"
if [[ $1 == 'wasm32' ]] || [[ $1 == 'wasm64' ]]; then
    thirdpartybit=${1#wasm}
    # Prepared protobuf-lite has one archive per ABI, shared by Debug and Release.
    thirdpartyarchive="${thirdpartywasmlibname}${thirdpartybit}.a"
    mkdir -p "$sdkpath/include/$thirdpartylib" "$sdkpath/lib"
    cp -Rf "$xueyelingshuang/tools/$thirdpartylib/include/${thirdpartylib}wasm/." "$sdkpath/include/$thirdpartylib/"
    cp -f "$xueyelingshuang/tools/$thirdpartylib/lib/wasm/$thirdpartyarchive" "$sdkpath/lib/"
    librariesrely+=($'vendor\t'"tools/$thirdpartylib/lib/wasm/$thirdpartyarchive")
else
    # Native protobuf headers are shared across native platforms in this SDK.
    nativeinclude="$xueyelingshuang/tools/$thirdpartylib/include"
    thirdpartylibname="libprotobuf-lite"
    thirdpartybit=$1
    thirdpartysuffix=".lib"
    thirdpartydebug=""
    if [[ "$native_target" == Linux ]]; then
        thirdpartysuffix=".a"
    elif [[ $3 == 'debug' ]]; then
        thirdpartydebug="d"
    fi
    thirdpartyarchive="${thirdpartylibname}static${thirdpartybit}${thirdpartydebug}${thirdpartysuffix}"
    if [[ ! -f "$xueyelingshuang/tools/$thirdpartylib/lib/$thirdpartyarchive" ]]; then
        echo "Missing prepared protobuf SDK: $thirdpartyarchive" >&2
        false
    fi
    mkdir -p "$sdkpath/include/$thirdpartylib" "$sdkpath/lib"
    cp -Rf "$nativeinclude/google" "$sdkpath/include/$thirdpartylib/"
    cp -f "$xueyelingshuang/tools/$thirdpartylib/lib/$thirdpartyarchive" "$sdkpath/lib/"
    protocname="protoc.exe"
    if [[ "$OSTYPE" =~ ^linux ]]; then
        protocname="protoc"
    fi
    "$xueyelingshuang/tools/protobuf/$protocname" \
        -I="$xueyelingshuang/src/ProtoMessage/ProtoMessage/src" \
        --cpp_out="$xueyelingshuang/src/ProtoMessage/ProtoMessage/src" \
        "$xueyelingshuang/src/ProtoMessage/ProtoMessage/src/ProtoMessage.proto"
fi
if [[ $1 != wasm32 && $1 != wasm64 ]]; then
    librariesrely+=($'link\t'"$thirdpartyarchive")
fi
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return