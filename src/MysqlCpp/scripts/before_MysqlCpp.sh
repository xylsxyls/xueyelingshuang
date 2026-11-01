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
SqlString_bit=$1
SqlString_dlllib=$2
SqlString_debugRelease=$3
SqlString_allSame=$4
if [[ $4 != only ]]; then
    "$CLOUD_REBUILD" SqlString "$SqlString_bit" "$SqlString_dlllib" "$SqlString_debugRelease" "$SqlString_allSame"
fi
librariesrely+=($'project\tSqlString\t'"$SqlString_dlllib")

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
thirdpartypath="$xueyelingshuang/tools/mysqlcpp"
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
        vendor_archive="mysqlcppconnstatic${1}${sdk_debug}.lib"
        vendor_runtime="libmysql.dll"
        ;;
    Linux)
        vendor_archive="libmysqlcppconnstatic${1}.a"
        vendor_runtime="libmysqlclient.so.18"
        ;;
    Darwin|iOS|Android)
        vendor_archive="libmysqlcppconnstatic${1}.a"
        vendor_runtime=""
        if [[ ! -f "$sdkpath/include/cppconn/connection.h" || ! -f "$sdkpath/lib/$vendor_archive" ]]; then
            echo "Prepare the target $native_target MySQL Connector SDK in $sdkpath first." >&2
            false
        fi
        ;;
    *) echo "Unsupported native target: $native_target" >&2; false ;;
esac
if [[ "$native_target" == Windows || "$native_target" == Linux ]]; then
    case "$1" in
        32) vendor_arch=x86 ;;
        64) vendor_arch=x64 ;;
        arm32|arm64) vendor_arch=$1 ;;
        *) echo "Unsupported native BIT: $1" >&2; false ;;
    esac
    mkdir -p "$sdkpath/include/cppconn" "$sdkpath/lib"
    cp -Rf "$thirdpartypath/include/cppconn/." "$sdkpath/include/cppconn/"
    cp -f "$thirdpartypath/lib/$vendor_arch/$vendor_archive" "$sdkpath/lib/"
    cp -f "$thirdpartypath/lib/$vendor_arch/$vendor_runtime" "$sdkpath/lib/"
fi
librariesrely+=($'link\t'"$vendor_archive")
if [[ "$native_target" == Linux ]]; then
    librariesrely+=($'link\t'"$vendor_runtime")
fi
fi

printf 'CLOUD_REBUILD_LIBRARIESRELY\t%s\n' "${librariesrely[@]}" return
