
:<<!EOF!
#--------------------------------------------------------------------
DllRelyTest_bit=$1
DllRelyTest_dlllib=$2
DllRelyTest_debugRelease=$3
DllRelyTest_allSame=$4
"$CLOUD_REBUILD" DllRelyTest $DllRelyTest_bit $DllRelyTest_dlllib $DllRelyTest_debugRelease $DllRelyTest_allSame
!EOF!

#--------------------------------------------------------------------
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
elif [[ "$OSTYPE" =~ ^linux ]]; then
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
    if [[ -n "${CLOUD_REBUILD_INPUTS:-}" ]]; then
        printf 'vendor\t%s\n' "tools/$thirdpartylib/lib/wasm/${thirdpartywasmlibname}${thirdpartybit}.a" >> "$CLOUD_REBUILD_INPUTS"
    fi
fi